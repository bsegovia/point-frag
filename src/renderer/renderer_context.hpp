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

#include "sys/command.hpp"

namespace pf
{
  class RendererContext;
  class RendererObj;
  class RendererSet;
  class RendererDispayList;
} /* namespace pf */

/*! A context is a handle to a renderer */
typedef class pf::RendererContext *RContext;
/*! Handle to a wavefront OBJ */
typedef class pf::RendererObj *RObj;
/*! A static set contains immutable instance of static objects */
typedef class pf::RendererSet *RSet;
/*! A display list contains objects to render. It is thread safe */
typedef class pf::RendererDispayList *RDisplayList;

///////////////////////////////////////////////////////////////////////////
/// Renderer context
///////////////////////////////////////////////////////////////////////////

/*! Create and destroy a renderer context */
PF_SCRIPT RContext rnCreateContext(void);
PF_SCRIPT void rnDestroContext(RContext);

/*! Create and destroy a renderer OBJ. Returns NULL if object not found */
PF_SCRIPT RObj rnContextCreateObj(RContext, const char *fileName);
PF_SCRIPT void rnContextDestroyObj(RObj);

/*! Create and destroy a immutable set */
PF_SCRIPT RSet rnContextCreateSet(RContext);
PF_SCRIPT void rnContextDestroySet(RSet);

/*! Create and destroy a display list */
PF_SCRIPT RDisplayList rnContextCreateList(RContext);
PF_SCRIPT void rnContextDestroyList(RDisplayList);

///////////////////////////////////////////////////////////////////////////
/// Renderer display list
///////////////////////////////////////////////////////////////////////////

/*! Push a renderer obj to display */
PF_SCRIPT void rnDisplayListPushObj(RDisplayList, RObj, const float *modelView);
/*! Push a immutable set to display */
PF_SCRIPT void rnDisplayListPushSet(RDisplayList, RSet, const float *modelView);

///////////////////////////////////////////////////////////////////////////
/// Renderer (immutable) set
///////////////////////////////////////////////////////////////////////////

/*! Append a object to the immutable set */
PF_SCRIPT void rnSetAppendObj(RSet, RObj, const float *modelView);
/*! Once compiled, a set cannot be modified anymore */
PF_SCRIPT void rnSetCompile(RSet);

#endif /* __PF_RENDERER_CONTEXT_HPP__ */

