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

#ifndef __PF_UTEST_HPP__
#define __PF_UTEST_HPP__

#include <vector>

namespace pf
{
  /*! Quick and dirty Unit test system with registration */
  struct UTest
  {
    /*! A unit test function to run */
    typedef void (*Function) (void);
    /*! Empty test */
    UTest(void);
    /*! Build a new unit test and append it to the unit test list */
    UTest(Function fn, const char *name);
    /*! Function to execute */
    Function fn;
    /*! Name of the test */
    const char *name;
    /*! The tests that are registered */
    static std::vector<UTest> *utestList;
    /*! Run the test with the given name */
    static void run(const char *name);
    /*! Run all the tests */
    static void runAll(void);
  };
} /* namespace pf */

/*! Register a new unit test */
#define UTEST_REGISTER(FN) static const pf::UTest __##NAME##__(FN, #FN);

#endif /* __PF_UTEST_HPP__ */

