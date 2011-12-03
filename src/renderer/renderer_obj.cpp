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

#include "renderer/renderer_obj.hpp"
#include "renderer/renderer_segment.hpp"
#include "renderer/renderer.hpp"
#include "models/obj.hpp"
#include "rt/bvh2.hpp"
#include "rt/bvh2_node.hpp"
#include "rt/rt_triangle.hpp"
#include "sys/logging.hpp"

#include <cstring>
#include <cassert>
#include <algorithm>

namespace pf
{
#define OGL_NAME (this->renderer.driver)

  /*! Append the newly loaded texture in the obj (XXX hack make something
   *  better later rather than using lock on the renderer obj. Better should
   *  be to make renderer obj immutable and replace it when all textures are
   *  loaded)
   */
  class TaskUpdateObjTexture : public Task
  {
  public:
    /*! Update the renderer object textures */
    TaskUpdateObjTexture(TextureStreamer &streamer,
                         RendererObj &obj,
                         const std::string &name) :
      Task("TaskUpdateObjTexture"), streamer(streamer), obj(obj), name(name) {}

    /*! Update the renderer obj with the fully loaded textures */
    virtual Task* run(void) {
      const TextureState state = streamer.getTextureState(name);
      PF_ASSERT(state.value == TextureState::COMPLETE);
      Lock<MutexSys> lock(obj.mutex);
      for (size_t matID = 0; matID < obj.matNum; ++matID)
        if (obj.mat[matID].name_Kd == name)
          obj.mat[matID].map_Kd = state.tex;
      return NULL;
    }
  private:
    TextureStreamer &streamer;  //!< Where to get the handle
    RendererObj &obj;           //!< The object to upate
    std::string name;           //!< Name of the texture to get
  };

  /*! Load all textures for the given renderer obj */
  class TaskLoadObjTexture : public Task
  {
  public:
    /*! The loads are triggered asynchronously */
    TaskLoadObjTexture(TextureStreamer &streamer,
                       RendererObj &rendererObj,
                       const Obj &obj) :
      Task("TaskLoadObjTexture"), streamer(streamer), rendererObj(rendererObj)
    {
      this->setPriority(TaskPriority::HIGH);
      if (obj.matNum) {
        this->texNum = obj.matNum;
        this->texName = PF_NEW_ARRAY(std::string, this->texNum);
        for (size_t i = 0; i < this->texNum; ++i) {
          const char *name = obj.mat[i].map_Kd;
          if (name == NULL || strlen(name) == 0)
            continue;
          this->texName[i] = name;
        }
      } else {
        this->texNum = 0;
        this->texName = NULL;
      }
    }

    /*! Only release everything after the completion of all sub-tasks */
    ~TaskLoadObjTexture(void) { PF_SAFE_DELETE_ARRAY(this->texName); }

    /*! Spawn one loading task per group */
    virtual Task* run(void) {
      for (size_t i = 0; i < texNum; ++i) {
        if (texName[i].size() == 0) continue;
        //const TextureRequest req(texName[i], PF_TEX_FORMAT_DXT1);
        const TextureRequest req(texName[i], PF_TEX_FORMAT_PLAIN);
        Ref<Task> loading = streamer.createLoadTask(req);
        if (loading) {
          Ref<Task> updateObj = PF_NEW(TaskUpdateObjTexture, streamer, rendererObj, texName[i]);
          loading->starts(updateObj.ptr);
          updateObj->ends(this);
          updateObj->scheduled();
          loading->scheduled();
        }
      }
      return NULL;
    }
  private:
    TextureStreamer &streamer; //!< The streamer that handles the loads
    RendererObj &rendererObj;  //!< The renderer object to update
    std::string *texName;      //!< All the textures to load
    size_t texNum;             //!< The number of texture to load
  };

  /*! Compute half of the node area */
  INLINE float halfArea(const BVH2Node &node) {
    const vec3f d = node.pmax - node.pmin;
    return d.x*(d.y+d.z)+d.y*d.z;
  }

  /*! Make the bbox bigger with the given triangle */
  INLINE void growBBox(BBox3f &bbox, const Obj &obj, uint32 triID) {
    for (int v = 0; v < 3; ++v) {
      const int vertID = obj.tri[triID].v[v];
      bbox.grow(obj.vert[vertID].p);
    }
  }

  /*! Initialize a new segment. triID is the triangle index in the Obj
   *  structure and first is the ID of the same triangle in the sorted array of
   *  triangle indices
   */
  INLINE void startSegment(RendererSegment &segment,
                           const Obj &obj,
                           uint32 triID,
                           uint32 first)
  {
    segment.first = first;
    segment.bbox = BBox3f(empty);
    segment.matID = obj.tri[triID].m; // Be aware, they must be remapped
  }

