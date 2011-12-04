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

#include "lua.hpp"
#include "script.hpp"
#include "string.hpp"
#include "default_path.hpp"

namespace pf
{
  /*! Implement the script system with LuaJIT */
  class LuaScriptSystem : public ScriptSystem
  {
  public:
    LuaScriptSystem(void);
    virtual ~LuaScriptSystem(void);
    /*! Implements the interface */
    virtual void run(const char *str, ScriptStatus &status);
    /*! Implements the interface */
    virtual void runFile(const char *path, ScriptStatus &status);
  private:
    /*! Report the error after the execution */
    bool report(int ret, ScriptStatus &status);
    /*! Complete lua state is stored here */
    lua_State *L;
  };

  ScriptSystem::~ScriptSystem(void) {}

  // Code mostly taken from luajit interpreter
  LuaScriptSystem::LuaScriptSystem(void)
  {
    L = lua_open();    // create lua state
    FATAL_IF (L == NULL, "unable to create the lua state");
    lua_gc(L, LUA_GCSTOP, 0);     // stop collector during initialization
    luaL_openlibs(L);             // open libraries
    lua_gc(L, LUA_GCRESTART, -1);

    // Look for the initialization script that build the pf table
    const FileName luainit("lua/init.lua");
    size_t i = 0;
    for (; i < defaultPathNum; ++i) {
      const FileName dataPath(defaultPath[i]);
      const FileName path = dataPath + luainit;
      const std::string initsrc = loadFile(path);
      if (initsrc.size() == 0)
        continue;
      else {
        printf(initsrc.c_str());
        ScriptStatus status;
        this->report(luaL_loadstring(L, initsrc.c_str()), status);
        if (status.success == false)
          FATAL(std::string("Lua initialization failed: ") + status.msg);
        this->report(lua_pcall(L,0,0,0), status);
        if (status.success == false)
          FATAL(std::string("Lua initialization failed: ") + status.msg);
        break;
      }
    }
    FATAL_IF(i==defaultPathNum, "Unable to find Lua initialization script");
  }

  LuaScriptSystem::~LuaScriptSystem(void) { lua_close(L); }

  bool LuaScriptSystem::report(int ret, ScriptStatus &status)
  {
    if (ret && !lua_isnil(L, -1)) {
      const char *msg = lua_tostring(L, -1);
      if (msg == NULL) msg = "(error object is not a string)";
      lua_pop(L, 1);
      status.msg = msg;
      status.success = false;
      return false;
    } else {
      status.success = true;
      return true;
    }
  }

  void LuaScriptSystem::run(const char *str, ScriptStatus &status)
  {
    if (str == NULL) {
      status.success = false;
      status.msg = "NULL string";
    } else {
      if (!this->report(luaL_loadstring(L, str), status))
        return;
      lua_getfield(L, LUA_GLOBALSINDEX, "pf");
      IF_DEBUG(int ret =) lua_setfenv(L, -2);
      assert(ret == 1);
      this->report(lua_pcall(L,0,0,0), status);
    }
  }

  void LuaScriptSystem::runFile(const char *path, ScriptStatus &status) 
  {
    const std::string source = loadFile(path);
    if (source.size() == 0) {
      status.success = false;
      status.msg = "Unable to open " + std::string(path);
    } else
      this->run(source.c_str(), status);
  }

  ScriptSystem *LuaScriptSystemCreate(void) { return PF_NEW(LuaScriptSystem); }

} /* namespace pf */

