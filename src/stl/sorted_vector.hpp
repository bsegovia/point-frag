// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_SORTED_VECTOR_HPP__
#define __PF_SORTED_VECTOR_HPP__

#include "functional.hpp"
#include "pair.hpp"
#include "sort.hpp"
#include "vector.hpp"

namespace pf
{
  namespace internal
  {
    template<class TPair, class TFunctor>
    struct compare_func
    {
      bool operator()(const TPair& lhs, const TPair& rhs) const
      {
        return TFunctor()(lhs.first, rhs.first);
      }
      bool operator()(const TPair& lhs, const typename TPair::first_type& rhs) const
      {
        return TFunctor()(lhs.first, rhs);
      }
      bool operator()(const typename TPair::first_type& lhs, const TPair& rhs) const
      {
        return TFunctor()(lhs, rhs.first);
      }
    };
  }

  template<typename TKey,
           typename TValue,
           class TCompare = pf::less<TKey>,
           class TAllocator = pf::allocator,
           class TStorage = pf::standard_vector_storage<pair<TKey, TValue>, TAllocator> >
  class sorted_vector : private vector<pair<TKey, TValue>, TAllocator, TStorage >
  {
    typedef vector<pair<TKey, TValue>, TAllocator, TStorage>  Base;

  public:
    typedef TKey                          key_type;
    typedef TValue                        mapped_type;
    typedef typename Base::size_type      size_type;
    typedef typename Base::value_type     value_type;
    typedef typename Base::iterator       iterator;
    typedef typename Base::const_iterator const_iterator;
    typedef typename Base::allocator_type allocator_type;

    explicit sorted_vector(const allocator_type& allocator = allocator_type())
    :  Base(allocator)
    { }
    template <class InputIterator>
    sorted_vector(InputIterator first,
                  InputIterator last,
                  const allocator_type& allocator = allocator_type()) :
      Base(first, last, allocator)
    {
      pf::quick_sort(begin(), end(), m_compare);
      PF_ASSERT(invariant());
    }

    // @note: no non-const operator[], it may cause performance problems.
    // use explicit ways: insert or find.
    using Base::begin;
    using Base::end;
    using Base::size;
    using Base::empty;
    using Base::capacity;

    pair<iterator, bool> insert(const value_type& val)
    {
      PF_ASSERT(invariant());
      bool found(true);
      iterator it = lower_bound(val.first);
      PF_ASSERT(it == end() || !m_compare(*it, val));
      if (it == end() || m_compare(*it, val))
      {
        it = Base::insert(it, val);
        found = false;
      }
      PF_ASSERT(invariant());
      return pair<iterator, bool>(it, !found);
    }
    // @extension
    INLINE
    pair<iterator, bool> insert(const key_type& k, const mapped_type& v)
    {
      return insert(value_type(k, v));
    }

    iterator find(const key_type& k)
    {
      PF_ASSERT(invariant());
      iterator i(lower_bound(k));
      if (i != end() && m_compare(k, *i))
      {
        i = end();
      }
      return i;
    }
    const_iterator find(const key_type& k) const
    {
      PF_ASSERT(invariant());
      const_iterator i(lower_bound(k));
      if (i != end() && m_compare(k, *i))
      {
        i = end();
      }
      return i;
    }

    INLINE iterator erase(iterator it)
    {
      PF_ASSERT(invariant());
      return Base::erase(it);
    }
    size_type erase(const key_type& k)
    {
      iterator i(find(k));
      if (i == end())
        return 0;
      erase(i);
      PF_ASSERT(invariant());
      return 1;
    }

    using Base::clear;
    using Base::get_allocator;
    using Base::set_allocator;

    iterator lower_bound(const key_type& k)
    {
      return pf::lower_bound(begin(), end(), k, m_compare);
    }
    const_iterator lower_bound(const key_type& k) const
    {
      return pf::lower_bound(begin(), end(), k, m_compare);
    }
    iterator upper_bound(const key_type& k)
    {
      return pf::upper_bound(begin(), end(), k, m_compare);
    }
    const_iterator upper_bound(const key_type& k) const
    {
      return pf::upper_bound(begin(), end(), k, m_compare);
    }
  private:
    // @note: block copying for the time being.
    sorted_vector(const sorted_vector&);
    sorted_vector& operator=(const sorted_vector&);

    bool invariant() const
    {
      // Make sure we're sorted according to m_compare.
      const_iterator first = begin();
      const_iterator last = end();
      PF_ASSERT(last >= first);
      if (first != last)
      {
        const_iterator next = first;
        if (++next != last)
        {
          PF_ASSERT(m_compare(*first, *next));
          first = next;
        }
      }
      return true;
    }

    internal::compare_func<value_type, TCompare>  m_compare;
  };

} /* namespace pf */

#endif /* __PF_SORTED_VECTOR_HPP__ */

