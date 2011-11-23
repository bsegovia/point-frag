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
#include "renderer/renderer.hpp"
#include "models/obj.hpp"
#include "rt/bvh2.hpp"
#include "rt/bvh2_node.hpp"
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
      assert(state.value == TextureState::COMPLETE);
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
        const TextureRequest req(texName[i], PF_TEX_FORMAT_DXT1);
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

  /*! In the case the user does not provide any BVH to segment the triangles,
   *  we simply take the groups as given by the OBJ loader. It retuns the
   *  indices to upload
   */
  static GLuint *RendererObjSegment(RendererObj &renderObj, const Obj &obj)
  {
    // Compute the bounding boxes of each segment
    renderObj.sgmt = PF_NEW_ARRAY(RendererObj::Segment, obj.grpNum);
    renderObj.sgmtNum = obj.grpNum;
    for (size_t grpID = 0; grpID < obj.grpNum; ++grpID) {
      const size_t first = obj.grp[grpID].first, last = obj.grp[grpID].last;
      RendererObj::Segment &segment = renderObj.sgmt[grpID];
      segment.first = first;
      segment.last = last;
      segment.bbox = BBox3f(empty);
      segment.matID = grpID;
      for (size_t j = first; j < last; ++j) {
        segment.bbox.grow(obj.vert[obj.tri[j].v[0]].p);
        segment.bbox.grow(obj.vert[obj.tri[j].v[1]].p);
        segment.bbox.grow(obj.vert[obj.tri[j].v[2]].p);
      }
    }

    // Just take the indices as they are returned by the parser
    GLuint *indices = PF_NEW_ARRAY(GLuint, obj.triNum * 3);
    for (size_t from = 0, to = 0; from < obj.triNum; ++from, to += 3) {
      indices[to+0] = obj.tri[from].v[0];
      indices[to+1] = obj.tri[from].v[1];
      indices[to+2] = obj.tri[from].v[2];
    }

    return indices;
  }

  /*! Sort the triangle ID according to their material */
  struct TriangeIDSorter {
    TriangeIDSorter(const Obj::Triangle *tri) : tri(tri) {}
    INLINE int operator() (const uint32 a, const uint32 b) const {
      return tri[a].m < tri[b].m;
    }
    const Obj::Triangle *tri;
  };

  /*! Helper to build the BVH on the renderer obj */
  class SegmentBVHBuilder
  {
  public:
    SegmentBVHBuilder(const Obj &obj, const BVH2<RTTriangle> &bvh);
    ~SegmentBVHBuilder(void);
    /*! Compute each node cost and number of primitives below each of them */
    void traverseBVH(uint32 nodeID = 0);
    /*! We cut the source tree to get the destination one */
    void cut(uint32 nodeID = 0, uint32 dstID = 0);
    /*! "Merge" the tree ie output it primitives in the merged array */
    void merge(uint32 nodeID);
    /*! Create the segments for the node. Return first segment index */
    uint32 makeSegments(uint32 dstID, uint32 first, uint32 n);
  private:
    BVH2<RendererObj::Segment> *dst;//!< This is what we want to build
    const Obj &obj;                 //!< Contains the triangles (with material)
    const BVH2<RTTriangle> &bvh;    //!< This guides the build of high level BVH
    float *cost;                    //!< Per node cost
    uint32 *primNum;                //!< Primitive number below each node
    uint32 *mergedID;               //!< Merged primitive IDs
    uint32 mergedNum;               //!< Number of primitives merged
    std::vector<BVH2Node> dstNode;  //!< Contains the destination nodes
    std::vector<RendererObj::Segment> segments; //!< The segments we are creating
    static const float traversalCost;   //!< Cost to traverse a node
    static const float intersectionCost;//!< Cost to "intersect" the triangles
  };

  const float SegmentBVHBuilder::traversalCost = 100.f;
  const float SegmentBVHBuilder::intersectionCost = 1.f;

  SegmentBVHBuilder::SegmentBVHBuilder(const Obj &obj, const BVH2<RTTriangle> &bvh) :
    obj(obj), bvh(bvh)
  {
    this->cost = PF_NEW_ARRAY(float, bvh.nodeNum);
    this->primNum = PF_NEW_ARRAY(uint32, bvh.nodeNum);
    this->mergedID = PF_NEW_ARRAY(uint32, bvh.primNum);
    this->dst = PF_NEW(BVH2<RendererObj::Segment>);
    this->mergedNum = 0;
  }

  SegmentBVHBuilder::~SegmentBVHBuilder(void) {
    PF_SAFE_DELETE_ARRAY(this->cost);
    PF_SAFE_DELETE_ARRAY(this->primNum);
    PF_SAFE_DELETE_ARRAY(this->mergedID);
  }

  static INLINE float halfArea(const BVH2Node &node) {
    const vec3f d = node.pmax - node.pmin;
    return d.x*(d.y+d.z)+d.y*d.z;
  }

  /*! Make the bbox bigger with the given triangle */
  static INLINE void growBBox(BBox3f &bbox, const Obj &obj, uint32 triID) {
    for (int v = 0; v < 3; ++v) {
      const int vertID = obj.tri[triID].v[v];
      bbox.grow(obj.vert[vertID].p);
    }
  }

  /*! Initialize a new segment. triID is the triangle index in the Obj
   *  structure and first is the ID of the same triangle in the sorted array of
   *  triangle indices
   */
  static INLINE void startSegment(RendererObj::Segment &segment,
                                  const Obj &obj,
                                  uint32 triID,
                                  uint32 first)
  {
    growBBox(segment.bbox, obj, triID);
    segment.first = first;
    segment.bbox = BBox3f(empty);
    segment.matID = obj.tri[triID].m; // Be aware, they must be remapped
  }

  /*! Just for symetry maniacs. We finish the segment by giving the last
   *  primitive index
   */
  static INLINE void endSegment(RendererObj::Segment &segment, uint32 last)
  {
    segment.last = last;
  }

  uint32 SegmentBVHBuilder::makeSegments(uint32 dstID, uint32 first, uint32 n)
  {
    const uint32 last = first + n - 1;
    const uint32 firstSegment = segments.size();
    std::sort(mergedID+first, mergedID+last+1, TriangeIDSorter(obj.tri));
    assert(n > 0);

    // Allocate the segments and build them
    uint32 segmentID = firstSegment;
    uint32 triID = mergedID[first];
    int32 matID = obj.tri[triID].m;
    segments.push_back(RendererObj::Segment());
    startSegment(segments[segmentID], obj, triID, first);

    // When a new material is encountered, we finish the current segment and
    // start a new one
    for (size_t i = first+1; i < first+n; ++i) {
      triID = mergedID[i];
      if (obj.tri[triID].m != matID) {
        // We are done with this segment
        endSegment(segments[segmentID], i-1);

        // Start the new one
        matID = obj.tri[triID].m;
        segmentID = segments.size();
        segments.push_back(RendererObj::Segment());
        startSegment(segments[segmentID], obj, triID, i);
      }
    }
    endSegment(segments[segmentID], last);

    // This is the ID of the first segment for the node we are building
    return firstSegment;
  }

  void SegmentBVHBuilder::traverseBVH(uint32 nodeID)
  {
    const BVH2Node &node = bvh.node[nodeID];
    if (node.isLeaf()) {
      const uint32 n = node.getPrimNum();
      cost[nodeID] = float(n) * intersectionCost + traversalCost;
      primNum[nodeID] = node.getPrimNum();
    } else {
      const uint32 left = node.getOffset(); // left child ID
      const uint32 right = left + 1;        // right child ID
      traverseBVH(left);
      traverseBVH(right);
      const BVH2Node &leftChild  = bvh.node[left];
      const BVH2Node &rightChild = bvh.node[right];
      const float nodeArea  = halfArea(node);
      const float leftArea  = halfArea(leftChild);
      const float rightArea = halfArea(rightChild);
      const float leftCost  = cost[left];
      const float rightCost = cost[right];
      const float childCost = (leftArea*leftCost+rightArea*rightCost)/nodeArea;
      cost[nodeID] = traversalCost + childCost;
      primNum[nodeID] = primNum[left] + primNum[right];
    }
  }

  void SegmentBVHBuilder::merge(uint32 nodeID)
  {
    const BVH2Node &node = bvh.node[nodeID];
    if (node.isLeaf()) {
      const uint32 *IDs = bvh.primID + node.getPrimID();
      const uint32 n = node.getPrimNum();
      for (size_t prim = 0; prim < n; ++prim)
        mergedID[mergedNum++] = IDs[prim];
    } else {
      merge(node.getOffset());
      merge(node.getOffset()+1);
    }
  }

  void SegmentBVHBuilder::cut(uint32 nodeID, uint32 dstID)
  {
    const BVH2Node &node = bvh.node[nodeID];
    // Leaf means that we are done. We do not have any other choice that
    // making a segment with that
    if (node.isLeaf()) {
      dstNode[dstID] = node;
      const uint32 beforeMerge = mergedNum;
      merge(node.getOffset());
      const uint32 afterMerge = mergedNum;
      const uint32 n = afterMerge - beforeMerge;
      const uint32 primID = makeSegments(dstID, beforeMerge, n);
      dstNode[dstID].setPrimID(primID);
    }
    // Non-leaf gives two choices. Either we cut here and we merge the child
    // nodes to create the segments or we recurse to create the segment in
    // lower levels. We use the computed cost for that
    else {
      const float actualCost = cost[nodeID];
      const float cutCost = traversalCost + intersectionCost * primNum[nodeID];
      // Too expensive to cut here, we recurse
      if (actualCost < cutCost) {
        const uint32 leftID  = dstNode.size();
        const uint32 rightID = dstNode.size() + 1;
        dstNode.push_back(BVH2Node());
        dstNode.push_back(BVH2Node());
        dstNode[dstID] = node;
        dstNode[dstID].setOffset(leftID);
        cut(node.getOffset(),   leftID);
        cut(node.getOffset()+1, rightID);
      }
      // OK, we create the segments here
      else {
        dstNode[dstID] = node;
        dstNode[dstID].setAsLeaf();
        const uint32 beforeMerge = mergedNum;
        merge(node.getOffset());
        merge(node.getOffset()+1);
        const uint32 afterMerge = mergedNum;
        const uint32 n = afterMerge - beforeMerge;
        const uint32 primID = makeSegments(dstID, beforeMerge, n);
        dstNode[dstID].setPrimID(primID);
      }
    }
  }

  // XXX remove this dependency by distinguishing between BVH and intersectabe
  template <> void BVH2<RendererObj::Segment>::traverse(const Ray &ray, Hit &hit) const {}
  template <> void BVH2<RendererObj::Segment>::traverse(const RayPacket &pckt, PacketHit &hit) const {}
  template <> bool BVH2<RendererObj::Segment>::occluded(const Ray &ray) const { return false; }
  template <> void BVH2<RendererObj::Segment>::occluded(const RayPacket &pckt, PacketOcclusion &o) const {}

