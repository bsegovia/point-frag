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

#ifndef __PF_RENDERER_CONTEXT_HPP__
#define __PF_RENDERER_CONTEXT_HPP__

#include "sys/platform.hpp"

namespace pf
{
  /*! A context is a handle to a renderer */
  typedef class Renderer *RnContext;
  /*! Handle to a wavefront OBJ */
  typedef class RendererObj *RnObj;
  /*! A static set contains immutable instance of static objects */
  typedef class RendererSet *RnSet;
  /*! A display list contains objects to render. It is thread safe */
  typedef class RendererDisplayList *RnDisplayList;
  /*! A frame defines how to render an image */
  typedef class RendererFrame *RnFrame;
  /*! A renderer task is just a regular task */
  typedef class Task *RnTask;

  ///////////////////////////////////////////////////////////////////////////
  /// Renderer context is the complete rendering interface
  ///////////////////////////////////////////////////////////////////////////

  /*! Create a renderer context */
  RnContext rnContextNew(void);
  /*! Remove a reference on the context. Delete it if unreferenced */
  void rnContextDelete(RnContext);
  /*! Add a reference on the context */
  void rnContextRetain(RnContext);

  ///////////////////////////////////////////////////////////////////////////
  /// A frame defines how to render the image
  ///////////////////////////////////////////////////////////////////////////

  /*! Create a new frame to render */
  RnFrame rnFrameNew(RnContext);
  /*! Remove a reference on the frame. Delete it if unreferenced */
  void rnFrameDelete(RnFrame);
  /*! Add a reference on the frame */
  void rnFrameRetain(RnFrame);
  /*! Set the camera for the frame */
  void rnFrameSetCamera(RnFrame frame,
                        const float *org,
                        const float *up,
                        const float *view,
                        float fov,
                        float ratio);
  /*! Set screen dimension for this frame */
  void rnFrameSetScreenDimension(RnFrame, uint32 w, uint32 h);
  /*! TODO do something better later (with several display lists) */
  void rnFrameSetDisplayList(RnFrame, RnDisplayList);
  /*! Make it immutable */
  void rnFrameCompile(RnFrame);
  /*! Display the frame. Return an *unscheduled* task */
  RnTask rnFrameDisplay(RnFrame);

  ///////////////////////////////////////////////////////////////////////////
  /// Renderer (immutable) set contains a set of objects glued together
  ///////////////////////////////////////////////////////////////////////////

  /*! Create and destroy a immutable set */
  RnSet rnSetNew(RnContext);
  /*! Destroy a immutable set */
  void rnSetDelete(RnSet);
  /*! Append a object to the immutable set. modelView == NULL means identity */
  void rnSetAddObj(RnSet, RnObj, const float *modelView = NULL);
  /*! Will be an occluder made of the union of all contained occluders */
  void rnSetProperties(uint32 properties);
  /*! Once compiled, a set cannot be modified anymore */
  void rnSetCompile(RnSet);

  ///////////////////////////////////////////////////////////////////////////
  /// Renderer OBJ. Directly maps a Wavefront OBJ
  ///////////////////////////////////////////////////////////////////////////

  enum {
    RN_OBJ_OCCLUDER        = 1 << 0, //!< Has its own BVH
    RN_OBJ_SHARED_OCCLUDER = 1 << 1, //!< Included in a Set BVH
  };

  /*! Create a renderer OBJ. Returns NULL if object not found */
  RnObj rnObjNew(RnContext, const char *fileName);
  /*! Remove a reference on a renderer OBJ. Delete it if unreferenced */
  void rnObjDelete(RnObj);
  /*! Add a reference on the OBJ */
  void rnObjRetain(RnObj);
  /*! Will be an occluder (has a BVH) */
  void rnObjProperties(RnObj, uint32 properties);
  /*! Once compiled the obj becomes immutable */
  void rnObjCompile(RnObj);

  ///////////////////////////////////////////////////////////////////////////
  /// Renderer display list. Contains objects to render. It is thread safe
  ///////////////////////////////////////////////////////////////////////////

  /*! Create a display list */
  RnDisplayList rnDisplayListNew(RnContext);
  /*! Remove a reference on a display list. Delete it if unreferenced */
  void rnDisplayListDelete(RnDisplayList);
  /*! Add an extra reference on a display list */
  void rnDisplayListRetain(RnDisplayList);
  /*! Push a renderer obj to display. modelView == NULL means identity */
  void rnDisplayListAddObj(RnDisplayList, RnObj, const float *modelView = NULL);
  /*! Push a immutable set to display. modelView == NULL means identity */
  void rnDisplayListAddSet(RnDisplayList, RnSet, const float *modelView = NULL);
  /*! Compiling it makes it immutable */
  void rnDisplayListCompile(RnDisplayList);

} /* namespace pf */

#endif /* __PF_RENDERER_CONTEXT_HPP__ */

