// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_TYPETRAITS_HPP__
#define __PF_TYPETRAITS_HPP__

namespace pf
{
  template<typename T> struct is_integral
  {
    enum { value = false };
  };

  template<typename T> struct is_floating_point
  {
    enum { value = false };
  };

#define PF_INTEGRAL(TYPE)  template<> struct is_integral<TYPE> { enum { value = true }; }
  PF_INTEGRAL(char);
  PF_INTEGRAL(bool);
  PF_INTEGRAL(short);
  PF_INTEGRAL(int);
  PF_INTEGRAL(long);
  PF_INTEGRAL(wchar_t);
#undef PF_INTEGRAL

  template<> struct is_floating_point<float> { enum { value = true }; };
  template<> struct is_floating_point<double> { enum { value = true }; };

  template<typename T> struct is_pointer
  {
    enum { value = false };
  };
  template<typename T> struct is_pointer<T*>
  {
    enum { value = true };
  };

  template<typename T> struct is_pod
  {
    enum { value = false };
  };

  template<typename T> struct is_fundamental
  {
    enum
    {
      value = is_integral<T>::value || is_floating_point<T>::value
    };
  };

  template<typename T> struct has_trivial_constructor
  {
    enum
    {
      value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
    };
  };

  template<typename T> struct has_trivial_copy
  {
    enum
    {
      value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
    };
  };

  template<typename T> struct has_trivial_assign
  {
    enum
    {
      value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
    };
  };

  template<typename T> struct has_trivial_destructor
  {
    enum
    {
      value = is_fundamental<T>::value || is_pointer<T>::value || is_pod<T>::value
    };
  };

  template<typename T> struct has_cheap_compare
  {
    enum
    {
      value = has_trivial_copy<T>::value && sizeof(T) <= 4
    };
  };

} /* namespace pf */

#endif /* __PF_TYPETRAITS_HPP__ */

