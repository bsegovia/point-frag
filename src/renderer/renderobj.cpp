#include "renderer/renderobj.hpp"
#include "renderer/renderer.hpp"

namespace pf
{
#if 0
  RenderObj::RenderObj(Renderer &renderer, const FileName &fileName)
  {
    obj = new Obj;
    for (size_t i = 0; i < defaultPathNum; ++i) {
      FATAL_IF (obj->load((dataPath + "f000.obj").c_str()) == false, "Loading failed");
    }

    // Load all textures
    for (size_t i = 0; i < obj->matNum; ++i) {
      const char *name = obj->mat[i].map_Kd;
      if (name == NULL || strlen(name) == 0)
        continue;
      if (texMap.find(name) == texMap.end()) {
        const std::string path = dataPath + name;
        Ref<Texture2D> tex = new Texture2D(*renderer, path);
        texMap[name] = tex->handle ? tex : chessTextureName;
      }
      else
        printf((std::string(name) + " already found\n").c_str());
    }

  }

  RenderObj::~RenderObj(void)
  {
  }
#endif
}

