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
  struct ConVar
  {
    /*! Register a integer ConVar */
    ConVar(const char *name, int32 min, int32 curr, int32 max, const char *desc = NULL);
    /*! Register a float ConVar */
    ConVar(const char *name, float min, float curr, float max, const char *desc = NULL);
    /*! Register a string ConVar */
    ConVar(const char *name, const std::string &str, const char *desc = NULL);
    /*! Describes the ConVar value */
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
  class ConVarSystem : public RefCount
  {
  public:
    /*! Build a new ConVar system */
    ConVarSystem(void);
    /*! Release it */
    ~ConVarSystem(void);
    /*! Duplicate the consoble variable system */
    ConVarSystem *clone(void) const;
    /*! Get ConVar at index "index" */
    ConVar &get(size_t index);
    /*! Get ConVar at index "index" */
    const ConVar &get(size_t index) const;
    /*! Says if a change happened in one of the variables */
    INLINE bool isModified(void) const { return this->modified; }
    /*! The script will notify that one variable has been changed */
    INLINE void setModified(void)      { this->modified = true;  }
    /*! Once the script is processed, set the cvar system as unmodified */
    INLINE void setUnmodified(void)    { this->modified = false; }
    /*! ConVarSystem instance built at pre-main */
    static ConVarSystem *global;
  private:
    friend struct ConVar; //!< We can access fields from the ConVar
    vector<ConVar> var;   //!< All the variables
    bool modified;      //!< Modified means that a variable changed 
  };

  /*! With LuaJIT, this is going to be super simple. LuaJIT is able to get a
   * symbol from a executable and DLL. We just need to provide the function
   * prototype. We take this idea from Cube 2 and we just use a string for
   * that. This structure just makes the book keeping when a macro declares
   * the exported function
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
    /* Command returned type. ret == 0 means no value is returned */
    char ret;
    /*! Stores all the commands declared in the code */
    static vector<ConCommand> *cmds;
  };

  /*! Initialize the console system. This includes the console variables which
   *  are exported to the LuaJIT state
   */
  void ConsoleSystemStart(ScriptSystem &scriptSystem);

  /*! Release all the resources (including the global ConVarSystem) */
  void ConsoleSystemEnd(void);

} /* namespace pf */

/*! Any function that needs to be exported with the luaJIT FFI must be
 *  declared with this prefix
 */
#define PF_SCRIPT extern "C" PF_EXPORT_SYMBOL

/* Declare a command by its name, its arguments and returned value types */
#define COMMAND(NAME, ARGS, RET)                              \
static const ConCommand ccom_##NAME(#NAME, ARGS, RET);

/*! Declare a variable (integer or float) */
#define _VAR(NAME,MIN,CURR,MAX,DESC,TYPE,FIELD,STR,CHAR)      \
/* Build the ConVar here. That appends it in ConVarSystem */  \
static const ConVar cvar_##NAME(#NAME, MIN, CURR, MAX, DESC); \
/* This is our accessor (read-only) */                        \
static INLINE TYPE NAME(const ConVarSystem *sys) {            \
  PF_ASSERT(sys != ConVarSystem::global);                     \
  return sys->get(cvar_##NAME.index).FIELD;                   \
}                                                             \
/* C function used by luaJIT when the variable is written */  \
PF_SCRIPT void cvarSet_##NAME(TYPE x) {                       \
  assert(ConVarSystem::global);                               \
  ConVar &cvar = ConVarSystem::global->get(cvar_##NAME.index);\
  if (x >= cvar.FIELD##min && x <= cvar.FIELD##max)           \
    cvar.FIELD = x;                                           \
  ConVarSystem::global->setModified();                        \
}                                                             \
/* C function used by luaJIT when the variable is read */     \
PF_SCRIPT TYPE cvarGet_##NAME() {                             \
  assert(ConVarSystem::global);                               \
  ConVar &cvar = ConVarSystem::global->get(cvar_##NAME.index);\
  return cvar.FIELD;                                          \
}                                                             \
/* Export both functions to LuaJIT */                         \
COMMAND(cvarSet_##NAME, STR, 0)                               \
COMMAND(cvarGet_##NAME, "", CHAR)

/*! Declare a integer variable */
#define VARI(NAME, MIN, CURR, MAX, DESC)                      \
  _VAR(NAME, MIN, CURR, MAX, DESC, int32, i, "i", 'i')

/*! Declare a float variable */
#define VARF(NAME, MIN, CURR, MAX, DESC)                      \
  _VAR(NAME, MIN, CURR, MAX, DESC, float, f, "f", 'f')

#endif /* __PF_COMMAND_HPP__ */

