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

#ifndef __PF_RENDERER_FRAME_HPP__
#define __PF_RENDERER_FRAME_HPP__

#include "renderer_object.hpp"
#include "renderer_display_list.hpp"
#include "math/vec.hpp"
#include "sys/platform.hpp"

namespace pf
{
  class Renderer;            // Owns this class
  class RendererDisplayList; // A set of display list makes a frame

  /*! A frame defines how to render an image */
  class RendererFrame : public RendererObject
  {
  public:
    RendererFrame(Renderer &renderer);
    ~RendererFrame(void);
    /*! Return the rendering task */
    Task *display(void);
    /*! This will be the used display list (TODO supports more that one list) */
    INLINE void setDisplayList(RendererDisplayList *list_) {
      PF_ASSERT(this->isCompiled() == false);
      this->list = list_;
    }
    /*! Set the screen dimensions */
    INLINE void setScreenDimension(uint32 w_, uint32 h_) {
      PF_ASSERT(this->isCompiled() == false);
      this->w = w_;
      this->h = h_;
    }
    /*! Set the camera */
    INLINE void setCamera(const float *org,
                          const float *up,
                          const float *view,
                          float fov,
                          float ratio)
    {
      if (view) {
        this->view.x = view[0];
        this->view.y = view[1];
        this->view.z = view[2];
      }
      if (up) {
        this->up.x = up[0];
        this->up.y = up[1];
        this->up.z = up[2];
      }
      if (org) {
        this->org.x = org[0];
        this->org.y = org[1];
        this->org.z = org[2];
      }
      this->fov   = fov;
      this->ratio = ratio;
    }
  private:
    virtual void onCompile(void);     //!< Implements base class
    virtual void onUnreferenced(void);//!< Implements base class
    vec3f org;                        //!< Camera origin
    vec3f up;                         //!< Camera up vector
    vec3f view;                       //!< Camera view vector
    float fov, ratio;                 //!< Fov and aspect ratio
    uint32 w, h;                      //!< Screen dimensions
    Ref<RendererDisplayList> list;    //!< Objects to display
  };

} /* namespace pf */

#endif /* __PF_RENDERER_FRAME_HPP__ */

