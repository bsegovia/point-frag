// ======================================================================== //
// Copyright (C) 2011 Benjamin Segovia                                      //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "renderer_driver.hpp"
#include "renderer_frame.hpp"
#include "renderer_obj.hpp"
#include "renderer.hpp"
#include "hiz.hpp"
#include "rt/rt_camera.hpp"
#include "sys/tasking.hpp"
#include "sys/tasking_utility.hpp"
#include "GL/gl3.h"

#include "image/stb_image.hpp" // XXX debug code

namespace pf
{
  RendererFrame::RendererFrame(Renderer &renderer) :
    RendererObject(renderer),
    org(zero), up(0.f,1.f,0.f), view(1.f, 0.f, 0.f),
    w(0u), h(0u)
  {}
  RendererFrame::~RendererFrame(void) {}
  void RendererFrame::onCompile(void) {}
  void RendererFrame::onUnreferenced(void) {}

  // XXX Debug code to see the actual effect of culling. Press 'l' to save the
  // displayed boxes and 'k' to display these boxes later. There will be a leak
  // here since the last allocated cullSegment will not be allocated.
  static vector<uint32> *savedVisible = NULL;
  static uint32 savedNum = 0;
  uint32 key_l = 0;
  uint32 key_k = 0;
  uint32 key_p = 0;

  /*! Contains the data we need to perform the culling */
  struct HiZCullState
  {
    HiZCullState(Ref<RendererObj> &renderObj, const RTCamera &cam) :
      renderObj(renderObj), visible(renderObj->segments.size()),
      cam(cam)
    {}
    Ref<RendererObj> renderObj; //!< Renderer object we are going to cull
    vector<uint32> visible;      //!< List of visible objects we update
    RTCamera cam;               //!< For frustum culling and HiZ culling
    uint32 visibleNum;          //!< Total number of visible objects
    PF_STRUCT(HiZCullState);
  };

  /*! Perform the HiZ culling (Frustum + Z) on the given segments */
  static Task *HiZCull(HiZCullState *state)
  {
    // Compute the HiZ buffer
    Ref<HiZ> hiz = PF_NEW(HiZ, 128, 64);
    Ref<Task> hizTask = hiz->rayTrace(state->cam, state->renderObj->intersector);

    // Then cull the segments
    Ref<Task> cullTask = spawn<Task>(HERE, [=]
    {
      PerspectiveFrustum fr(state->cam, hiz);
      const uint32 segmentNum = state->renderObj->segments.size();
      state->visibleNum = 0;
      for (uint32 i = 0; i < segmentNum; ++i)
        if (fr.isVisible(state->renderObj->segments[i]))
          state->visible[state->visibleNum++] = i;

      // XXX Saved the currently visible boxes
      if (key_l) {
        if (savedVisible == NULL)
          savedVisible = PF_NEW(vector<uint32>, segmentNum);
        savedNum = state->visibleNum;
        for (uint32 i = 0; i < savedNum; ++i)
          (*savedVisible)[i] = state->visible[i];
      }

      // XXX Restored the previously visible boxes
      if (key_k && savedVisible) {
        for (uint32 i = 0; i < savedNum; ++i)
          state->visible[i] = (*savedVisible)[i];
        state->visibleNum = savedNum;
      }

      // XXX Output the HiZ buffer
      if (key_p) {
        // Output the complete HiZ
        uint8 *rgba = PF_NEW_ARRAY(uint8, 4 * hiz->width * hiz->height);
        hiz->greyRGBA(&rgba);
        stbi_write_tga("hiz.tga", hiz->width, hiz->height, 4, rgba);
        PF_DELETE_ARRAY(rgba);

        // Output the minimum z values
        rgba = PF_NEW_ARRAY(uint8, 4 * hiz->tileXNum * hiz->tileYNum);
        hiz->greyMinRGBA(&rgba);
        stbi_write_tga("hiz_min.tga",  hiz->tileXNum, hiz->tileYNum, 4, rgba);

        // Output the maximum z values
        hiz->greyMaxRGBA(&rgba);
        stbi_write_tga("hiz_max.tga",  hiz->tileXNum, hiz->tileYNum, 4, rgba);
        PF_DELETE_ARRAY(rgba);
      }
    });

    hizTask->starts(cullTask);
    hizTask->scheduled();
    return cullTask;
  }

#define OGL_NAME (this->renderer.driver)

