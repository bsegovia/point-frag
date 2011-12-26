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

#ifndef __PF_FONT_HPP__
#define __PF_FONT_HPP__

#include "sys/platform.hpp"
#include "sys/vector.hpp"
#include "sys/filename.hpp"

namespace pf
{
  /*! Bitmap font mostly as exported by Bitmap font generator that may be
   *  found here: http://www.angelcode.com/products/bmfont/
   */
  struct Font : public NonCopyable
  {
    /*! Load the font data from file */
    bool load(const FileName &fileName);

    /*! Holds information on how the font was generated */
    struct Info
    {
      Info(void);
      std::string face;    //!< This is the name of the true type font
      std::string charset; //!< The name of the OEM charset used (when not unicode)
      int16 size;          //!< The size of the true type font.
      int16 bold;          //!< 1 if bold
      int16 italic;        //!< 1 if italic
      int16 unicode;       //!< 1 if unicode char set
      int16 smooth;        //!< 1 if smoothing was turned on
      int16 stretchH;      //!< Font height stretch in percentage
      int16 aa;            //!< Sample per pixel (1 == no super sampling)
      int16 padding[4];    //!< Padding for each character (up, right, down, left)
      int16 spacing[2];    //!< Spacing for each character (horizontal, vertical)
      int16 outline;       //!< Outline thickness
      PF_STRUCT(Info);
    };

    /*! Holds information common to all characters */
    struct Common
    {
      Common(void);
      /*! 0 if channel holds glyph data, 1 if it holds outline, 2 if it holds
       *  glyph and outline, 3 if its set to zero, and 4 if its set to one.
       */
      int16 alphaChnl, redChnl, greenChnl, blueChnl;
      int16 lineHeight; //!< Distance in pixels between each line of text
      int16 base;       //!< Number of pixels from top of the line to base of characters
      int16 scaleW;     //!< Texture width
      int16 scaleH;     //!< Texture height
      int16 pages;      //!< Number of texture pages included in the font
      int16 packed;     //!< Set to 1 if the monochrome characters have been packed
      PF_STRUCT(Common);
    };

    /*! Page information */
    struct Page
    {
      Page(void);
      int16 id;         //!< Page ID
      std::string file; //!< Texture file name
      PF_STRUCT(Page);
    };

    /*! This tag describes on character in the font. There is one for each
     *  included character in the font
     */
    struct Char
    {
      Char(void);
      int32 id;       //!< Character ID
      int16 x;        //!< x origin
      int16 y;        //!< y origin
      int16 width;    //!< Width
      int16 height;   //!< Height
      int16 xoffset;  //!< How the position should be offset in screen
      int16 yoffset;  //!< How the position should be offset in screen
      int16 xadvance; //!< How much the current position should be advanced in the screen
      int16 page;     //!< Page ID
      int16 channel;  //!< 1=blue, 2=green, 4=red, 8=alpha, 15=all channels
      PF_STRUCT(Char);
    };

    /*! The kerning information is used to adjust the distance between certain
     *  characters, e.g. some characters should be placed closer to each other
     *  than others.
     */
    struct Kerning
    {
      Kerning(void);
      int16 first;
      int16 second;
      int16 amount;
      PF_STRUCT(Kerning);
    };

    Info info;
    Common common;
    vector<Char> chars;
    vector<Page> pages;
    vector<Kerning> kernings;
    PF_STRUCT(Font);
  };

} /* namespace pf */

#endif /* __PF_FONT_HPP__ */

