// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_INT_TO_TYPE_HPP__
#define __PF_INT_TO_TYPE_HPP__

namespace pf
{
  /**
   * Sample usage:
   *  void fun(int_to_type<true>)  { ... }
   *  void fun(int_to_type<false>) { ... }
   *  template<typename T> void bar()
   *  {
   *    fun(int_to_type<std::numeric_limits<T>::is_exact>())
   *  }
   */
  template<int TVal>
  struct int_to_type
  {
    enum { value = TVal };
  };
} /* namespace pf */

#endif /* __PF_INT_TO_TYPE_HPP__ */

