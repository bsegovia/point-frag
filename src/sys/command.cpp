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

#include "command.hpp"

namespace pf
{
  Ref<CVarSystem> CVarSystem::global = NULL;

  CVarSystem::CVarSystem(void) : modified(false) {}
  CVarSystem::~CVarSystem(void) {}
  CVar &CVarSystem::get(size_t index) {
    PF_ASSERT(index < var.size());
    return var[index];
  }
  const CVar &CVarSystem::get(size_t index) const {
    PF_ASSERT(index < var.size());
    return var[index];
  }

  CVar::CVar(const char *name, int32 min, int32 curr, int32 max, const char *desc)
  {
    if (CVarSystem::global == false) CVarSystem::global = PF_NEW(CVarSystem);
    this->index = CVarSystem::global->var.size();
    this->type = CVAR_INT;
    this->i = curr;
    this->imin = min;
    this->imax = max;
    this->name = name;
    this->desc = desc;
    CVarSystem::global->var.push_back(*this);
    printf("DD");
  }

  CVar::CVar(const char *name, float min, float curr, float max, const char *desc)
  {
    if (CVarSystem::global == false) CVarSystem::global = PF_NEW(CVarSystem);
    this->index = CVarSystem::global->var.size();
    this->type = CVAR_FLOAT;
    this->f = curr;
    this->fmin = min;
    this->fmax = max;
    this->name = name;
    this->desc = desc;
    CVarSystem::global->var.push_back(*this);
  }

  CVar::CVar(const char *name, const std::string &str, const char *desc)
  {
    if (CVarSystem::global == false) CVarSystem::global = PF_NEW(CVarSystem);
    this->index = CVarSystem::global->var.size();
    this->type = CVAR_STRING;
    this->str = str;
    this->name = name;
    this->desc = desc;
    CVarSystem::global->var.push_back(*this);
  }

} /* namespace pf */