  /*! Symmetry maniacs. We finish the segments with the last primitive index */
  INLINE void endSegment(RendererSegment &segment, uint32 last)
  {
    segment.last = last;
  }

  /*! Sort the triangle ID according to their material */
  struct TriangeIDSorter {
    TriangeIDSorter(const Obj::Triangle *tri) : tri(tri) {}
    INLINE int operator() (const uint32 a, const uint32 b) const {
      return tri[a].m < tri[b].m;
    }
    const Obj::Triangle *tri;
  };

  static void doSegment(const Obj &obj,
                        BVH2<RTTriangle> &bvh,
                        std::vector<RendererSegment> &segments,
                        uint32 nodeID)
  {
    BVH2Node &node = bvh.node[nodeID];
    const uint32 first = node.getPrimID();
    const uint32 n = node.getPrimNum();
    const uint32 last = first + n - 1;
    const uint32 firstSegment = segments.size();
    uint32 *IDs = bvh.primID + first;
    std::sort(IDs, IDs+n, TriangeIDSorter(obj.tri));
    PF_ASSERT(n > 0);

    // Allocate the segments and build them
    uint32 segmentID = firstSegment;
    uint32 triID = IDs[0];
    int32 matID = obj.tri[triID].m;
    segments.push_back(RendererSegment());
    startSegment(segments[segmentID], obj, triID, first);

    // When a new material is encountered, we finish the current segments and
    // start a new one
    for (size_t i = 1; i < n; ++i) {
      triID = IDs[i];
      if (obj.tri[triID].m != matID) {
        // We are done with this segment
        endSegment(segments[segmentID], first+i-1);

        // Start the new one
        matID = obj.tri[triID].m;
        segmentID = segments.size();
        segments.push_back(RendererSegment());
        startSegment(segments[segmentID], obj, triID, first+i);
      }
      growBBox(segments[segmentID].bbox, obj, triID);
    }
    endSegment(segments[segmentID], last);
    node.setPrimID(firstSegment);
  }

  /*! Create segments from a SAH BVH of triangles */
  static GLuint *RendererObjSegment(RendererObj &renderObj, const Obj &obj)
  {
    // Compute the RT triangles first
    PF_MSG_V("RendererObj: building BVH of segments");
    RTTriangle *tris = PF_NEW_ARRAY(RTTriangle, obj.triNum);
    for (size_t i = 0; i < obj.triNum; ++i) {
      const vec3f &v0 = obj.vert[obj.tri[i].v[0]].p;
      const vec3f &v1 = obj.vert[obj.tri[i].v[1]].p;
      const vec3f &v2 = obj.vert[obj.tri[i].v[2]].p;
      tris[i] = RTTriangle(v0,v1,v2);
    }

    // Build the BVH with appropriate options (this is *not* for ray tracing
    // but for GPU rasterization)
    Ref< BVH2<RTTriangle> > bvh = PF_NEW(BVH2<RTTriangle>);
    const BVH2BuildOption option(1024, 0xffffffff, 1.f, 1024.f);
    buildBVH2(tris, obj.triNum, *bvh, option);
    PF_DELETE_ARRAY(tris);

    // Traverse all leaves and create the segments
    std::vector<RendererSegment> segments;
    for (size_t nodeID = 0; nodeID < bvh->nodeNum; ++nodeID) {
      BVH2Node &node = bvh->node[nodeID];
      if (node.isLeaf()) doSegment(obj, *bvh, segments, nodeID);
    }

    // Now we allocate the segments in the OBJ
    PF_MSG_V("RendererObj: " << segments.size() << " segments are created");
    renderObj.segmentNum = segments.size();
    renderObj.segments = PF_NEW_ARRAY(RendererSegment, renderObj.segmentNum);
    for (size_t segmentID = 0; segmentID < renderObj.segmentNum; ++segmentID)
      renderObj.segments[segmentID] = segments[segmentID];

    // Build the new index buffer
    GLuint *indices = PF_NEW_ARRAY(GLuint, obj.triNum * 3);
    for (size_t from = 0, to = 0; from < obj.triNum; ++from, to += 3) {
      const uint32 triID = bvh->primID[from];
      PF_ASSERT(triID < obj.triNum);
      indices[to+0] = obj.tri[triID].v[0];
      indices[to+1] = obj.tri[triID].v[1];
      indices[to+2] = obj.tri[triID].v[2];
    }

    return indices;
  }

