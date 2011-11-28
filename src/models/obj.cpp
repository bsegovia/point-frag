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

#include "obj.hpp"
#include "sys/platform.hpp"
#include "sys/alloc.hpp"
#include "sys/logging.hpp"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <map>
#include <vector>
#include <algorithm>

#if defined (__WIN32__)
#define strtok_r(a,b,c) strtok_s(a,b,c)
#endif

namespace pf
{
  struct ObjLoaderVec;
  struct ObjLoaderFace;
  struct ObjLoaderMat;

  /*! Helper to load OBJ wavefront files */
  struct ObjLoader
  {
    ObjLoader(void) : objSaved(NULL), mtlSaved(NULL) {}
    /*! Load the obj model */
    bool loadObj(const char *fileName);
    /*! Load the mtllib file */
    bool loadMtl(const char *mtlFileName, const char *objFileName);
    /*! Find a material by name */
    int findMaterial(const char *name);
    /*! Parse and return a new face */
    ObjLoaderFace* parseFace(void);
    /*! Parse and return a new vector */
    ObjLoaderVec* parseVector(void);
    /*! Get the vertex indices from the face */
    void parseVertexIndex(ObjLoaderFace &face);
    /*! Get a valid index from the lists (-1 means invalid) */
    INLINE int getListIndex(int currMax, int index) {
      if (index == 0) return -1;
      if (index < 0)  return currMax + index;
      return index - 1;
    }
    /*! Get valid indices from the lists (-1 means invalid) */
    void getListIndexV(int current_max, int *indices) {
      for (int i = 0; i < MAX_VERT_NUM; i++)
        indices[i] = this->getListIndex(current_max, indices[i]);
    }
    std::vector<ObjLoaderVec*>  vertexList;      //!< All positions parsed
    std::vector<ObjLoaderVec*>  normalList;      //!< All normals parsed
    std::vector<ObjLoaderVec*>  textureList;     //!< All textures parsed
    std::vector<ObjLoaderFace*> faceList;        //!< All faces parsed
    std::vector<ObjLoaderMat*>  materialList;    //!< All materials parsed
    GrowingPool<ObjLoaderFace>  faceAllocator;   //!< Speeds up face allocation
    GrowingPool<ObjLoaderVec>   vectorAllocator; //!< Speeds up vector allocation
    GrowingPool<ObjLoaderMat>   matAllocator;    //!< Speeds up material allocation
    char *objSaved, *mtlSaved;                   //!< for strtok_r
    static const char *whiteSpace;               //!< White space characters
    static const int FILENAME_SZ = 1024;
    static const int MAT_NAME_SZ = 1024;
    static const int LINE_SZ = 4096;
    static const int MAX_VERT_NUM = 4;
  };
  const char *ObjLoader::whiteSpace = " \t\n\r";

#define DECL_FAST_ALLOC(ALLOCATOR)                            \
  INLINE void *operator new (size_t sz, ObjLoader &loader) {  \
    return loader.ALLOCATOR.allocate();                       \
  }
  /*! Face as parsed by the OBJ loader */
  struct ObjLoaderFace {
    DECL_FAST_ALLOC(faceAllocator);
    int vertexID[ObjLoader::MAX_VERT_NUM];
    int normalID[ObjLoader::MAX_VERT_NUM];
    int textureID[ObjLoader::MAX_VERT_NUM];
    int vertexNum;
    int materialID;
  };

  /*! Vector as parsed by the OBJ loader */
  struct ObjLoaderVec {
    DECL_FAST_ALLOC(vectorAllocator);
    double e[3];
  };

