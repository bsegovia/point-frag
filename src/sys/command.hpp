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

#ifndef __PF_COMMAND_HPP__
#define __PF_COMMAND_HPP__

#include "platform.hpp"
#include "vector.hpp"
#include "ref.hpp"

#include <string>

namespace pf
{

  /*! A CVar value either holds an integer, a float or a string value */
  struct CVar
  {
    /*! Register a integer CVar */
    CVar(const char *name, int32 min, int32 curr, int32 max, const char *desc = NULL);
    /*! Register a float CVar */
    CVar(const char *name, float min, float curr, float max, const char *desc = NULL);
    /*! Register a string CVar */
    CVar(const char *name, const std::string &str, const char *desc = NULL);
    /*! Describes the CVar value */
    enum Type
    {
      CVAR_FLOAT   = 0,
      CVAR_INT     = 1,
      CVAR_STRING  = 2,
      CVAR_INVALID = 0xffffffff
    };
    Type type;       //!< float, int or string
    std::string str; //!< Empty if not a string
    union {
      float f;
      int32_t i;
    };
    union {
      float fmin;
      int32_t imin;
    };
    union {
      float fmax;
      int32_t imax;
    };
    size_t index;
    const char *name;
    const char *desc;
  };

  /*! Hold all the console variables in one place. Because we can use frame
   *  overlapping in the game, we may have *several* values at the same time
   *  for one variable. We could use a master mutex or something like that but
   *  we really need the variable to stay unchanged over the frame (ie we get
   *  the new value at the beginning of the frame and then this value remains
   *  unchanged)
   */
  class CVarSystem : public RefCount
  {
  public:
    /*! Build a new CVar system */
    CVarSystem(void);
    /*! Release it */
    ~CVarSystem(void);
    /*! Get CVar at index "index" */
    CVar &get(size_t index);
    /*! Get CVar at index "index" */
    const CVar &get(size_t index) const;
    /*! The script will notify that one variable has been changed */
    INLINE void setModified(void)   { this->modified = true;  }
    /*! Once the script is processed, set the cvar system as unmodified */
    INLINE void setUnmodified(void) { this->modified = false; }
    /*! CVarSystem instance built at pre-main */
    static Ref<CVarSystem> global;
  private:
    friend struct CVar; //!< We can access fields from the CVar
    vector<CVar> var;   //!< All the variables
    bool modified;      //!< Modified means that a variable changed 
  };

} /* namespace pf */

/*! Any function that needs to be exported with the luaJIT FFI must be
 *  declared with this prefix
 */
#define PF_SCRIPT extern "C" PF_EXPORT_SYMBOL

/*! Declare a integer variable */
#define VARI(NAME, MIN, CURR, MAX, DESC)                      \
/* Build the CVar here. That appends it in the CVarSystem */  \
static const CVar cvar_##NAME(#NAME, MIN, CURR, MAX, DESC);   \
/* This is our accessor (read-only) */                        \
static INLINE int32 NAME(const CVarSystem &sys) {             \
  return sys.get(cvar_##NAME.index).i;                        \
}                                                             \
/* C function used by luaJIT when the variable is written */  \
PF_SCRIPT void cvarSet_##NAME(int32_t x) {                    \
  assert(CVarSystem::global);                                 \
  CVar &cvar = CVarSystem::global->get(cvar_##NAME.index);    \
  if (x >= cvar.imin && x <= cvar.imax)                       \
    cvar.i = x;                                               \
  CVarSystem::global->setModified();                          \
}                                                             \
/* C function used by luaJIT when the variable is read */     \
PF_SCRIPT int32_t cvarGet_##NAME() {                          \
  assert(CVarSystem::global);                                 \
  CVar &cvar = CVarSystem::global->get(cvar_##NAME.index);    \
  return cvar.i;                                              \
}

#endif /* __PF_COMMAND_HPP__ */

