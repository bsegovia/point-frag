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

#include "renderer/renderer.hpp"
#include "renderer/renderer_context.hpp"
#include "renderer/renderer_obj.hpp"
#include "renderer/renderer_segment.hpp"
#include "renderer/renderer_driver.hpp"
#include "models/obj.hpp"
#include "rt/bvh2.hpp"
#include "rt/bvh2_node.hpp"
#include "rt/bvh2_traverser.hpp"
#include "rt/rt_triangle.hpp"
#include "sys/logging.hpp"
#include "sys/tasking_utility.hpp"

#include <cstring>
#include <cassert>
#include <algorithm>

namespace pf
{
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
                         Ref<RendererObj> renderObj,
                         const std::string &name) :
      Task("TaskUpdateObjTexture"), streamer(streamer), renderObj(renderObj), name(name) {}

    /*! Update the renderer obj with the fully loaded textures */
    virtual Task* run(void) {
      const TextureState state = streamer.getTextureState(name);
      PF_ASSERT(state.value == TextureState::COMPLETE);
      Lock<MutexSys> lock(renderObj->mutex);
      for (size_t matID = 0; matID < renderObj->mat.size(); ++matID)
        if (renderObj->mat[matID].name_Kd == name)
          renderObj->mat[matID].map_Kd = state.tex;
      return NULL;
    }
  private:
    TextureStreamer &streamer;  //!< Where to get the handle
    Ref<RendererObj> renderObj; //!< The object to upate
    std::string name;           //!< Name of the texture to get
  };

  /*! Load all textures for the given renderer obj */
  class TaskLoadObjTexture : public Task
  {
  public:
    /*! The loads are triggered asynchronously */
    TaskLoadObjTexture(TextureStreamer &streamer,
                       Ref<RendererObj> renderObj,
                       const Obj &obj) :
      Task("TaskLoadObjTexture"), streamer(streamer), renderObj(renderObj)
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
        const TextureRequest req(texName[i], PF_TEX_FORMAT_DXT1);
        //const TextureRequest req(texName[i], PF_TEX_FORMAT_PLAIN);
        Ref<Task> loading = streamer.createLoadTask(req);
        if (loading) {
          Ref<Task> updateObj = PF_NEW(TaskUpdateObjTexture, streamer, renderObj, texName[i]);
          loading->starts(updateObj);
          updateObj->ends(this);
          updateObj->scheduled();
          loading->scheduled();
        }
      }
      return NULL;
    }
  private:
    TextureStreamer &streamer;  //!< The streamer that handles the loads
    Ref<RendererObj> renderObj; //!< The renderer object to update
    std::string *texName;       //!< All the textures to load
    size_t texNum;              //!< The number of texture to load
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

  /*! Symetry maniacs. We finish the segments with the last primitive index */
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
    const uint32 segmentNum = segments.size();
    renderObj.segments.resize(segmentNum);
    for (size_t segmentID = 0; segmentID < segmentNum; ++segmentID)
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

  /*! Data to upload to OGL and possibly shared by Renderer Set */
  struct RendererObjSharedData : public RefCount
  {
    RendererObjSharedData(void);
    ~RendererObjSharedData(void);
    void upload(RendererObj &obj);
    Obj::Vertex *vertices;
    uint32 *indices;
    uint32 vertNum;
    uint32 indexNum;
  };

