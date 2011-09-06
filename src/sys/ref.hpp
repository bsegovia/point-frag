// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
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

#ifndef __PF_REF_HPP__
#define __PF_REF_HPP__

#include "sys/atomic.hpp"
#include "sys/alloc.hpp"

namespace pf
{
  class RefCount
  {
  template<typename Type> friend class Ref;
  public:
    RefCount() : refCounter(0) {}
    virtual ~RefCount() {}
  private:
    INLINE void refInc() { refCounter++; }
    INLINE bool refDec() { return !(--refCounter); }
    Atomic refCounter;
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Reference to single object
  ////////////////////////////////////////////////////////////////////////////////

  template<typename Type>
  class Ref {
  public:
    Type* const ptr;

    ////////////////////////////////////////////////////////////////////////////////
    /// Constructors, Assignment & Cast Operators
    ////////////////////////////////////////////////////////////////////////////////

    INLINE Ref(void) : ptr(NULL) {}
    INLINE Ref(NullTy) : ptr(NULL) {}
    INLINE Ref(const Ref& input) : ptr(input.ptr) { if ( ptr ) ptr->refInc(); }
    INLINE Ref(Type* const input) : ptr(input) { if (ptr) ptr->refInc(); }
    INLINE ~Ref(void) { if (ptr && ptr->refDec()) DELETE(ptr); }

    INLINE Ref& operator= (const Ref &input) {
      if ( input.ptr ) input.ptr->refInc();
      if ( ptr && ptr->refDec() ) DELETE(ptr);
      *(Type**)&ptr = input.ptr;
      return *this;
    }

    INLINE Ref& operator= (NullTy) {
      if (ptr && ptr->refDec()) DELETE(ptr);
      *(Type**)&ptr = NULL;
      return *this;
    }

    INLINE operator bool(void) const { return ptr != NULL; }

    ////////////////////////////////////////////////////////////////////////////////
    /// Properties
    ////////////////////////////////////////////////////////////////////////////////

    INLINE const Type& operator  *( void ) const { return *ptr; }
    INLINE       Type& operator  *( void )       { return *ptr; }
    INLINE const Type* operator ->( void ) const { return  ptr; }
    INLINE       Type* operator ->( void )       { return  ptr; }

    template<typename TypeOut>
    INLINE       Ref<TypeOut> cast()       { return Ref<TypeOut>(static_cast<TypeOut*>(ptr)); }
    template<typename TypeOut>
    INLINE const Ref<TypeOut> cast() const { return Ref<TypeOut>(static_cast<TypeOut*>(ptr)); }
  };

  template<typename Type> INLINE  bool operator< ( const Ref<Type>& a, const Ref<Type>& b ) { return a.ptr <  b.ptr ; }

  template<typename Type> INLINE  bool operator== ( const Ref<Type>& a, NullTy             ) { return a.ptr == NULL  ; }
  template<typename Type> INLINE  bool operator== ( NullTy            , const Ref<Type>& b ) { return NULL  == b.ptr ; }
  template<typename Type> INLINE  bool operator== ( const Ref<Type>& a, const Ref<Type>& b ) { return a.ptr == b.ptr ; }

  template<typename Type> INLINE  bool operator!= ( const Ref<Type>& a, NullTy             ) { return a.ptr != NULL  ; }
  template<typename Type> INLINE  bool operator!= ( NullTy            , const Ref<Type>& b ) { return NULL  != b.ptr ; }
  template<typename Type> INLINE  bool operator!= ( const Ref<Type>& a, const Ref<Type>& b ) { return a.ptr != b.ptr ; }
}

#endif

