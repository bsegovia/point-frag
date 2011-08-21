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

#include "compute_bounds.hpp"

namespace pf
{
  ComputeBoundsTask::ComputeBoundsTask(const BuildTriangle* triangles_i,
                                       size_t numTriangles,
                                       Box* prims_o)
    : triangles(triangles_i), numTriangles(numTriangles), prims(prims_o) {}

  void ComputeBoundsTask::go()
  {
    scheduler->addTask((Task::runFunction)&computeBounds,this,8,
                       (Task::completeFunction)&mergeBounds,this);
    scheduler->go();
  }

  void ComputeBoundsTask::computeBounds(size_t tid, ComputeBoundsTask* This, size_t elt)
  {
    /* static work allocation */
    size_t start = elt*This->numTriangles/8;
    size_t end = (elt+1)*This->numTriangles/8;
    Box geomBounds = empty, centBounds = empty;

    for (size_t i=start; i<end; i++) {
      const BuildTriangle& tri = This->triangles[i];
      Box b = merge(merge(Box(tri.v0()),Box(tri.v1())),Box(tri.v2()));
      This->prims[i] = b; This->prims[i].lower.i[3] = (int)i;
      geomBounds = merge(geomBounds,b);
      centBounds = merge(centBounds,center2(b));
    }
    This->geomBounds[elt] = geomBounds;
    This->centBounds[elt] = centBounds;
  }

  void ComputeBoundsTask::mergeBounds(size_t tid, ComputeBoundsTask* This)
  {
    This->geomBound = empty;
    This->centBound = empty;
    for (int i=0; i<8; i++) {
      This->geomBound = merge(This->geomBound,This->geomBounds[i]);
      This->centBound = merge(This->centBound,This->centBounds[i]);
    }
  }
}

