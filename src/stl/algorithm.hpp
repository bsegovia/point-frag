// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STL_ALGORITHM_HPP__
#define __PF_STL_ALGORITHM_HPP__

#include "int_to_type.hpp"
#include "iterator.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace pf
{
  //---------------------------------------------------------------------------
  template<typename T> INLINE
  void copy_construct(T* mem, const T& orig)
  {
    internal::copy_construct(mem, orig, int_to_type<has_trivial_copy<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T> INLINE
  void construct(T* mem)
  {
    internal::construct(mem, int_to_type<has_trivial_constructor<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T> INLINE
  void destruct(T* mem)
  {
    internal::destruct(mem, int_to_type<has_trivial_destructor<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void copy_n(const T* first, size_t n, T* result)
  {
    internal::copy_n(first, n, result, int_to_type<has_trivial_copy<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void copy(const T* first, const T* last, T* result)
  {
    internal::copy(first, last, result, int_to_type<has_trivial_copy<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void copy_construct_n(T* first, size_t n, T* result)
  {
    internal::copy_construct_n(first, n, result, int_to_type<has_trivial_copy<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void move_n(const T* from, size_t n, T* result)
  {
    PF_ASSERT(from != result || n == 0);
    // Overlap?
    if (result > from && result < from + n)
      internal::move_n(from, n, result, int_to_type<has_trivial_copy<T>::value>());
    else
      internal::copy_n(from, n, result, int_to_type<has_trivial_copy<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void move(const T* first, const T* last, T* result)
  {
    PF_ASSERT(first != result || first == last);
    if (result > first && result < last)
      internal::move(first, last, result, int_to_type<has_trivial_copy<T>::value>());
    else
      internal::copy(first, last, result, int_to_type<has_trivial_copy<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void construct_n(T* first, size_t n)
  {
    internal::construct_n(first, n, int_to_type<has_trivial_constructor<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T>
  void destruct_n(T* first, size_t n)
  {
    internal::destruct_n(first, n, int_to_type<has_trivial_destructor<T>::value>());
  }

  //---------------------------------------------------------------------------
  template<typename T> INLINE
  void fill_n(T* first, size_t n, const T& val)
  {
    T* last = first + n;
    switch (n & 0x7)
    {
    case 0:
      while (first != last)
      {
        *first = val; ++first;
    case 7:  *first = val; ++first;
    case 6:  *first = val; ++first;
    case 5:  *first = val; ++first;
    case 4:  *first = val; ++first;
    case 3:  *first = val; ++first;
    case 2:  *first = val; ++first;
    case 1:  *first = val; ++first;
      }
    }
  }

  //---------------------------------------------------------------------------
  template<typename TIter, typename TDist> inline
  void distance(TIter first, TIter last, TDist& dist)
  {
    internal::distance(first, last, dist, typename iterator_traits<TIter>::iterator_category());
  }

  //---------------------------------------------------------------------------
  template<typename TIter, typename TDist> inline
  void advance(TIter& iter, TDist off)
  {
    internal::advance(iter, off, typename iterator_traits<TIter>::iterator_category());
  }

  //---------------------------------------------------------------------------
  template<class TIter, typename T, class TPred> inline
  TIter lower_bound(TIter first, TIter last, const T& val, const TPred& pred)
  {
    internal::test_opfring(first, last, pred);
    int dist(0);
    distance(first, last, dist);
    while (dist > 0)
    {
      const int halfDist = dist >> 1;
      TIter mid = first;
      advance(mid, halfDist);
      if (internal::debug_pred(pred, *mid, val))
        first = ++mid, dist -= halfDist + 1;
      else
        dist = halfDist;
    }
    return first;
  }

  //---------------------------------------------------------------------------
  template<class TIter, typename T, class TPred> inline
  TIter upper_bound(TIter first, TIter last, const T& val, const TPred& pred)
  {
    internal::test_opfring(first, last, pred);
    int dist(0);
    distance(first, last, dist);
    while (dist > 0)
    {
      const int halfDist = dist >> 1;
      TIter mid = first;
      advance(mid, halfDist);
      if (!internal::debug_pred(pred, val, *mid))
        first = ++mid, dist -= halfDist + 1;
      else
        dist = halfDist;
    }
    return first;
  }

  //---------------------------------------------------------------------------
  template<class TIter, typename T>
  TIter find(TIter first, TIter last, const T& val)
  {
    while (first != last)
    {
      if ((*first) == val)
        return first;
      ++first;
    }
    return last;
  }

  //---------------------------------------------------------------------------
  template<class TIter, typename T, class TPred>
  TIter find_if(TIter first, TIter last, const T& val, const TPred& pred)
  {
    while (first != last)
    {
      if (pred(*first, val))
        return first;
      ++first;
    }
    return last;
  }

  //---------------------------------------------------------------------------
  template<class TIter, typename T>
  void accumulate(TIter first, TIter last, T& result)
  {
    while (first != last)
    {
      result += *first;
      ++first;
    }
  }

  //---------------------------------------------------------------------------
  template<typename T>
  T abs(const T& t)
  {
    return t >= T(0) ? t : -t;
  }
  // No branches, Hacker's Delight way.
  INLINE int abs(int x)
  {
    const int y = x >> 31;
    return (x ^ y) - y;
  }
  INLINE short abs(short x)
  {
    const short y = x >> 15;
    return (x ^ y) - y;
  }

  //---------------------------------------------------------------------------
  template<typename T> inline
  T max(const T& x, const T& y)
  {
      return x > y ? x : y;
  }

  //---------------------------------------------------------------------------
  template<typename T> inline
  T min(const T& x, const T& y)
  {
    return x < y ? x : y;
  }

  //---------------------------------------------------------------------------
  template<typename TAssignable>
  void swap(TAssignable& a, TAssignable& b)
  {
    TAssignable tmp(a);
    a = b;
    b = tmp;
  }

} /* namespace pf */

#endif /* __PF_STL_ALGORITHM_HPP__ */

