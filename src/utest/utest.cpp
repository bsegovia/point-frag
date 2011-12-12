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

#include "utest.hpp"
#include "sys/string.hpp"

namespace pf
{
  std::vector<UTest> *UTest::utestList = NULL;
  void releaseUTestList(void) { if (UTest::utestList) delete UTest::utestList; }

  UTest::UTest(Function fn, const char *name) : fn(fn), name(name) {
    if (utestList == NULL) {
      utestList = new std::vector<UTest>;
      atexit(releaseUTestList);
    }
    utestList->push_back(*this);
  }

  UTest::UTest(void) : fn(NULL), name(NULL) {}

  void UTest::run(const char *name) {
    if (name == NULL) return;
    if (utestList == NULL) return;
    for (size_t i = 0; i < utestList->size(); ++i) {
      const UTest &utest = (*utestList)[i];
      if (utest.name == NULL || utest.fn == NULL) continue;
      if (strequal(utest.name, name)) (utest.fn)();
    }
  }

  void UTest::runAll(void) {
    if (utestList == NULL) return;
    for (size_t i = 0; i < utestList->size(); ++i) {
      const UTest &utest = (*utestList)[i];
      if (utest.fn == NULL) continue;
      (utest.fn)();
    }
  }
} /* namespace pf */

