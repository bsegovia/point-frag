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

#include "font.hpp"
#include "sys/filename.hpp"
#include "sys/vector.hpp"
#include "sys/logging.hpp"
#include "sys/string.hpp"
#include "math/math.hpp"

#include <cstdio>
#include <cstring>

namespace pf
{

  Font::Page::Page(void) { this->id = 0; }
  Font::Char::Char(void) { std::memset(this, 0, sizeof(Char)); }
  Font::Common::Common(void) { std::memset(this, 0, sizeof(Common)); }
  Font::Kerning::Kerning(void) { std::memset(this, 0, sizeof(Kerning)); }
  Font::Info::Info(void) {
    for (size_t i = 0; i < 4; ++i) this->padding[i] = 0;
    for (size_t i = 0; i < 2; ++i) this->spacing[i] = 0;
    this->size = 0;
    this->bold = 0;
    this->italic = 0;
    this->unicode = 0;
    this->smooth = 0;
    this->stretchH = 0;
    this->aa = 0;
    this->outline = 0;
  }

  bool Font::load(const FileName &fileName)
  {
    vector<Char> tchars;      // We push the chars here, regardless the order
    vector<Kerning> tkernings;// we use a vector since this is resizable
    const int LINE_SZ = 4096;
    char currLine[LINE_SZ];
    int32 lineno = 0, maxID = 0;
    bool isLoaded = false;

    // Open font file
    FILE *fontFile = fopen(fileName.c_str(), "r");
    if (fontFile == NULL) {
      PF_MSG("Font Loader: error reading file: " << fileName);
      goto error;
    }

#define SEPARATOR " \t\n\r,\"="

#define NEXT_TOKEN(TOK)                                   \
  if ((TOK = tokenize(NULL, SEPARATOR, &saved)) == NULL)  \
    break;

#define GET_FIELD(F,N,A) if (strequal(tok, #N)) F = A

    // Parser loop
    while (fgets(currLine, LINE_SZ, fontFile)) {
      char *saved = NULL, *tok = NULL, *value = NULL;
      tok = tokenize(currLine, SEPARATOR, &saved);
      lineno++;

      // info gives data about how the font has been generated
      if (strequal(tok, "info")) {
        for (;;) {
          NEXT_TOKEN(tok);
          NEXT_TOKEN(value);
          GET_FIELD(this->info.face, face, value);
          else GET_FIELD(this->info.charset, charset, value);
          else GET_FIELD(this->info.size, size, atoi(value));
          else GET_FIELD(this->info.bold, bold, atoi(value));
          else GET_FIELD(this->info.italic, italic, atoi(value));
          else GET_FIELD(this->info.unicode, unicode, atoi(value));
          else GET_FIELD(this->info.stretchH, stretchH, atoi(value));
          else GET_FIELD(this->info.smooth, smooth, atoi(value));
          else GET_FIELD(this->info.aa, aa, atoi(value));
          else GET_FIELD(this->info.outline, outline, atoi(value));
          else if (strequal(tok, "padding")) {
            this->info.padding[0] = atoi(value); NEXT_TOKEN(value);
            this->info.padding[1] = atoi(value); NEXT_TOKEN(value);
            this->info.padding[2] = atoi(value); NEXT_TOKEN(value);
            this->info.padding[3] = atoi(value);
          }
          else if (strequal(tok, "spacing")) {
            this->info.spacing[0] = atoi(value); NEXT_TOKEN(value);
            this->info.spacing[1] = atoi(value);
          }
        }
      }
      // info gives data about how the font has been generated
      else if (strequal(tok, "common")) {
        for (;;) {
          NEXT_TOKEN(tok);
          NEXT_TOKEN(value);
          GET_FIELD(this->common.lineHeight, lineHeight, atoi(value));
          else GET_FIELD(this->common.base, base, atoi(value));
          else GET_FIELD(this->common.scaleW, scaleW, atoi(value));
          else GET_FIELD(this->common.scaleH, scaleH, atoi(value));
          else GET_FIELD(this->common.pages, pages, atoi(value));
          else GET_FIELD(this->common.packed, packed, atoi(value));
          else GET_FIELD(this->common.alphaChnl, alphaChnl, atoi(value));
          else GET_FIELD(this->common.redChnl, redChnl, atoi(value));
          else GET_FIELD(this->common.greenChnl, greenChnl, atoi(value));
          else GET_FIELD(this->common.blueChnl, blueChnl, atoi(value));
        }
      }
      // page gives data about the textures where the fonts are stored
      else if (strequal(tok, "page")) {
        if (UNLIKELY(pages.size() == 1)) {
          PF_MSG_V("Only one texture per font is supported");
          goto error;
        } else {
          pages.resize(1);
          Page &page = pages[0];
          for (;;) {
            NEXT_TOKEN(tok);
            NEXT_TOKEN(value);
            GET_FIELD(page.id, id, atoi(value));
            else GET_FIELD(page.file, file, value);
          }
        }
      }
      // char gives data about one particular character
      else if (strequal(tok, "char")) {
        Char ch;
        for (;;) {
          NEXT_TOKEN(tok);
          NEXT_TOKEN(value);
          GET_FIELD(ch.id, id, atoi(value));
          else GET_FIELD(ch.x, x, atoi(value));
          else GET_FIELD(ch.y, y, atoi(value));
          else GET_FIELD(ch.width, width, atoi(value));
          else GET_FIELD(ch.height, height, atoi(value));
          else GET_FIELD(ch.xoffset, xoffset, atoi(value));
          else GET_FIELD(ch.yoffset, yoffset, atoi(value));
          else GET_FIELD(ch.xadvance, xadvance, atoi(value));
          else GET_FIELD(ch.page, page, atoi(value));
          else GET_FIELD(ch.channel, channel, atoi(value));
        }
        tchars.push_back(ch);
      }

      // kerning gives data about how to offset characters for nice display
      else if (strequal(tok, "kerning")) {
        Kerning kerning;
        for (;;) {
          NEXT_TOKEN(tok);
          NEXT_TOKEN(value);
          GET_FIELD(kerning.first, first, atoi(value));
          else GET_FIELD(kerning.second, second, atoi(value));
          else GET_FIELD(kerning.amount, amount, atoi(value));
        }
        tkernings.push_back(kerning);
      }
    }

#undef GET_FIELD
#undef NEXT_TOKEN
#undef SEPARATOR

    // Now we look for the maximum character ID and we then remap the
    // characters such that their ID is their location in the array.
    for (size_t i = 0; i < tchars.size(); ++i)
      maxID = max(maxID, tchars[i].id);
    this->chars.resize(maxID+1);
    for (size_t i = 0; i < tchars.size(); ++i) {
      int32 charID = tchars[i].id;
      this->chars[charID] = tchars[i];
    }

    // Copy the kernings data
    this->kernings.resize(tkernings.size());
    for (size_t i = 0; i < this->kernings.size(); ++i)
      this->kernings[i] = tkernings[i];
    isLoaded = true;

  exit:
    return isLoaded;
  error:
    this->chars.resize(0);
    this->pages.resize(0);
    this->kernings.resize(0);
    isLoaded = false;
    goto exit;
  }
} /* namespace pf */

