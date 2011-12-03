// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STRING_HPP__
#define __PF_STRING_HPP__

#include "basic_string.hpp"
#include "hash.hpp"

namespace pf
{
  typedef basic_string<char>  string;

  template<typename E, class TAllocator, typename TStorage>
  struct hash<basic_string<E, TAllocator, TStorage> >
  {
    hash_value_t operator()(const basic_string<E, TAllocator, TStorage>& x) const
    {
      // Derived from: http://blade.nagaokaut.ac.jp/cgi-bin/scat.rb/ruby/ruby-talk/142054
      hash_value_t h = 0;
      for (typename basic_string<E, TAllocator, TStorage>::size_type p = 0; p < x.length(); ++p)
      {
        h = x[p] + (h<<6) + (h<<16) - h;
      }
      return h & 0x7FFFFFFF;
    }
  };
} /* namespace pf */

#endif /* __PF_STRING_HPP__ */
