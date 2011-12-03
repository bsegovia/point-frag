// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STL_FIXED_SORTED_VECTOR_HPP__
#define __PF_STL_FIXED_SORTED_VECTOR_HPP__

#include "fixed_vector.hpp"
#include "sorted_vector.hpp"

namespace pf
{
  template<typename TKey,
           typename TValue,
           int TCapacity,
           bool TGrowOnOverflow,
           class TCompare = pf::less<TKey>,
           class TAllocator = pf::allocator>
  class fixed_sorted_vector :
    public sorted_vector<TKey,
                         TValue,
                         TCompare,
                         TAllocator,
                         fixed_vector_storage<pair<TKey, TValue>,
                                              TAllocator,
                                              TCapacity,
                                              TGrowOnOverflow> >
  {
  public:
    explicit fixed_sorted_vector(const TAllocator& allocator = TAllocator())
    :  sorted_vector<TKey,
                     TValue,
                     TCompare,
                     TAllocator,
                     fixed_vector_storage<pair<TKey, TValue>,
                                          TAllocator,
                                          TCapacity,
                                          TGrowOnOverflow> > (allocator) { }
  };

} /* namespace pf */

#endif /* __PF_STL_FIXED_SORTED_VECTOR_HPP__ */

