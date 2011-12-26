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

#ifndef __PF_RENDERER_OBJECT_HPP__
#define __PF_RENDERER_OBJECT_HPP__

#include "sys/platform.hpp" 
#include "sys/ref.hpp"

namespace pf
{
  class Renderer; // Own all renderer objects

  /*! Base class for renderer side objects */
  class RendererObject : public RefCount
  {
  public:
    RendererObject(Renderer &renderer);
    INLINE void externalRefInc(void) { this->externalRef++; }
    INLINE void externalRefDec(void) {
      if (!(--this->externalRef))
        this->onUnreferenced();
    }
    /*! Compiling an object makes it immutable */
    INLINE void compile(void) { this->onCompile(); compiled = true; }
    INLINE bool isCompiled(void) const { return compiled; }
    /*! When there is no more reference externally (ie the user does not hold
     *  any reference on it), we can trigger any specific code
     */
    virtual void onUnreferenced(void) = 0;
    Renderer &renderer;
  protected:
    /*! Override it in the inherited classes */
    virtual void onCompile(void) = 0;
  private:
    Atomic externalRef;//!< Number of references held by the user
    bool compiled;     //!< compiled == immutable
  };

} /* namespace pf */

#endif /* __PF_RENDERER_OBJECT_HPP__ */

