// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_FUNCTIONAL_HPP__
#define __PF_FUNCTIONAL_HPP__

namespace pf
{
  template<typename T>
  struct less {
    bool operator()(const T& lhs, const T& rhs) const { return lhs < rhs; }
  };

  template<typename T>
  struct greater {
    bool operator()(const T& lhs, const T& rhs) const { return lhs > rhs; }
  };

  template<typename T>
  struct equal_to {
    bool operator()(const T& lhs, const T& rhs) const { return lhs == rhs; }
  };
} /* namespace pf */

#endif /* __PF_FUNCTIONAL_HPP__ */