  /*! Material as parsed by the OBJ loader */
  struct ObjLoaderMat {
    DECL_FAST_ALLOC(matAllocator);
    INLINE void setDefault(void);
    char name[ObjLoader::MAT_NAME_SZ];
    char map_Ka[ObjLoader::FILENAME_SZ];
    char map_Kd[ObjLoader::FILENAME_SZ];
    char map_D[ObjLoader::FILENAME_SZ];
    char map_Bump[ObjLoader::FILENAME_SZ];
    double amb[3], diff[3], spec[3];
    double km;
    double reflect;
    double refract;
    double trans;
    double shiny;
    double glossy;
    double refract_index;
  };
#undef DECL_FAST_ALLOC

  static bool INLINE strequal(const char *s1, const char *s2) {
    if (strcmp(s1, s2) == 0) return true;
    return false;
  }

  static bool INLINE contains(const char *haystack, const char *needle) {
    if (strstr(haystack, needle) == NULL) return false;
    return true;
  }

  void ObjLoaderMat::setDefault(void) {
    this->map_Ka[0] = this->map_Kd[0] = this->map_D[0] = this->map_Bump[0] = '\0';
    this->amb[0]  = this->amb[1]  = this->amb[2]  = 0.2;
    this->diff[0] = this->diff[1] = this->diff[2] = 0.8;
    this->spec[0] = this->spec[1] = this->spec[2] = 1.0;
    this->reflect = 0.0;
    this->trans = 1.;
    this->glossy = 98.;
    this->shiny = 0.;
    this->refract_index = 1.;
  }

  void ObjLoader::parseVertexIndex(ObjLoaderFace &face) {
    char **saved = &this->objSaved;
    char *temp_str = NULL, *token = NULL;
    int vertexNum = 0;

    while ((token = strtok_r(NULL, whiteSpace, saved)) != NULL) {
      if (face.textureID != NULL) face.textureID[vertexNum] = 0;
      if (face.normalID != NULL)  face.normalID[vertexNum] = 0;
      face.vertexID[vertexNum] = atoi(token);

      if (contains(token, "//")) {
        temp_str = strchr(token, '/');
        temp_str++;
        face.normalID[vertexNum] = atoi(++temp_str);
      } else if (contains(token, "/")) {
        temp_str = strchr(token, '/');
        face.textureID[vertexNum] = atoi(++temp_str);
        if (contains(temp_str, "/")) {
          temp_str = strchr(temp_str, '/');
          face.normalID[vertexNum] = atoi(++temp_str);
        }
      }
      vertexNum++;
    }
    face.vertexNum = vertexNum;
  }

  int ObjLoader::findMaterial(const char *name) {
    for (size_t i = 0; i < materialList.size(); ++i)
      if (strequal(name, materialList[i]->name)) return i;
    return -1;
  }

  ObjLoaderFace* ObjLoader::parseFace(void) {
    ObjLoaderFace *face = new (*this) ObjLoaderFace;
    this->parseVertexIndex(*face);
    this->getListIndexV(vertexList.size(), face->vertexID);
    this->getListIndexV(textureList.size(), face->textureID);
    this->getListIndexV(normalList.size(), face->normalID);
    return face;
  }

  ObjLoaderVec* ObjLoader::parseVector(void) {
    ObjLoaderVec *v = new (*this) ObjLoaderVec;
    v->e[0] = v->e[1] = v->e[2] = 0.;
    for (int i = 0; i < 3; ++i) {
      const char * str = strtok_r(NULL, whiteSpace, &this->objSaved);
      if (str == NULL)
        break;
      v->e[i] = atof(str);
    }
    return v;
  }

