// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#include "list.hpp"

namespace pf
{
  namespace internal
  {
    void list_base_node::link_before(list_base_node* nextNode)
    {
      PF_ASSERT(!in_list());
      prev = nextNode->prev;
      prev->next = this;
      nextNode->prev = this;
      next = nextNode;
    }
    void list_base_node::unlink()
    {
      PF_ASSERT(in_list());
      prev->next = next;
      next->prev = prev;
      next = prev = this;
    }
  } /* namespace internal */
} /* namespace pf */