#if 0
  /*! In the case the user provides a BVH for the OBJ, we use it to segment
   *  the geometry in a useful way. This will allow us to cull the geometry
   *  using the ray-traced z buffer
   */
  static GLuint *RendererObjSegmentFromBVH(RendererObj &renderObj,
                                           const Obj &obj,
                                           const BVH2<RTTriangle> &bvh)
  {
    SegmentBVHBuilder builder(obj, bvh);

    // Compute node cost and primimite number per node
    builder.traverseBVH();

    // Cut the tree and compute the segments
    builder.cut();

    return NULL;
  }
#endif

  RendererObj::RendererObj(Renderer &renderer, const Obj &obj, const BVH2<RTTriangle> *bvh) :
    renderer(renderer), mat(NULL), sgmt(NULL), matNum(0), sgmtNum(0),
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
        assert(matID >= 0); // We ensure that in the loader
        matRemap[matID] = i;
        Material &material = this->mat[i];
        material.map_Kd = renderer.defaultTex;
        material.name_Kd = obj.mat[matID].map_Kd;
      }

      // Start to load the textures
      this->texLoading = PF_NEW(TaskLoadObjTexture, streamer, *this, obj);
      this->texLoading->scheduled();

      // Right now we only create one segment per material
      PF_MSG_V("RendererObj: creating geometry segments");
      GLuint *indices = RendererObjSegment(*this, obj);

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
    PF_SAFE_DELETE_ARRAY(this->sgmt);
    PF_SAFE_DELETE_ARRAY(this->mat);
  }

  void RendererObj::display(void) {
    Lock<MutexSys> lock(mutex);
    R_CALL (BindVertexArray, vertexArray);
    R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
    R_CALL (ActiveTexture, GL_TEXTURE0);
    for (size_t sgmtID = 0; sgmtID < sgmtNum; ++sgmtID) {
      const Segment &segment = sgmt[sgmtID];
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