  bool ObjLoader::loadMtl(const char *mtlFileName, const char *objFileName)
  {
    char *tok = NULL;
    FILE *mtlFile = NULL;
    ObjLoaderMat *currMat = NULL;
    char currLine[LINE_SZ];
    int lineNumber = 0;
    char material_open = 0;

    // Try to get directly from the provided file name
    FileName lastLocation = mtlFileName;
    mtlFile = fopen(mtlFileName, "r");

    // If failed try to get it from where the obj directory
    const FileName objName(objFileName);
    const FileName mtlName(mtlFileName);
    if (mtlFile == NULL) {
      lastLocation = objName.path() + mtlName.base();
      mtlFile = fopen(lastLocation.c_str(), "r");
    }

    // Try to get it an obj sub-directory
    if (mtlFile == NULL) {
      lastLocation = objName.path() + mtlName;
      mtlFile = fopen(lastLocation.c_str(), "r");
    }

    // We were not able to find it
    if (mtlFile == NULL) return false;

    // Parse it
    while (fgets(currLine, LINE_SZ, mtlFile)) {
      char **saved = &this->mtlSaved;
      tok = strtok_r(currLine, " \t\n\r", saved);
      lineNumber++;

      // skip comments
      if (tok == NULL || strequal(tok, "//") || strequal(tok, "#"))
        continue;
      // start material
      else if (strequal(tok, "newmtl")) {
        material_open = 1;
        currMat = new (*this) ObjLoaderMat;
        currMat->setDefault();
        strncpy(currMat->name, strtok_r(NULL, whiteSpace, saved), MAT_NAME_SZ);
        materialList.push_back(currMat);
      }
      // ambient
      else if (strequal(tok, "Ka") && material_open) {
        currMat->amb[0] = atof(strtok_r(NULL, " \t", saved));
        currMat->amb[1] = atof(strtok_r(NULL, " \t", saved));
        currMat->amb[2] = atof(strtok_r(NULL, " \t", saved));
      }
      // diff
      else if (strequal(tok, "Kd") && material_open) {
        currMat->diff[0] = atof(strtok_r(NULL, " \t", saved));
        currMat->diff[1] = atof(strtok_r(NULL, " \t", saved));
        currMat->diff[2] = atof(strtok_r(NULL, " \t", saved));
      }
      // specular
      else if (strequal(tok, "Ks") && material_open) {
        currMat->spec[0] = atof(strtok_r(NULL, " \t", saved));
        currMat->spec[1] = atof(strtok_r(NULL, " \t", saved));
        currMat->spec[2] = atof(strtok_r(NULL, " \t", saved));
      }
      // shiny
      else if (strequal(tok, "Ns") && material_open)
        currMat->shiny = atof(strtok_r(NULL, " \t", saved));
      // map_Km
      else if (strequal(tok, "Km") && material_open)
        currMat->km = atof(strtok_r(NULL, " \t", saved));
      // transparent
      else if (strequal(tok, "d") && material_open)
        currMat->trans = atof(strtok_r(NULL, " \t", saved));
      // reflection
      else if (strequal(tok, "r") && material_open)
        currMat->reflect = atof(strtok_r(NULL, " \t", saved));
      // glossy
      else if (strequal(tok, "sharpness") && material_open)
        currMat->glossy = atof(strtok_r(NULL, " \t", saved));
      // refract index
      else if (strequal(tok, "Ni") && material_open)
        currMat->refract_index = atof(strtok_r(NULL, " \t", saved));
      // map_Ka
      else if (strequal(tok, "map_Ka") && material_open)
        strncpy(currMat->map_Ka, strtok_r(NULL, " \"\t\r\n", saved), FILENAME_SZ);
      // map_Kd
      else if (strequal(tok, "map_Kd") && material_open)
        strncpy(currMat->map_Kd, strtok_r(NULL, " \"\t\r\n", saved), FILENAME_SZ);
      // map_D
      else if (strequal(tok, "map_D") && material_open)
        strncpy(currMat->map_D, strtok_r(NULL, " \"\t\r\n", saved), FILENAME_SZ);
      // map_Bump
      else if (strequal(tok, "map_Bump") && material_open)
        strncpy(currMat->map_Bump, strtok_r(NULL, " \"\t\r\n", saved), FILENAME_SZ);
      // illumination type
      else if (strequal(tok, "illum") && material_open) { }
      else
        PF_ERROR_V("ObjLoader: Unknown command : " << tok <<
                   "in material file " << mtlFileName <<
                   "at line " << lineNumber <<
                   ", \"" << currLine << "\"");
    }

    fclose(mtlFile);
    return true;
  }

