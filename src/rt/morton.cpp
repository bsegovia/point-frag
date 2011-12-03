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

#include <cstdint>
#include <cassert>
#include <iostream>
#include <iomanip>

// C code to generate the morton curve look-up tables
static const int32_t dim = 64;
static const int32_t n = dim*dim;

int main(int argc, char *argv[])
{
  int32_t mortonX[n];
  int32_t mortonY[n];

  for (uint32_t i = 0; i < n; ++i) mortonX[i] = mortonY[i] = -1;
  for (uint16_t y = 0; y < dim; ++y)
  for (uint16_t x = 0; x < dim; ++x) {
    uint32_t z = 0;
    for (int i = 0; i < sizeof(x) * 8; i++)
      z |= (x & 1U << i) << i | (y & 1U << i) << (i + 1);
    mortonX[z] = x;
    mortonY[z] = y;
  }

#ifndef NDEBUG
  for (uint32_t i = 0; i < n; ++i) PF_ASSERT(mortonX[i] != -1 && mortonY[i] != -1);
#endif /* NDEBUG */

  std::cout << "const int32 ALIGNED(16) RTCameraPacketGen::mortonX[] = {" << std::endl;
  for (uint32_t i = 0; i < n; i += 16) {
    for (uint32_t j = i; j < i + 16; ++j)
      std::cout << std::setw(2) << mortonX[j] << ", ";
    std::cout << std::endl;
  }
  std::cout << "};" << std::endl;
  std::cout << std::endl;

  std::cout << "const int32 ALIGNED(16) RTCameraPacketGen::mortonY[] = {" << std::endl;
  for (uint32_t i = 0; i < n; i += 16) {
    for (uint32_t j = i; j < i + 16; ++j)
      std::cout << std::setw(2) << mortonY[j] << ", ";
    std::cout << std::endl;
  }
  std::cout << "};" << std::endl;
  std::cout << std::endl;
}

