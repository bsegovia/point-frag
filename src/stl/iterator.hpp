// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_ITERATOR_HPP__
#define __PF_ITERATOR_HPP__

#include "rdestl_common.hpp"

namespace pf
{
  struct input_iterator_tag {};
  struct output_iterator_tag {};
  struct forward_iterator_tag: public input_iterator_tag {};
  struct bidirectional_iterator_tag: public forward_iterator_tag {};
  struct random_access_iterator_tag: public bidirectional_iterator_tag {};

  template<typename IterT>
  struct iterator_traits
  {
     typedef typename IterT::iterator_category iterator_category;
  };

  template<typename T>
  struct iterator_traits<T*>
  {
     typedef random_access_iterator_tag iterator_category;
  };

  namespace internal
  {
    template<typename TIter, typename TDist> INLINE
    void distance(TIter first, TIter last, TDist& dist, pf::random_access_iterator_tag)
    {
      dist = TDist(last - first);
    }
    template<typename TIter, typename TDist> INLINE
    void distance(TIter first, TIter last, TDist& dist, pf::input_iterator_tag)
    {
      dist = 0;
      while (first != last) {
        ++dist;
        ++first;
      }
    }

    template<typename TIter, typename TDist> INLINE
    void advance(TIter& iter, TDist d, pf::random_access_iterator_tag)
    {
      iter += d;
    }
    template<typename TIter, typename TDist> INLINE
    void advance(TIter& iter, TDist d, pf::bidirectional_iterator_tag)
    {
      if (d >= 0) {
        while (d--)
          ++iter;
      } else {
        while (d++)
          --iter;
      }
    }
    template<typename TIter, typename TDist> INLINE
    void advance(TIter& iter, TDist d, pf::input_iterator_tag)
    {
      PF_ASSERT(d >= 0);
      while (d--)
        ++iter;
    }
  } /* namespace internal */
} /* namespace pf */

#endif /* __PF_ITERATOR_HPP__ */