  bool ObjLoader::loadObj(const char *fileName)
  {
    FILE* objFile;
    int currMaterial = -1; 
    char *tok = NULL;
    char currLine[LINE_SZ];
    int lineNumber = 0;

    // open scene
    objFile = fopen(fileName, "r");
    if (objFile == NULL) {
      PF_MSG("ObjLoader: error reading file: " << fileName);
      return false;
    }

    // parser loop
    while (fgets(currLine, LINE_SZ, objFile)) {
      char **saved = &this->objSaved;
      tok = strtok_r(currLine, " \t\n\r", saved);
      lineNumber++;

      // skip comments
      if (tok == NULL || tok[0] == '#')
        continue;

      // parse objects
      else if (strequal(tok, "v"))    // vertex
        vertexList.push_back(this->parseVector());
      else if (strequal(tok, "vn"))   // vertex normal
        normalList.push_back(this->parseVector());
      else if (strequal(tok, "vt"))   // vertex texture
        textureList.push_back(this->parseVector());
      else if (strequal(tok, "f")) {  // face
        ObjLoaderFace *face = this->parseFace();
        face->materialID = currMaterial;
        faceList.push_back(face);
      }
      else if (strequal(tok, "usemtl"))   // usemtl
        currMaterial = this->findMaterial(strtok_r(NULL, whiteSpace, saved));
      else if (strequal(tok, "mtllib")) { //  mtllib
        const char *mtlFileName = strtok_r(NULL, whiteSpace, saved);
        if (this->loadMtl(mtlFileName, fileName) == 0) {
          PF_ERROR_V("ObjLoader: Error loading " << mtlFileName);
          return false;
        }
        continue;
      }
      else if (strequal(tok, "p")) {} // point
      else if (strequal(tok, "o")) {} // object name
      else if (strequal(tok, "s")) {} // smoothing
      else if (strequal(tok, "g")) {} // group
      else
        PF_ERROR_V("ObjLoader: Unknown command : " << tok <<
                   "in obj file " << fileName <<
                   "at line " << lineNumber <<
                   ", \"" << currLine << "\"");
    }

    fclose(objFile);
    return true;
  }

  static inline bool cmp(Obj::Triangle t0, Obj::Triangle t1) {return t0.m < t1.m;}

  Obj::Obj(void) { std::memset(this, 0, sizeof(Obj)); }

  INLINE uint32 str_hash(const char *key) {
    uint32 h = 5381;
    for (size_t i = 0, k; (k = key[i]); i++) h = ((h<<5)+h)^k; // bernstein k=33 xor
    return h;
  }

  template <typename T>
  INLINE uint32 hash(const T &elem) {
    const char *key = (const char *) &elem;
    uint32 h = 5381;
    for (size_t i = 0; i < sizeof(T); i++) h = ((h<<5)+h)^key[i];
    return h;
  }

  /*! Sort vertices faces */
  struct VertexKey {
    INLINE VertexKey(int p_, int n_, int t_) : p(p_), n(n_), t(t_) {}
    bool operator == (const VertexKey &other) const {
      return (p == other.p) && (n == other.n) && (t == other.t);
    }
    bool operator < (const VertexKey &other) const {
      if (p != other.p) return p < other.p;
      if (n != other.n) return n < other.n;
      if (t != other.t) return t < other.t;
      return false;
    }
    int p,n,t;
  };

  /*! Store face connectivity */
  struct Poly { int v[4]; int mat; int n; };

