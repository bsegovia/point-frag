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

#include "distribution.hpp"
#include "math/math.hpp"
#include <algorithm>

namespace pf
{
  /////////////////////////////////////////////////////////////////////////////
  // 2D Distribution
  /////////////////////////////////////////////////////////////////////////////
  Distribution1D::Distribution1D() : size(0), PDF(NULL), CDF(NULL) {}

  Distribution1D::Distribution1D(const float* f, size_t size_in) {
    init(f, size_in);
  }

  Distribution1D::~Distribution1D() {
    if (PDF) DELETE_ARRAY(PDF); PDF = NULL;
    if (CDF) DELETE_ARRAY(CDF); CDF = NULL;
  }

  void Distribution1D::init(const float* f, size_t size_in)
  {
    size = size_in;
    PDF = NEW_ARRAY(float, size);
    CDF = NEW_ARRAY(float, size+1);

    CDF[0] = 0.0f;
    for (size_t i=1; i<size+1; i++)
      CDF[i] = CDF[i-1] + f[i-1];

    float rcpSum = CDF[size] == 0.0f ? 0.0f : 1.f / CDF[size];
    for (size_t i = 1; i<size+1; i++) {
      PDF[i-1] = f[i-1] * rcpSum * size;
      CDF[i] *= rcpSum;
    }
    CDF[size] = 1.0f;
  }

  size_t Distribution1D::index(float u) const {
    float* pointer = std::upper_bound(CDF, CDF+size, u);
    return clamp(int(pointer-CDF-1),0,int(size)-1);
  }

  Sample1f Distribution1D::sample(float u) const
  {
    int index = this->index(u);
    float fraction = (u - CDF[index]) * 1.f / (CDF[index+1] - CDF[index]);
    return Sample1f(float(index)+fraction,PDF[index]);
  }

  float Distribution1D::pdf(float p) const {
    return PDF[clamp(int(p*size),0,int(size)-1)];
  }

  /////////////////////////////////////////////////////////////////////////////
  // 2D Distribution
  /////////////////////////////////////////////////////////////////////////////
  Distribution2D::Distribution2D()
    : width(0), height(0), xDists(NULL) {}

  Distribution2D::Distribution2D(const float** f, const size_t width, const size_t height)
    : width(width), height(height), xDists(NULL)
  {
    init(f, width, height);
  }

  Distribution2D::~Distribution2D() {
    SAFE_DELETE_ARRAY(xDists); xDists = NULL;
  }

  void Distribution2D::init(const float** f, const size_t w, const size_t h)
  {
    /*! create arrays */
    SAFE_DELETE_ARRAY(xDists);
    width = w; height = h;
    xDists = NEW_ARRAY(Distribution1D, height);
    float* fy = NEW_ARRAY(float, height);

    /*! compute y distribution and initialize row distributions */
    for (size_t y=0; y<height; y++)
    {
      /*! accumulate row to compute y distribution */
      fy[y] = 0.0f;
      for (size_t x=0; x<width; x++)
        fy[y] += f[y][x];

      /*! initialize distribution for current row */
      xDists[y].init(f[y], width);
    }

    /*! initializes the y distribution */
    yDist.init(fy, height);
    DELETE_ARRAY(fy);
  }

  Sample2f Distribution2D::sample(const vec2f& u) const
  {
    /*! use u.y to sample a row */
    Sample1f sy = yDist.sample(u.y);
    int idx = clamp(int(sy),0,int(height)-1);

    /*! use u.x to sample inside the row */
    Sample1f sx = xDists[idx].sample(u.x);
    return Sample2f(vec2f(sx,sy),sx.pdf*sy.pdf);
  }

  float Distribution2D::pdf(const vec2f& p) const {
    int idx = clamp(int(p.y*height),0,int(height)-1);
    return xDists[idx].pdf(p.x) * yDist.pdf(p.y);
  }
}

