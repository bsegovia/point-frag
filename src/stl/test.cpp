// ======================================================================== //
// Directly taken and adapted from RDESTL                                   //
// http://code.google.com/p/rdestl/                                         //
// ======================================================================== //

#include "algorithm.hpp"
#include "alignment.hpp"
#include "allocator.hpp"
#include "basic_string.hpp"
#include "buffer_allocator.hpp"
#include "fixed_array.hpp"
#include "fixed_list.hpp"
#include "fixed_sorted_vector.hpp"
#include "fixed_substring.hpp"
#include "fixed_vector.hpp"
#include "sorted_vector.hpp"
#include "hash.hpp"
#include "hash_map.hpp"
#include "intrusive_list.hpp"
#include "intrusive_slist.hpp"
#include "iterator.hpp"
#include "list.hpp"
#include "pair.hpp"
#include "radix_sorter.hpp"
#include "rb_tree.hpp"
#include "rdestl_common.hpp"
#include "set.hpp"
#include "slist.hpp"
#include "sstream.hpp"
#include "stack_allocator.hpp"
#include "stack.hpp"
#include "vector.hpp"

namespace pf{
typedef basic_string<char> string;

int p(const string &d, const string &g) { return 0;}
int p(fixed_array<char,8> &s) {return 3;}
int p(fixed_list<char,8> &s) {return 3;}
int p(sorted_vector<char,int> &s) {return 3;}
int p(hash<int> &s) {return 3;}
int p(hash_map<int, char> &s) {return 3;}
int p(intrusive_list<int> &s) {return 3;}
int p(intrusive_slist<int*> &s) {return 3;}
int p(list<int*> &s) {return 3;}
int p(pair<int,char> &s) {return 3;}
int p(radix_sorter<int> &s) {return 3;}
int p(rb_tree<int> &s) {return 3;}
int p(set<int> &s) {return 3;}
int p(slist<int> &s) {return 3;}
int p(vector<int> &s) {return 3;}
void q()
{
  int x[13];
  quick_sort(x,x+13);
}

void d()
{
  stringstream p;

}
int qq(stack<int> &D) { return 1;}
int sqq(stack_allocator<2> &D) { return 1;}
}


