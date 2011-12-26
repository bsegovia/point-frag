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

#include "renderer_context.hpp"
#include "renderer_display_list.hpp"
#include "renderer_frame.hpp"
#include "renderer_obj.hpp"
#include "renderer.hpp"
#include "models/obj.hpp"
#include "sys/mutex.hpp"
#include "sys/default_path.hpp"
#include "sys/logging.hpp"
#include "sys/alloc.hpp"

namespace pf
{
  ///////////////////////////////////////////////////////////////////////////
  // Renderer context
  ///////////////////////////////////////////////////////////////////////////

  // Only one renderer is supported. Creating many contexts will simply add
  // more references on this renderer
  static Renderer *renderer = NULL;
  static MutexSys rendererMutex;

  RnContext rnContextNew(void) {
    if (renderer == NULL) {
      Lock<MutexSys> lock(rendererMutex);
      if (renderer == NULL)
        renderer = PF_NEW(Renderer);
    }
    renderer->refInc();
    return renderer;
  }
  void rnContextDelete(RnContext ctx) {
    if (ctx == NULL) return;
    PF_ASSERT(ctx == renderer);
    if (ctx->refDec()) {
      Lock<MutexSys> lock(rendererMutex);
      PF_DELETE(renderer);
      renderer = NULL;
    }
  }

  /*! Common for all user renderer object creation */
  static void RendererObjectNew(RendererObject *object) {
    PF_ASSERT(object);
    object->refInc();
    object->externalRefInc();
  }
  /*! Common for all user renderer object deletion */
  static void RendererObjectDelete(RendererObject *object) {
    PF_ASSERT(object);
    object->externalRefDec();
    if (object->refDec()) PF_DELETE(object);
  }
  /*! Common for all user renderer object retains */
  static void RendererObjectRetain(RendererObject *object) {
    PF_ASSERT(object);
    object->refInc();
    object->externalRefInc();
  }
  /*! Common for compilable objects */
  template <typename T>
  static void RendererObjectCompile(T *object) {
    PF_ASSERT(object);
    object->compile();
  }

  ///////////////////////////////////////////////////////////////////////////
  // Renderer wavefront mesh
  ///////////////////////////////////////////////////////////////////////////

  RnObj rnObjNew(RnContext ctx, const char *fileName) {
    PF_ASSERT(ctx && fileName);
    const FileName objName(fileName);
    size_t path = 0;
    Obj obj;
    for (path = 0; path < defaultPathNum; ++path) {
      if (obj.load(FileName(defaultPath[path]) + objName)) {
        PF_MSG_V("Obj: " << objName << " loaded from " << defaultPath[path]);
        break;
      }
    }
    if (path == defaultPathNum) {
      PF_WARNING_V("Obj: " << objName << " not found");
      return NULL;
    }
    RnObj renderObj = PF_NEW(RendererObj, *ctx, obj);
    renderObj->refInc();
    renderObj->externalRefInc();
    return renderObj;
  }
  void rnObjRetain(RnObj obj)  { RendererObjectRetain(obj); }
  void rnObjDelete(RnObj obj)  { RendererObjectDelete(obj); }
  void rnObjCompile(RnObj obj) { RendererObjectCompile(obj); }
  void rnObjProperties(RnObj obj, uint32 properties) {
    PF_ASSERT(obj && obj->isCompiled() == false);
    obj->properties = properties;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Renderer display list
  ///////////////////////////////////////////////////////////////////////////

  RnDisplayList rnDisplayListNew(RnContext ctx) {
    PF_ASSERT(ctx);
    RnDisplayList list = PF_NEW(RendererDisplayList, *ctx);
    RendererObjectNew(list);
    return list;
  }
  void rnDisplayListDelete(RnDisplayList list)  { RendererObjectDelete(list); }
  void rnDisplayListRetain(RnDisplayList list)  { RendererObjectRetain(list); }
  void rnDisplayListCompile(RnDisplayList list) { RendererObjectCompile(list);}
  void rnDisplayListAddObj(RnDisplayList list, RnObj obj, const float *modelView) {
    list->add(obj);
  }
  void rnDisplayListAddSet(RnDisplayList, RnSet, const float *modelView) {
    NOT_IMPLEMENTED;
  }

  ///////////////////////////////////////////////////////////////////////////
  // Renderer frame
  ///////////////////////////////////////////////////////////////////////////

  RnFrame rnFrameNew(RnContext ctx) {
    PF_ASSERT(ctx);
    RnFrame frame = PF_NEW(RendererFrame, *ctx);
    RendererObjectNew(frame);
    return frame;
  }
  void rnFrameDelete(RnFrame frame)  { RendererObjectDelete(frame); }
  void rnFrameRetain(RnFrame frame)  { RendererObjectRetain(frame); }
  void rnFrameCompile(RnFrame frame) { RendererObjectCompile(frame); }
  void rnFrameSetScreenDimension(RnFrame frame, uint32 w, uint32 h) {
    PF_ASSERT(frame);
    frame->setScreenDimension(w, h);
  }
  void rnFrameSetCamera(RnFrame frame,
                        const float *org,
                        const float *up,
                        const float *view,
                        float fov,
                        float ratio) {
    PF_ASSERT(frame);
    frame->setCamera(org, up, view, fov, ratio);
  }
  RnTask rnFrameDisplay(RnFrame frame) {
    PF_ASSERT(frame);
    return frame->display();
  }
  void rnFrameSetDisplayList(RnFrame frame, RnDisplayList list) {
    PF_ASSERT(frame);
    frame->setDisplayList(list);
  }

} /* namespace pf */

