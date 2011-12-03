// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STACK_ALLOCATOR_HPP__
#define __PF_STACK_ALLOCATOR_HPP__

#include "rdestl_common.hpp"

namespace pf
{
  /*! Stack based allocator.
   *  Traits:
   *   - operates on buffer of TBytes bytes of stack memory
   *   - never frees memory
   *   - cannot be copied
   */
  template<int TBytes>
  class stack_allocator
  {
  public:
    explicit stack_allocator(const char* name = "STACK") :
      m_name(name),
      m_bufferTop(0)
    {}

    void* allocate(size_t bytes, int /*flags*/ = 0)
    {
      PF_ASSERT(m_bufferTop + bytes <= TBytes);
      char* ret = &m_buffer[0] + m_bufferTop;
      m_bufferTop += bytes;
      return ret;
    }
    void deallocate(void* ptr, size_t /*bytes*/)
    {
      PF_ASSERT(ptr == 0 || (ptr >= &m_buffer[0] && ptr < &m_buffer[TBytes]));
    }

    const char* get_name() const  { return m_name; }

  private:
    stack_allocator(const stack_allocator&);
    stack_allocator& operator=(const stack_allocator&);

    const char*  m_name;
    char         m_buffer[TBytes];
    size_t       m_bufferTop;
  };

} /* namespace pf */

#endif /* __PF_STACK_ALLOCATOR_HPP__ */

