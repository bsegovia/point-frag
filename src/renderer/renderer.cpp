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

#include "renderer.hpp"
#include "renderer_driver.hpp"
#include "texture.hpp"

namespace pf
{
  Renderer::Renderer(void)
  {
    this->driver = PF_NEW(RendererDriver);
    this->streamer = PF_NEW(TextureStreamer, *this);
    const TextureRequest req("Maps/chess.tga", PF_TEX_FORMAT_PLAIN, GL_NEAREST, GL_NEAREST);
    Ref<Task> loadingTask = this->streamer->createLoadTask(req);
    loadingTask->scheduled();
    loadingTask->waitForCompletion();
    const TextureState state = this->streamer->getTextureState("Maps/chess.tga");
    this->defaultTex = state.tex;
    FATAL_IF (defaultTex == false  || defaultTex->isValid() == false,
              "Default \"chess.tga\" texture not found");
  }

  Renderer::~Renderer(void)
  {
    this->defaultTex = NULL;
    PF_DELETE(this->streamer);
    PF_DELETE(this->driver);
  }
} /* namespace pf */

