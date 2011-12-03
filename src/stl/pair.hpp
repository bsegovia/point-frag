// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_PAIR_HPP__
#define __PF_PAIR_HPP__

#include "type_traits.hpp"

namespace pf
{
  template<typename T1, typename T2>
  struct pair
  {
    typedef T1  first_type;
    typedef T2  second_type;

    pair() {}
    pair(const T1& a, const T2& b):  first(a), second(b) {}
    explicit pair(const T1& a):  first(a) {}
    pair(const pair<T1,T2>& rhs) : first(rhs.first), second(rhs.second) {}

    pair& operator=(const pair<T1,T2>& rhs) {
      first = rhs.first;
      second = rhs.second;
      return *this;
    }

    T1  first;
    T2  second;
  };

  template<typename T1, typename T2> struct is_pod<pair<T1, T2> >
  {
    enum { value = (is_pod<T1>::value || is_fundamental<T1>::value) &&
                   (is_pod<T2>::value || is_fundamental<T2>::value) };
  };

  template<typename T1, typename T2>
  pair<T1, T2> make_pair(const T1& a, const T2& b)
  {
    return pair<T1, T2>(a, b);
  }

} /* namespace pf */

#endif /* __PF_PAIR_HPP__ */