  bool Obj::load(const FileName &fileName)
  {
    ObjLoader loader;
    std::map<VertexKey, int> map;
    std::vector<Poly> polys;
    if (loader.loadObj(fileName.c_str()) == 0) return false;

    int vert_n = 0;
    for (size_t i = 0; i < loader.faceList.size(); ++i) {
      const ObjLoaderFace *face = loader.faceList[i];
      if (face == NULL) {
        PF_MSG_V("ObjLoader: NULL face for " << fileName.str());
        return false;
      }
      if (face->vertexNum > 4) {
        PF_MSG_V("ObjLoader: Too many vertices for " << fileName.str());
        return false;
      }
      int v[] = {0,0,0,0};
      for (int j = 0; j < face->vertexNum; ++j) {
        const int p = face->vertexID[j];
        const int n = face->normalID[j];
        const int t = face->textureID[j];
        const VertexKey key(p,n,t);
        const auto it = map.find(key);
        if (it == map.end())
          v[j] = map[key] = vert_n++;
        else
          v[j] = it->second;
      }
      const Poly p = {{v[0],v[1],v[2],v[3]},face->materialID,face->vertexNum};
      polys.push_back(p);
    }

    // No face defined
    if (polys.size() == 0) return true;

    // Create triangles now
    std::vector<Triangle> tris;
    for (auto poly = polys.begin(); poly != polys.end(); ++poly) {
      if (poly->n == 3) {
        const Triangle tri(vec3i(poly->v[0], poly->v[1], poly->v[2]), poly->mat);
        tris.push_back(tri);
      } else {
        const Triangle tri0(vec3i(poly->v[0], poly->v[1], poly->v[2]), poly->mat);
        const Triangle tri1(vec3i(poly->v[0], poly->v[2], poly->v[3]), poly->mat);
        tris.push_back(tri0);
        tris.push_back(tri1);
      }
    }

    // Sort them by material and save the material group
    std::sort(tris.begin(), tris.end(), cmp);
    std::vector<MatGroup> matGrp;
    int curr = tris[0].m;
    matGrp.push_back(MatGroup(0,0,curr));
    for (size_t i = 0; i < tris.size(); ++i) {
      if (tris[i].m != curr) {
        curr = tris[i].m;
        matGrp.back().last = (int) (i-1);
        matGrp.push_back(MatGroup((int)i,0,curr));
      }
    }
    matGrp.back().last = tris.size() - 1;

    // We replace the undefined material by the default one if needed
    if (tris[0].m == -1) {
      ObjLoaderMat *mat = new (loader) ObjLoaderMat;
      mat->setDefault();
      loader.materialList.push_back(mat);

      // First path the triangle
      const size_t matID = loader.materialList.size() - 1;
      for (size_t i = 0; i < tris.size(); ++i)
        if (tris[i].m != -1)
          break;
        else
          tris[i].m = matID;

      // Then, their group
      assert(matGrp[0].m == -1);
      matGrp[0].m = matID;
    }

    // Create all the vertices and store them
    const size_t vertNum = map.size();
    std::vector<Vertex> verts;
    verts.resize(vertNum);
    bool allPositionSet = true, allNormalSet = true, allTexCoordSet = true;
    for (auto it = map.begin(); it != map.end(); ++it) {
      const VertexKey src = it->first;
      const int dst = it->second;
      Vertex &v = verts[dst];

      // Get the position if specified
      if (src.p != -1) {
        const ObjLoaderVec *p = loader.vertexList[src.p];
        v.p[0] = float(p->e[0]);
        v.p[1] = float(p->e[1]);
        v.p[2] = float(p->e[2]);
      } else {
        v.p[0] = v.p[1] = v.p[2] = 0.f;
        allPositionSet = false;
      }

      // Get the normal if specified
      if (src.n != -1) {
        const ObjLoaderVec *n = loader.normalList[src.n];
        v.n[0] = float(n->e[0]);
        v.n[1] = float(n->e[1]);
        v.n[2] = float(n->e[2]);
      } else {
        v.n[0] = v.n[1] = v.n[2] = 1.f;
        allNormalSet = false;
      }

      // Get the texture coordinates if specified
      if (src.t != -1) {
        const ObjLoaderVec *t = loader.textureList[src.t];
        v.t[0] = float(t->e[0]);
        v.t[1] = float(t->e[1]);
      } else {
        v.t[0] = v.t[1] = 0.f;
        allTexCoordSet = false;
      }
    }

    if (allPositionSet == false)
      PF_WARNING_V("ObjLoader: some positions are unspecified for " << fileName.str());
    if (allNormalSet == false)
      PF_WARNING_V("ObjLoader: some normals are unspecified for " << fileName.str());
    if (allTexCoordSet == false)
      PF_WARNING_V("ObjLoader: some texture coordinates are unspecified for " << fileName.str());

    // Allocate the ObjMaterial
    Material *mat = PF_NEW_ARRAY(Material, loader.materialList.size());
    std::memset(mat, 0, sizeof(Material) * loader.materialList.size());

#define COPY_FIELD(NAME)                           \
    if (from.NAME) {                               \
      const size_t len = std::strlen(from.NAME);   \
      to.NAME = PF_NEW_ARRAY(char, len + 1);       \
      if (len) memcpy(to.NAME, from.NAME, len);    \
      to.NAME[len] = 0;                            \
    }
    for (size_t i = 0; i < loader.materialList.size(); ++i) {
      const ObjLoaderMat &from = *loader.materialList[i];
      assert(loader.materialList[i] != NULL);
      Material &to = mat[i];
      COPY_FIELD(name);
      COPY_FIELD(map_Ka);
      COPY_FIELD(map_Kd);
      COPY_FIELD(map_D);
      COPY_FIELD(map_Bump);
    }
#undef COPY_FIELD

    // Now return the properly allocated Obj
    std::memset(this, 0, sizeof(Obj));
    this->triNum = tris.size();
    this->vertNum = verts.size();
    this->grpNum = matGrp.size();
    this->matNum = loader.materialList.size();
    if (this->triNum) {
      this->tri = PF_NEW_ARRAY(Triangle, this->triNum);
      std::memcpy(this->tri, &tris[0],  sizeof(Triangle) * this->triNum);
    }
    if (this->vertNum) {
      this->vert = PF_NEW_ARRAY(Vertex, this->vertNum);
      std::memcpy(this->vert, &verts[0], sizeof(Vertex) * this->vertNum);
    }
    if (this->grpNum) {
      this->grp = PF_NEW_ARRAY(MatGroup, this->grpNum);
      std::memcpy(this->grp,  &matGrp[0],  sizeof(MatGroup) * this->grpNum);
    }
    this->mat = mat;
    PF_MSG_V("ObjLoader: " << fileName.str() << ", " << triNum << " triangles");
    PF_MSG_V("ObjLoader: " << fileName.str() << ", " << vertNum << " vertices");
    PF_MSG_V("ObjLoader: " << fileName.str() << ", " << grpNum << " groups");
    PF_MSG_V("ObjLoader: " << fileName.str() << ", " << matNum << " materials");
    return true;
  }

  Obj::~Obj(void) {
    PF_SAFE_DELETE_ARRAY(this->tri);
    PF_SAFE_DELETE_ARRAY(this->vert);
    PF_SAFE_DELETE_ARRAY(this->grp);
    for (size_t i = 0; i < this->matNum; ++i) {
      Material &mat = this->mat[i];
      PF_SAFE_DELETE_ARRAY(mat.name);
      PF_SAFE_DELETE_ARRAY(mat.map_Ka);
      PF_SAFE_DELETE_ARRAY(mat.map_Kd);
      PF_SAFE_DELETE_ARRAY(mat.map_D);
      PF_SAFE_DELETE_ARRAY(mat.map_Bump);
    }
    PF_SAFE_DELETE_ARRAY(this->mat);
  }
}

#if defined (__WIN32__)
#undef strtok_r
#endif