  Task *RendererFrame::display(void)
  {
    PF_ASSERT(this->isCompiled() == true);
    Task *cull = NULL;
    TaskInOut *display = NULL;
    HiZCullState *state = NULL;
    uint32 elemNum = 0;
    RTCamera cam(org, up, view, fov, ratio);

    // XXX right now we only support one or zero object
    if (this->displayList) {
      list<RendererDisplayList::Elem> &elems = this->displayList->getList();
      for (auto it = elems.begin(); it != elems.end(); ++it) elemNum++;
      FATAL_IF(elemNum > 1, "XXX only one object per display list is supported");
      if (elemNum) {
        auto elem = elems.begin();
        FATAL_IF(elem->isIdentity == false, "XXX non identity matrix");
        FATAL_IF(elem->object->getType() != RN_DISPLAYABLE_WAVEFRONT,
          "XXX only wavefront object supported");
        Ref<RendererObj> refObj = elem->object.cast<RendererObj>();
        state = PF_NEW(HiZCullState, refObj, cam);
        cull = HiZCull(state);
      }
    }

    // Be careful we capture the RendererFrame
    this->refInc();

    // There is something to display here
    if (elemNum) {
      display = spawn<TaskInOut>(HERE, [=] {
        const mat4x4f MVP = cam.getMatrix();

        // Set the display viewport
        R_CALL (Viewport, 0, 0, w, h);
        R_CALL (Enable, GL_DEPTH_TEST);
        R_CALL (Enable, GL_CULL_FACE);
        R_CALL (CullFace, GL_BACK);
        R_CALL (DepthFunc, GL_LEQUAL);

        // Clear color buffer with black
        R_CALL (ClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
        R_CALL (ClearDepth, 1.0f);
        R_CALL (Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Display the objects with their textures
        R_CALL (UseProgram, renderer.driver->diffuse.program);
        R_CALL (UniformMatrix4fv, renderer.driver->diffuse.uMVP, 1, GL_FALSE, &MVP[0][0]);
        state->renderObj->display(state->visible, state->visibleNum);
        R_CALL (UseProgram, 0);

        // Display all the bounding boxes
        R_CALL (setMVP, MVP);
        BBox3f *bbox = PF_NEW_ARRAY(BBox3f, state->visibleNum);
        for (size_t i = 0; i < state->visibleNum; ++i) {
          const uint32 segmentID = state->visible[i];
          bbox[i] = state->renderObj->segments[segmentID].bbox;
        }
        R_CALL (displayBBox, bbox, state->visibleNum);
        PF_SAFE_DELETE_ARRAY(bbox);
        PF_SAFE_DELETE(state);
        R_CALL(swapBuffers);
        if (this->refDec()) PF_DELETE(this);
      });
    }
    // We just clear the screen if nothing is displayed
    else {
      display = spawn<TaskInOut>(HERE, [=] {
        R_CALL (Viewport, 0, 0, w, h);
        R_CALL (ClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
        R_CALL (ClearDepth, 1.0f);
        R_CALL (Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        R_CALL(swapBuffers);
        if (this->refDec()) PF_DELETE(this);
      });
    }

    // Force dependencies between renderering tasks
    if (renderer.renderingTask) {
      renderer.renderingTask->multiStarts(display);
      renderer.renderingTask = display;
    }

    // Properly order tasks
    Task *dummy = PF_NEW(TaskDummy);
    display->multiStarts(dummy);
    display->setAffinity(PF_TASK_MAIN_THREAD);
    if (cull) {
      cull->starts(display);
      cull->scheduled();
    }
    display->scheduled();
    return dummy;
  }

#undef OGL_NAME

} /* namespace pf */

