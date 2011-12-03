// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#ifndef __PF_STL_BUFFER_ALLOCATOR_HPP__
#define __PF_STL_BUFFER_ALLOCATOR_HPP__

#include "rdestl_common.hpp"

namespace pf
{
  class buffer_allocator
  {
  public:
    explicit buffer_allocator(const char* name, char* mem, size_t bufferSize) :
      m_name(name),
      m_buffer(mem),
      m_bufferTop(0),
      m_bufferSize(bufferSize)
    { }

    void* allocate(size_t bytes, int /*flags*/ = 0)
    {
      PF_ASSERT(m_bufferTop + bytes <= m_bufferSize);
      char* ret = m_buffer + m_bufferTop;
      m_bufferTop += bytes;
      return ret;
    }
    void deallocate(void* ptr, size_t /*bytes*/)
    {
      PF_ASSERT(ptr == 0 || (ptr >= m_buffer && ptr < m_buffer + m_bufferSize));
    }

    const char* get_name() const  { return m_name; }

  private:
    const char*  m_name;
    char*     m_buffer;
    size_t    m_bufferTop;
    size_t    m_bufferSize;
  };

} /* namespace pf */

#endif /* __PF_STL_BUFFER_ALLOCATOR_HPP__ */

