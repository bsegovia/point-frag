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

/*! This file implement a command / console variable system for point frag.
 *  With LuaJIT, this task is rather easy. LuaJIT with its FFI is indeed able
 *  to parse the binary and to extract the symbols. The only thing we need to
 *  do is to declare the prototype of the functions and variable for LuaJIT
 *
 *  To do that job, we simply use some C++ pre-main stuff. Global variables are
 *  declared for both commands and variables. Their constructors registers
 *  what we need in global variable. When the command system is initialized,
 *  we upload all the information to LuaJIT. Pretty straightforward!
 */
namespace pf
{
  // Need to start the command system
  class ScriptSystem;

  /*! A ConVar value either holds an integer, a float or a string value */
  class ConVar
  {
  public:
    /*! Register a integer ConVar */
    ConVar(const char *name, int32 min, int32 curr, int32 max, const char *desc = NULL);
    /*! Register a float ConVar */
    ConVar(const char *name, float min, float curr, float max, const char *desc = NULL);
    /*! Register a string ConVar */
    ConVar(const char *name, const char *str, const char *desc = NULL);
    /*! Release allocated strings if required */
    ~ConVar(void);
    void set(const char *str); //<! Set the value (must be a string)
    void set(double x);        //<! Set the value (must be a float or an integer)
    /*! Describes the ConVar value */
    enum Type
    {
      CVAR_FLOAT   = 0,
      CVAR_INT     = 1,
      CVAR_STRING  = 2,
      CVAR_INVALID = 0xffffffff
    };
    Type type;        //!< float, int or string
    size_t index;     //!< Index of the cvar in the ConVarSystem
    const char *name; //!< Name of the cvar
    const char *desc; //!< Optional string description
    union {
      char *str;
      float f;
      int32 i;
    };
    union {
      float fmin;
      int32 imin;
    };
    union {
      float fmax;
      int32 imax;
    };
  };

  /*! Hold all the console variables in one place */
  class ConVarSystem : public RefCount
  {
  public:
    /*! Build a new ConVar system */
    ConVarSystem(void);
    /*! Release it */
    ~ConVarSystem(void);
    /*! Get ConVar at index "index" */
    ConVar &get(size_t index);
    /*! Get ConVar at index "index" */
    const ConVar &get(size_t index) const;
    /*! ConVarSystem instance built at pre-main */
    static ConVarSystem *global;
  private:
    friend struct ConVar;    //!< We can access fields from the ConVar
    std::vector<ConVar> var; //!< All the variables (use std::vector since this is pre-main)
    bool modified;           //!< Modified means that a variable changed 
  };

  /*! With LuaJIT, this is going to be super simple. LuaJIT is able to get a
   *  symbol from a executable and DLL. We just need to provide the function
   *  prototype. We take this idea from Cube 2 and we just use a string for
   *  that. This structure just makes the book keeping when a macro declares
   *  the exported function
   */
  struct ConCommand
  {
    ConCommand(const char *name, const char *argument, char ret = 0);
    /*! Just the function name */
    const char *name;
    /*! Command arguments. For example "if", means a function with two
     *  arguments, the first is an integer and the other is a float
     */
    const char *argument;
    /*! Stores the commands declared in the code (use std since pre-main) */
    static std::vector<ConCommand> *cmds;
    /*! Command returned type. ret == 0 means no value is returned */
    char ret;
  };

  /*! Initialize the command system. This includes the console variables which
   *  are exported to the LuaJIT state
   */
  void CommandSystemStart(ScriptSystem &scriptSystem);

  /*! Release all the resources (including the global ConVarSystem) */
  void CommandSystemEnd(void);

} /* namespace pf */

/*! Any function that needs to be exported with the luaJIT FFI must be
 *  declared with this prefix
 */
#define PF_SCRIPT extern "C" PF_EXPORT_SYMBOL

/*! Declare a command by its name, its arguments and returned value types */
#define COMMAND(NAME, ARGS, RET)                                             \
  static const ConCommand ccom_##NAME(#NAME, ARGS, RET);

/*! Create the functions to interface with both C++ and Lua codes */
#define _VAR(NAME,TYPE,FIELD,ARGS,RET)                                       \
  /* C function used by lua when the variable is written */                  \
  PF_SCRIPT void cvarSet_##NAME(TYPE x) {                                    \
    PF_ASSERT(ConVarSystem::global);                                         \
    ConVar &cvar = ConVarSystem::global->get(cvar_##NAME.index);             \
    cvar.set(x);                                                             \
  }                                                                          \
  /* C function used by lua when the variable is read */                     \
  PF_SCRIPT TYPE cvarGet_##NAME(void) {                                      \
    PF_ASSERT(ConVarSystem::global);                                         \
    ConVar &cvar = ConVarSystem::global->get(cvar_##NAME.index);             \
    return cvar.FIELD;                                                       \
  }                                                                          \
  /* Indicate if this is a string */                                         \
  PF_SCRIPT int32 cvarIsString_##NAME(void) {                                \
    return RET == 's' ? 1 : 0;                                               \
  }                                                                          \
  /* Export both functions to Lua */                                         \
  COMMAND(cvarSet_##NAME, ARGS, 0)                                           \
  COMMAND(cvarGet_##NAME, "", RET)                                           \
  COMMAND(cvarIsString_##NAME, "", 'i')                                      \
  /* This is our accessor (read-only) */                                     \
  static INLINE TYPE NAME(void) { return cvarGet_##NAME(); }                 \

/*! Declare an integer variable */
#define VARI(NAME, MIN, CURR, MAX, DESC)                                     \
  static const ConVar cvar_##NAME(#NAME, MIN, CURR, MAX, DESC);              \
  _VAR(NAME, int32, i, "i", 'i')

/*! Declare a float variable */
#define VARF(NAME, MIN, CURR, MAX, DESC)                                     \
  static const ConVar cvar_##NAME(#NAME, MIN, CURR, MAX, DESC);              \
  _VAR(NAME, float, f, "f", 'f')

/*! Declare a string variable */
#define VARS(NAME, CURR, DESC)                                               \
  static const ConVar cvar_##NAME(#NAME, CURR, DESC);                        \
  _VAR(NAME, const char*, str, "s", 's');

#endif /* __PF_COMMAND_HPP__ */

