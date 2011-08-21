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

#ifndef __PF_COMPUTE_BOUNDS_H__
#define __PF_COMPUTE_BOUNDS_H__

#include "rtcore/rtcore.hpp"

namespace pf
{
  /*! Computes bounds of triangles. Parallel task that computes the
   *  geometry and centroid bounds of primitives as well as an array
   *  of bounding boxes, one per primitive. */
  class ComputeBoundsTask
  {
  public:

    /*! Constructor. */
    ComputeBoundsTask(const BuildTriangle* triangles_i, size_t numTriangles, Box* prims_o);

    /*! Initiates the parallel compute bounds task. */
    void go();

    /*! Parallel computation of geometry bounding and centroid bounding. */
    static void computeBounds(size_t tid, ComputeBoundsTask* This, size_t elt);

    /*! Merging of bounds. */
    static void mergeBounds(size_t tid, ComputeBoundsTask* This);

  private:
    const BuildTriangle* triangles;   //!< Input triangles.
    size_t numTriangles;              //!< Number of input triangles.
    Box geomBounds[8];                //!< Geometry bounds per thread
    Box centBounds[8];                //!< Centroid bounds per thread

  public:
    Box geomBound;                   //!< Merged geometry bounds.
    Box centBound;                   //!< Merged centroid bounds.
    Box* prims;                      //!< Primitive bounds get stored here.

  private:
    Task::completeFunction continuation; //!< Continuation function
    void* data;                          //!< Argument to call the continuation with
  };
}

#endif