#define OGL_NAME (obj.renderer.driver)

  RendererObjSharedData::RendererObjSharedData(void) :
    vertices(NULL), indices(NULL), vertNum(0), indexNum(0) {}
  RendererObjSharedData::~RendererObjSharedData(void) {
    PF_SAFE_DELETE(vertices);
    PF_SAFE_DELETE(indices);
  }

  void RendererObjSharedData::upload(RendererObj &obj)
  {
    PF_MSG_V("RendererObj: creating OGL objects");

    // Create the index buffer
    const size_t indexSize = sizeof(GLuint) * this->indexNum;
    R_CALL (GenBuffers, 1, &obj.elementBuffer);
    R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, obj.elementBuffer);
    R_CALL (BufferData, GL_ELEMENT_ARRAY_BUFFER, indexSize, this->indices, GL_STATIC_DRAW);
    R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create the OGL vertex buffer
    const size_t vertexSize = this->vertNum * sizeof(Obj::Vertex);
    R_CALL (GenBuffers, 1, &obj.arrayBuffer);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, obj.arrayBuffer);
    R_CALL (BufferData, GL_ARRAY_BUFFER, vertexSize, this->vertices, GL_STATIC_DRAW);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);

    // Create the OGL vertex array
    R_CALL (GenVertexArrays, 1, &obj.vertexArray);
    R_CALL (BindVertexArray, obj.vertexArray);
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, obj.arrayBuffer);
    R_CALL (VertexAttribPointer, RendererDriver::ATTR_POSITION, 3,
            GL_FLOAT, GL_FALSE, sizeof(Obj::Vertex),
            NULL);
    R_CALL (VertexAttribPointer, RendererDriver::ATTR_TEXCOORD, 2,
            GL_FLOAT, GL_FALSE, sizeof(Obj::Vertex),
            (void*) offsetof(Obj::Vertex, t));
    R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
    R_CALL (EnableVertexAttribArray, RendererDriver::ATTR_POSITION);
    R_CALL (EnableVertexAttribArray, RendererDriver::ATTR_TEXCOORD);
    R_CALL (BindVertexArray, 0);
  }

  RendererObj::RendererObj(Renderer &renderer, const Obj &obj) :
    RendererDisplayable(renderer, RN_DISPLAYABLE_WAVEFRONT),
    sharedData(NULL), vertexArray(0), arrayBuffer(0), elementBuffer(0),
    properties(0)
  {
    TextureStreamer &streamer = *renderer.streamer;
    PF_MSG_V("RendererObj: asynchronously loading textures");

    // Map each material group to the texture name. Since we also remove the
    // possibly unused materials, we remap their IDs
    uint32 *matRemap = PF_NEW_ARRAY(uint32, obj.matNum);
    this->mat.resize(obj.grpNum);
    const uint32 matNum = obj.grpNum;
    for (size_t i = 0; i < matNum; ++i) {
      const int32 matID = obj.grp[i].m;
      PF_ASSERT(matID >= 0); // We ensure that in the loader
      matRemap[matID] = i;
      Material &material = this->mat[i];
      material.map_Kd = renderer.defaultTex;
      material.name_Kd = obj.mat[matID].map_Kd;
    }

    // Start to load the textures
    this->sharedData = PF_NEW(RendererObjSharedData);
    this->texLoading = PF_NEW(TaskLoadObjTexture, streamer, this, obj);
    this->texLoading->scheduled();

    // Create the rendererOBJ segment
    PF_MSG_V("RendererObj: creating geometry segments");
    RendererObjSharedData *shared = this->sharedData.cast<RendererObjSharedData>();
    shared->indices = RendererObjSegment(*this, obj);
    shared->indexNum = 3*obj.triNum;
    const uint32 segmentNum = this->segments.size();
    for (size_t segmentID = 0; segmentID < segmentNum; ++segmentID)
      segments[segmentID].matID = matRemap[segments[segmentID].matID];
    shared->vertices = PF_NEW_ARRAY(Obj::Vertex, obj.vertNum);
    shared->vertNum = obj.vertNum;
    std::memcpy(shared->vertices, obj.vert, obj.vertNum * sizeof(Obj::Vertex));
    PF_SAFE_DELETE_ARRAY(matRemap);
  }

  void RendererObj::onCompile(void) {
    if (this->properties & RN_OBJ_OCCLUDER) {
      PF_MSG_V("Game: building BVH");
      RendererObjSharedData *shared = (RendererObjSharedData*) sharedData.ptr;
      PF_ASSERT(shared->indexNum % 3 == 0);
      const uint32 triNum = shared->indexNum / 3;
      RTTriangle *tris = PF_NEW_ARRAY(RTTriangle, triNum);
      for (size_t index = 0; index < shared->indexNum; index += 3) {
        const uint32 index0 = shared->indices[index+0];
        const uint32 index1 = shared->indices[index+1];
        const uint32 index2 = shared->indices[index+2];
        const vec3f &v0 = shared->vertices[index0].p;
        const vec3f &v1 = shared->vertices[index1].p;
        const vec3f &v2 = shared->vertices[index2].p;
        tris[index / 3] = RTTriangle(v0,v1,v2);
      }
      BVH2<RTTriangle>* bvh = PF_NEW(BVH2<RTTriangle>);
      buildBVH2(tris, triNum, *bvh);
      PF_DELETE_ARRAY(tris);
      this->intersector = PF_NEW(BVH2Traverser<RTTriangle>, bvh);
    }
  }

  void RendererObj::onUnreferenced(void) {
    this->texLoading = NULL;
    this->sharedData = NULL;
  }

#undef OGL_NAME
#define OGL_NAME (this->renderer.driver)

  RendererObj::~RendererObj(void) {
    if (this->vertexArray)   R_CALL (DeleteVertexArrays, 1, &this->vertexArray);
    if (this->arrayBuffer)   R_CALL (DeleteBuffers, 1, &this->arrayBuffer);
    if (this->elementBuffer) R_CALL (DeleteBuffers, 1, &this->elementBuffer);
  }

  void RendererObj::display(const vector<uint32> &visible, uint32 visibleNum)
  {
    // Creating the OGL data if not done yet
    if (UNLIKELY(this->sharedData)) {
      RendererObjSharedData *shared = (RendererObjSharedData*) sharedData.ptr;
      shared->upload(*this);
      sharedData = NULL;
    }
    Lock<MutexSys> lock(mutex); // XXX remove that
    R_CALL (BindVertexArray, vertexArray);
    R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    R_CALL (ActiveTexture, GL_TEXTURE0);
    for (size_t visibleID = 0; visibleID < visibleNum; ++visibleID) {
      const uint32 segmentID = visible[visibleID];
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

