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

#ifndef __DISTRIBUTION_HPP__
#define __DISTRIBUTION_HPP__

#include "sample.hpp"
#include <cstdlib>

namespace pf
{
  /*! 1D probability distribution. The probability distribution
   *  function (PDF) can be initialized with arbitrary data and be
   *  sampled. */
  class Distribution1D
  {
  public:

    /*! Default construction. */
    Distribution1D();

    /*! Construction from distribution array f. */
    Distribution1D(const float* f, size_t size);

    /*! Destruction. */
    ~Distribution1D();

    /*! Initialized the PDF and CDF arrays. */
    void init(const float* f, size_t size);

    /*! Draws a sample from the distribution. \param u is a random
     *  number to use for sampling */
    Sample1f sample(float u) const;

    /*! Return the index of sample(u) */
    size_t index(float u) const;

    /*! Returns the probability density a sample would be drawn from location p. */
    float pdf(float p) const;

  private:
    size_t size;  //!< Number of elements in the PDF
    float* PDF;   //!< Probability distribution function
    float* CDF;   //!< Cumulative distribution function (required for sampling)
  };

  /*! 2D probability distribution. The probability distribution
   *  function (PDF) can be initialized with arbitrary data and be
   *  sampled. */
  class Distribution2D
  {
  public:

    /*! Default construction. */
    Distribution2D();

    /*! Construction from 2D distribution array f. */
    Distribution2D(const float** f, const size_t width, const size_t height);

    /*! Destruction. */
    ~Distribution2D();

    /*! Initialized the PDF and CDF arrays. */
    void init(const float** f, const size_t width, const size_t height);

  public:

    /*! Draws a sample from the distribution. \param u is a pair of
     *  random numbers to use for sampling */
    Sample2f sample(const vec2f& u) const;

    /*! Returns the probability density a sample would be drawn from
     *  location p. */
    float pdf(const vec2f& p) const;

  private:
    size_t width;            //!< Number of elements in x direction
    size_t height;           //!< Number of elements in y direction
    Distribution1D yDist;    //!< Distribution to select between rows
    Distribution1D* xDists;  //!< One 1D Distribution per row
  };
}
#endif /* __DISTRIBUTION_HPP__ */

