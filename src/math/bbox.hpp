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

#ifndef __PF_BBOX_HPP__
#define __PF_BBOX_HPP__

#include "vec.hpp"

namespace pf
{
  template<typename T>
  struct BBox
  {
    T lower, upper;

    INLINE BBox() { }
    INLINE BBox(const BBox& other) { lower = other.lower; upper = other.upper; }
    INLINE BBox& operator= (const BBox &other) { lower = other.lower; upper = other.upper; return *this; }
    INLINE BBox (const T& v) : lower(v),   upper(v) {}
    INLINE BBox (const T& lower, const T& upper) : lower(lower), upper(upper) {}
    INLINE void grow(const BBox& other) { lower = min(lower,other.lower); upper = max(upper,other.upper); }
    INLINE void grow(const T   & other) { lower = min(lower,other); upper = max(upper,other); }
    INLINE BBox(EmptyTy) : lower(pos_inf), upper(neg_inf) {}
    INLINE BBox(FullTy)  : lower(neg_inf), upper(pos_inf) {}
    INLINE BBox(FalseTy) : lower(pos_inf), upper(neg_inf) {}
    INLINE BBox(TrueTy)  : lower(neg_inf), upper(pos_inf) {}
    INLINE BBox(NegInfTy): lower(pos_inf), upper(neg_inf) {}
    INLINE BBox(PosInfTy): lower(neg_inf), upper(pos_inf) {}
  };

  /*! tests if box is empty */
  template<typename T> INLINE bool isEmpty(const BBox<T>& box) { for (int i=0; i<T::N; i++) if (box.lower[i] > box.upper[i]) return true; return false; }

  /*! computes the center of the box */
  template<typename T> INLINE T center (const BBox<T>& box) { return T(.5f)*(box.lower + box.upper); }
  template<typename T> INLINE T center2(const BBox<T>& box) { return box.lower + box.upper; }

  /*! computes the size of the box */
  template<typename T> INLINE T size(const BBox<T>& box) { return box.upper - box.lower; }

  /*! computes the volume of a bounding box */
  template<typename T> INLINE T volume(const BBox<T>& b) { return reduce_mul(size(b)); }

  /*! computes the surface area of a bounding box */
  template<typename T> INLINE T area(const BBox<vec2<T> >& b) { const vec2<T> d = size(b); return d.x*d.y; }
  template<typename T> INLINE T area(const BBox<vec3<T> >& b) { const vec3<T> d = size(b); return T(2)*(d.x*(d.y+d.z)+d.y*d.z); }

  /*! merges bounding boxes and points */
  template<typename T> INLINE const BBox<T> merge(const BBox<T>& a, const       T& b) { return BBox<T>(min(a.lower, b), max(a.upper, b)); }
  template<typename T> INLINE const BBox<T> merge(const       T& a, const BBox<T>& b) { return BBox<T>(min(a    , b.lower), max(a    , b.upper)); }
  template<typename T> INLINE const BBox<T> merge(const BBox<T>& a, const BBox<T>& b) { return BBox<T>(min(a.lower, b.lower), max(a.upper, b.upper)); }
  template<typename T> INLINE const BBox<T> merge(const BBox<T>& a, const BBox<T>& b, const BBox<T>& c) { return merge(a,merge(b,c)); }
  template<typename T> INLINE const BBox<T>& operator+=(BBox<T>& a, const BBox<T>& b) { return a = merge(a,b); }
  template<typename T> INLINE const BBox<T>& operator+=(BBox<T>& a, const       T& b) { return a = merge(a,b); }

  /*! Merges four boxes. */
  template<typename T> INLINE BBox<T> merge(const BBox<T>& a, const BBox<T>& b, const BBox<T>& c, const BBox<T>& d) {
    return merge(merge(a,b),merge(c,d));
  }

  /*! Merges eight boxes. */
  template<typename T> INLINE BBox<T> merge(const BBox<T>& a, const BBox<T>& b, const BBox<T>& c, const BBox<T>& d,
                                            const BBox<T>& e, const BBox<T>& f, const BBox<T>& g, const BBox<T>& h) {
    return merge(merge(a,b,c,d),merge(e,f,g,h));
  }

  /*! Comparison Operators */
  template<typename T> INLINE bool operator==(const BBox<T>& a, const BBox<T>& b) { return a.lower == b.lower && a.upper == b.upper; }
  template<typename T> INLINE bool operator!=(const BBox<T>& a, const BBox<T>& b) { return a.lower != b.lower || a.upper != b.upper; }

  /*! intersect bounding boxes */
  template<typename T> INLINE const BBox<T> intersect(const BBox<T>& a, const BBox<T>& b) { return BBox<T>(max(a.lower, b.lower), min(a.upper, b.upper)); }
  template<typename T> INLINE const BBox<T> intersect(const BBox<T>& a, const BBox<T>& b, const BBox<T>& c) { return intersect(a,intersect(b,c)); }

  /*! tests if bounding boxes (and points) are disjoint (empty intersection) */
  template<typename T> INLINE bool disjoint(const BBox<T>& a, const BBox<T>& b)
  { const T d = min(a.upper, b.upper) - max(a.lower, b.lower); for (index_t i = 0 ; i < T::N ; i++) if (d[i] < typename T::Scalar(zero)) return true; return false; }
  template<typename T> INLINE bool disjoint(const BBox<T>& a, const  T& b)
  { const T d = min(a.upper, b)     - max(a.lower, b);     for (index_t i = 0 ; i < T::N ; i++) if (d[i] < typename T::Scalar(zero)) return true; return false; }
  template<typename T> INLINE bool disjoint(const  T& a, const BBox<T>& b)
  { const T d = min(a, b.upper)     - max(a, b.lower);     for (index_t i = 0 ; i < T::N ; i++) if (d[i] < typename T::Scalar(zero)) return true; return false; }

  /*! tests if bounding boxes (and points) are conjoint (non-empty intersection) */
  template<typename T> INLINE bool conjoint(const BBox<T>& a, const BBox<T>& b)
  { const T d = min(a.upper, b.upper) - max(a.lower, b.lower); for (index_t i = 0 ; i < T::N ; i++) if (d[i] < typename T::Scalar(zero)) return false; return true; }
  template<typename T> INLINE bool conjoint(const BBox<T>& a, const  T& b)
  { const T d = min(a.upper, b)     - max(a.lower, b);     for (index_t i = 0 ; i < T::N ; i++) if (d[i] < typename T::Scalar(zero)) return false; return true; }
  template<typename T> INLINE bool conjoint(const  T& a, const BBox<T>& b)
  { const T d = min(a, b.upper)     - max(a, b.lower);     for (index_t i = 0 ; i < T::N ; i++) if (d[i] < typename T::Scalar(zero)) return false; return true; }

  /*! output operator */
  template<typename T> INLINE std::ostream& operator<<(std::ostream& cout, const BBox<T>& box) {
    return cout << "[" << box.lower << "; " << box.upper << "]";
  }

  /*! default template instantiations */
  typedef BBox<vec2<float> > BBox2f;
  typedef BBox<vec3<float> > BBox3f;
}

#endif
