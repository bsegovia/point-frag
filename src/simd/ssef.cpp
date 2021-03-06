// ======================================================================== //
// Copyright 2009-2011 Intel Corporation                                    //
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

#include "ssef.hpp"
namespace pf
{
  const ssef ssef::ONE(1.f,1.f,1.f,1.f);
  const ssef ssef::IDENTITY(0.f,1.f,2.f,3.f);
  const ssef ssef::LANE_NUM(4.f,4.f,4.f,4.f);
  const ssef ssef::EPSILON(1e-10f,1e-10f,1e-10f,1e-10f);
} /* namespace pf */

