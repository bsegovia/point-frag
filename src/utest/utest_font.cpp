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

#include "renderer/font.hpp"
#include "sys/default_path.hpp"

#include <string>

using namespace pf;
static const std::string fontName = "font.fnt";

int main(int argc, char *argv[])
{
  Font font;
  size_t i = 0;
  for (; i < defaultPathNum; ++i) {
    const FileName path(std::string(defaultPath[i]) + fontName);
    if (font.load(path)) break;
  }
  PF_ASSERT(i < defaultPathNum);
}