  RendererObj::RendererObj(Renderer &renderer, const Obj &obj) :
    renderer(renderer), mat(NULL), segments(NULL), matNum(0), segmentNum(0),
    vertexArray(0), arrayBuffer(0), elementBuffer(0)
  {
    TextureStreamer &streamer = *renderer.streamer;

    if (obj.isValid()) {
      PF_MSG_V("RendererObj: asynchronously loading textures");

      // Map each material group to the texture name. Since we also remove the
      // possibly unused materials, we remap their IDs
      uint32 *matRemap = PF_NEW_ARRAY(uint32, obj.matNum);
      this->mat = PF_NEW_ARRAY(Material, obj.grpNum);
      this->matNum = obj.grpNum;
      for (size_t i = 0; i < this->matNum; ++i) {
        const int32 matID = obj.grp[i].m;
        PF_ASSERT(matID >= 0); // We ensure that in the loader
        matRemap[matID] = i;
        Material &material = this->mat[i];
        material.map_Kd = renderer.defaultTex;
        material.name_Kd = obj.mat[matID].map_Kd;
      }

      // Start to load the textures
      this->texLoading = PF_NEW(TaskLoadObjTexture, streamer, *this, obj);
      this->texLoading->scheduled();

      // Right now we only create one segments per material
      PF_MSG_V("RendererObj: creating geometry segments");
      GLuint *indices = RendererObjSegment(*this, obj);
      for (size_t segmentID = 0; segmentID < segmentNum; ++segmentID)
        segments[segmentID].matID = matRemap[segments[segmentID].matID];

      // Create the OGL index buffer
      PF_MSG_V("RendererObj: creating OGL objects");
      const size_t indexSize = sizeof(GLuint[3]) * obj.triNum;
      R_CALL (GenBuffers, 1, &this->elementBuffer);
      R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, this->elementBuffer);
      R_CALL (BufferData, GL_ELEMENT_ARRAY_BUFFER, indexSize, indices, GL_STATIC_DRAW);
      R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
      PF_DELETE_ARRAY(indices);

      // Create the OGL vertex buffer
      const size_t vertexSize = obj.vertNum * sizeof(Obj::Vertex);
      R_CALL (GenBuffers, 1, &this->arrayBuffer);
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->arrayBuffer);
      R_CALL (BufferData, GL_ARRAY_BUFFER, vertexSize, obj.vert, GL_STATIC_DRAW);
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);

      // Create the OGL vertex array
      R_CALL (GenVertexArrays, 1, &this->vertexArray);
      R_CALL (BindVertexArray, this->vertexArray);
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->arrayBuffer);
      R_CALL (VertexAttribPointer, RendererDriver::ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Obj::Vertex), NULL);
      R_CALL (VertexAttribPointer, RendererDriver::ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Obj::Vertex), (void*)offsetof(Obj::Vertex, t));
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
      R_CALL (EnableVertexAttribArray, RendererDriver::ATTR_POSITION);
      R_CALL (EnableVertexAttribArray, RendererDriver::ATTR_TEXCOORD);
      R_CALL (BindVertexArray, 0);
      PF_SAFE_DELETE_ARRAY(matRemap);
    }
  }

  RendererObj::~RendererObj(void) {
    if (this->texLoading)    this->texLoading->waitForCompletion();
    if (this->vertexArray)   R_CALL (DeleteVertexArrays, 1, &this->vertexArray);
    if (this->arrayBuffer)   R_CALL (DeleteBuffers, 1, &this->arrayBuffer);
    if (this->elementBuffer) R_CALL (DeleteBuffers, 1, &this->elementBuffer);
    PF_SAFE_DELETE_ARRAY(this->segments);
    PF_SAFE_DELETE_ARRAY(this->mat);
  }

  void RendererObj::display(void) {
    Lock<MutexSys> lock(mutex);
    R_CALL (BindVertexArray, vertexArray);
    R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    R_CALL (ActiveTexture, GL_TEXTURE0);
    for (size_t segmentID = 0; segmentID < segmentNum; ++segmentID) {
      const RendererSegment &segment = segments[segmentID];
      const Material &material = mat[segment.matID];
      const uintptr_t offset = segment.first * sizeof(int[3]);
      const GLuint n = segment.last - segment.first + 1;
      R_CALL (BindTexture, GL_TEXTURE_2D, material.map_Kd->handle);
      R_CALL (DrawElements, GL_TRIANGLES, 3*n, GL_UNSIGNED_INT, (void*) offset);
    }
    R_CALL (BindVertexArray, 0);
    R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
  }

#undef OGL_NAME
}

