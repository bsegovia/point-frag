// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#include "slist.hpp"

namespace pf
{
  namespace internal
  {
    void slist_base_node::link_after(slist_base_node* prevNode)
    {
      PF_ASSERT(!in_list());
      next = prevNode->next;
      prevNode->next = this;
    }
    void slist_base_node::unlink(slist_base_node* prevNode)
    {
      PF_ASSERT(in_list());
      PF_ASSERT(prevNode->next == this);
      prevNode->next = next;
      next = this;
    }
  }
} /* namespace pf */

