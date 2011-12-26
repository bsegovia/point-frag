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

#ifndef __PF_RENDERER_HPP__
#define __PF_RENDERER_HPP__

#include "renderer/texture.hpp"
#include "sys/tasking_utility.hpp"

namespace pf
{
  class RendererObj;
  class RendererDriver;
  class TextureStreamer;

  /*! Renderer. This is the real interface for all other game component. It
   *  contains all the graphics objects, performs the culling, manage the
   *  occlusion queuries ...
   */
  class Renderer : public NonCopyable, public RefCount
  {
  public:
    Renderer(void);
    ~Renderer(void);

    /*! Get the texture or possibly a task loading it */
    INLINE TextureState getTexture(const char *name) {
      return this->streamer->getTextureState(name);
    }
    /*! Current rendering task (TODO make a pipeline) */
    Ref<TaskInOut> renderingTask;
    /*! Default texture */
    Ref<Texture2D> defaultTex;
    /*! Low-level interface to the graphics API */
    RendererDriver *driver;
    /*! Handles and stores textures */
    TextureStreamer *streamer;
    PF_CLASS(Renderer);
  };

} /* namespace pf*/
#endif /* __PF_RENDERER_HPP__ */
