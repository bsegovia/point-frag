// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_UTILITY_HPP__
#define __PF_UTILITY_HPP__

#include "rdestl_common.hpp"
#include <new>

namespace pf
{
  namespace internal
  {
    template<typename T>
    void copy_n(const T* first, size_t n, T* result, int_to_type<false>)
    {
      const T* last = first + n;
      //while (first != last)
      //  *result++ = *first++;
      switch (n & 0x3)
      {
      case 0:
        while (first != last)
        {
          *result++ = *first++;
      case 3:  *result++ = *first++;
      case 2: *result++ = *first++;
      case 1: *result++ = *first++;
        }
      }
    }
    template<typename T>
    void copy_n(const T* first, size_t n, T* result, int_to_type<true>)
    {
      PF_ASSERT(result >= first + n || result < first);
      Sys::MemCpy(result, first, n * sizeof(T));
    }

    template<typename T>
    void copy(const T* first, const T* last, T* result, int_to_type<false>)
    {
      while (first != last)
        *result++ = *first++;
    }
    template<typename T>
    void copy(const T* first, const T* last, T* result, int_to_type<true>)
    {
      const size_t n = reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first);
      Sys::MemCpy(result, first, n);
    }

    template<typename T> INLINE
    void move_n(const T* from, size_t n, T* result, int_to_type<false>)
    {
      for (int i = int(n) - 1; i >= 0; --i)
        result[i] = from[i];
    }
    template<typename T> INLINE
    void move_n(const T* first, size_t n, T* result, int_to_type<true>)
    {
      Sys::MemMove(result, first, n * sizeof(T));
    }

    template<typename T> INLINE
    void move(const T* first, const T* last, T* result, int_to_type<false>)
    {
      while (--last >= first)
        *result++ = *last;
    }
    template<typename T> INLINE
    void move(const T* first, const T* last, T* result, int_to_type<true>)
    {
      // Meh, MSVC does pretty stupid things here.
      //memmove(result, first, (last - first) * sizeof(T));
      const size_t n = reinterpret_cast<const char*>(last) - reinterpret_cast<const char*>(first);
      //const size_t n = (last - first) * sizeof(T);
      Sys::MemMove(result, first, n);
    }


    template<typename T>
    void copy_construct_n(const T* first, size_t n, T* result, int_to_type<false>)
    {
      for (size_t i = 0; i < n; ++i)
        new (result + i) T(first[i]);
    }
    template<typename T>
    void copy_construct_n(const T* first, size_t n, T* result, int_to_type<true>)
    {
      PF_ASSERT(result >= first + n || result < first);
      Sys::MemCpy(result, first, n * sizeof(T));
    }

    template<typename T>
    void destruct_n(T* first, size_t n, int_to_type<false>)
    {
      // For unknown reason MSVC cant see reference to first here...
      // sizeof(first);
      for (size_t i = 0; i < n; ++i)
        (first + i)->~T();
    }
    template<typename T> INLINE
    void destruct_n(T*, size_t, int_to_type<true>)
    {
      // Nothing to do, no destructor needed.
    }

    template<typename T>
    void destruct(T* mem, int_to_type<false>)
    {
      // sizeof(mem);
      mem->~T();
    }
    template<typename T> INLINE
    void destruct(T*, int_to_type<true>)
    {
      // Nothing to do, no destructor needed.
    }

    template<typename T>
    void construct(T* mem, int_to_type<false>)
    {
      new (mem) T();
    }
    template<typename T> INLINE
    void construct(T*, int_to_type<true>)
    {
      // Nothing to do
    }

    template<typename T> INLINE
    void copy_construct(T* mem, const T& orig, int_to_type<false>)
    {
      new (mem) T(orig);
    }
    template<typename T> INLINE
    void copy_construct(T* mem, const T& orig, int_to_type<true>)
    {
      mem[0] = orig;
    }

    template<typename T>
    void construct_n(T* to, size_t count, int_to_type<false>)
    {
      // sizeof(to);
      for (size_t i = 0; i < count; ++i)
        new (to + i) T();
    }
    template<typename T> inline
    void construct_n(T*, int, int_to_type<true>)
    {
      // trivial ctor, nothing to do.
    }

    // Tests if all elements in range are opfred according to pred.
    template<class TIter, class TPred>
    void test_opfring(TIter first, TIter last, const TPred& pred)
    {
#ifndef NDEBUG
      if (first != last)
      {
        TIter next = first;
        if (++next != last)
        {
          PF_ASSERT(pred(*first, *next));
          first = next;
        }
      }
#else
      // sizeof(first); sizeof(last); sizeof(pred);
#endif
    }

    template<typename T1, typename T2, class TPred> inline
    bool debug_pred(const TPred& pred, const T1& a, const T2& b)
    {
#ifndef NDEBUG
      if (pred(a, b))
      {
        PF_ASSERT(!pred(b, a));
        return true;
      }
      else
      {
        return false;
      }
#else
      return pred(a, b);
#endif
    }
  } /* namespace internal */
} /* namespace pf */

#endif /* __PF_UTILITY_HPP__ */

