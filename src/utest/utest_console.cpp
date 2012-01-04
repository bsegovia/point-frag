// ======================================================================== //
// Copyright (C) 2011 Benjamin Segovia                                      //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "utest/utest.hpp"
#include "sys/console.hpp"
#include "sys/logging.hpp"
#include "sys/set.hpp"
#include <string>

using namespace pf;

void utest_string(void)
{
  std::string s0 = "ll";
  std::string s1 = "lll";
  set<std::string> hop;
  hop.insert("ll");
  hop.insert("lll");
  hop.insert("llol");
  PF_MSG_V(((s0 < s1) ? "true" : "false"));
  PF_MSG_V(*hop.lower_bound("llm"));
}

UTEST_REGISTER(utest_string);

