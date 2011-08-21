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

#ifndef __PF_STACK_ITEM_H__
#define __PF_STACK_ITEM_H__

namespace pf
{
  /*! An item on the stack holds the node ID and distance of that node. */
  struct StackItem
  {
    /*! Copy operator */
    StackItem& operator=(const StackItem& other) { all = other.all; return *this; }

    /*! Sort a stack item by distance. */
    static INLINE void sort(StackItem& a, StackItem& b) { if (a.all < b.all) std::swap(a,b); }
    //static INLINE void sort(StackItem& a, StackItem& b) { if (a.dist < b.dist) std::swap(a,b); }

    union {
      struct { int32 ofs; float dist; };
      struct { int32 low, high;  };
      int64 all;
    };
  };

  /*! Sort 3 stack items. */
  INLINE void sort(StackItem& a, StackItem& b, StackItem& c)
  {
    int64 s1 = a.all;
    int64 s2 = b.all;
    int64 s3 = c.all;
    if (s2 < s1) std::swap(s2,s1);
    if (s3 < s2) std::swap(s3,s2);
    if (s2 < s1) std::swap(s2,s1);
    a.all = s1;
    b.all = s2;
    c.all = s3;
  }

  /*! Sort 4 stack items. */
  INLINE void sort(StackItem& a, StackItem& b, StackItem& c, StackItem& d)
  {
    int64 s1 = a.all;
    int64 s2 = b.all;
    int64 s3 = c.all;
    int64 s4 = d.all;
    if (s2 < s1) std::swap(s2,s1);
    if (s4 < s3) std::swap(s4,s3);
    if (s3 < s1) std::swap(s3,s1);
    if (s4 < s2) std::swap(s4,s2);
    if (s3 < s2) std::swap(s3,s2);
    a.all = s1;
    b.all = s2;
    c.all = s3;
    d.all = s4;
  }
}

#endif
