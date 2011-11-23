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
#include "sys/logging.hpp"

#include <cstring>

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

  RendererObj::RendererObj(Renderer &renderer, const Obj &obj) :
    renderer(renderer), mat(NULL), sgmt(NULL), matNum(0), sgmtNum(0),
    vertexArray(0), arrayBuffer(0), elementBuffer(0)
  {
    TextureStreamer &streamer = *renderer.streamer;

    if (obj.isValid()) {
      PF_MSG_V("RendererObj: asynchronously loading textures");

      // Map each material group to the texture name
      this->mat = PF_NEW_ARRAY(Material, obj.grpNum);
      this->sgmt = PF_NEW_ARRAY(Segment, obj.grpNum);
      this->sgmtNum = this->matNum = obj.grpNum;
      for (size_t i = 0; i < obj.grpNum; ++i) {
        const int32 matID = obj.grp[i].m;
        FATAL_IF(matID < 0, "Face with no material TODO support it in the loader");
        Material &material = this->mat[i];
        material.map_Kd = renderer.defaultTex;
        material.name_Kd = obj.mat[matID].map_Kd;
      }

      // Start to load the textures
      this->texLoading = PF_NEW(TaskLoadObjTexture, streamer, *this, obj);
      this->texLoading->scheduled();

      // Right now we only create one segment per material
      PF_MSG_V("RendererObj: creating geometry segments");
      for (size_t grpID = 0; grpID < obj.grpNum; ++grpID) {
        const size_t first = obj.grp[grpID].first, last = obj.grp[grpID].last;
        Segment &segment = this->sgmt[grpID];
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

      // Create the vertex buffer
      PF_MSG_V("RendererObj: creating OGL objects");
      const size_t vertexSize = obj.vertNum * sizeof(Obj::Vertex);
      R_CALL (GenBuffers, 1, &this->arrayBuffer);
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->arrayBuffer);
      R_CALL (BufferData, GL_ARRAY_BUFFER, vertexSize, obj.vert, GL_STATIC_DRAW);
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);

      // Build the indices
      GLuint *indices = PF_NEW_ARRAY(GLuint, obj.triNum * 3);
      const size_t indexSize = sizeof(GLuint[3]) * obj.triNum;
      for (size_t from = 0, to = 0; from < obj.triNum; ++from, to += 3) {
        indices[to+0] = obj.tri[from].v[0];
        indices[to+1] = obj.tri[from].v[1];
        indices[to+2] = obj.tri[from].v[2];
      }
      R_CALL (GenBuffers, 1, &this->elementBuffer);
      R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, this->elementBuffer);
      R_CALL (BufferData, GL_ELEMENT_ARRAY_BUFFER, indexSize, indices, GL_STATIC_DRAW);
      R_CALL (BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
      PF_DELETE_ARRAY(indices);

      // Contain the vertex array
      R_CALL (GenVertexArrays, 1, &this->vertexArray);
      R_CALL (BindVertexArray, this->vertexArray);
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, this->arrayBuffer);
      R_CALL (VertexAttribPointer, RendererDriver::ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Obj::Vertex), NULL);
      R_CALL (VertexAttribPointer, RendererDriver::ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Obj::Vertex), (void*)offsetof(Obj::Vertex, t));
      R_CALL (BindBuffer, GL_ARRAY_BUFFER, 0);
      R_CALL (EnableVertexAttribArray, RendererDriver::ATTR_POSITION);
      R_CALL (EnableVertexAttribArray, RendererDriver::ATTR_TEXCOORD);
      R_CALL (BindVertexArray, 0);
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

