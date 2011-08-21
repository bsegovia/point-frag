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

#ifdef USE_OPENEXR

#include "image/image.hpp"

/* include OpenEXR headers */
#ifdef __WIN32__
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#pragma comment (lib, "Half.lib")
#pragma comment (lib, "Iex.lib")
#pragma comment (lib, "IlmImf.lib")
#pragma comment (lib, "IlmThread.lib")
#pragma comment (lib, "Imath.lib")
#pragma comment (lib, "zlib.lib")
#else
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#endif

namespace pf
{
  /*! load an EXR file from disk */
  Ref<Image> loadExr(const FileName& filename)
  {
    Imf::RgbaInputFile file (filename.c_str());
    Imath::Box2i dw = file.dataWindow();
    index_t width = dw.max.x - dw.min.x + 1;
    index_t height = dw.max.y - dw.min.y + 1;

    Imf::Array2D<Imf::Rgba> pixels(height, width);
    file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
    file.readPixels (dw.min.y, dw.max.y);

    Ref<Image> img = new Image3f(width,height,filename);

    if (file.lineOrder() == Imf::INCREASING_Y) {
      for (index_t y=0; y<height; y++) {
        for (index_t x=0; x<width; x++) {
          Imf::Rgba c = pixels[y][x];
          img->set(x,y,Col3f(c.r,c.g,c.b));
        }
      }
    }
    else {
      for (index_t y=0; y<height; y++) {
        for (index_t x=0; x<width; x++) {
          Imf::Rgba c = pixels[y][x];
          img->set(x,height-y-1,Col3f(c.r,c.g,c.b));
        }
      }
    }

    return img;
  }

  /*! store an EXR file to disk */
  void storeExr(const Ref<Image>& img, const FileName& filename)
  {
    Imf::Array2D<Imf::Rgba> pixels(img->height,img->width);
    for (size_t y=0; y<img->height; y++) {
      for (size_t x=0; x<img->width; x++) {
        Col3f c = img->get(x,y);
        pixels[y][x] = Imf::Rgba(c.r,c.g,c.b,1.0f);
      }
    }

    Imf::RgbaOutputFile file(filename.c_str(), img->width, img->height, Imf::WRITE_RGBA);
    file.setFrameBuffer(&pixels[0][0], 1, img->width);
    file.writePixels(img->height);
  }
}

#endif
