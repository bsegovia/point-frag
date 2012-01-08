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

#ifndef __PF_WINDOWING_HPP__
#define __PF_WINDOWING_HPP__

#include "platform.hpp"
#include "fixed_array.hpp"
#include "vector.hpp"
#include "ref.hpp"

namespace pf
{
  /*! Special key definitions. Special keys take the [128,255] slots (just after
   *  the ascii definitions)
   */
  enum {
    PF_KEY_F1        = 0x0001 + 0x80,
    PF_KEY_F2        = 0x0002 + 0x80,
    PF_KEY_F3        = 0x0003 + 0x80,
    PF_KEY_F4        = 0x0004 + 0x80,
    PF_KEY_F5        = 0x0005 + 0x80,
    PF_KEY_F6        = 0x0006 + 0x80,
    PF_KEY_F7        = 0x0007 + 0x80,
    PF_KEY_F8        = 0x0008 + 0x80,
    PF_KEY_F9        = 0x0009 + 0x80,
    PF_KEY_F10       = 0x000A + 0x80,
    PF_KEY_F11       = 0x000B + 0x80,
    PF_KEY_F12       = 0x000C + 0x80,
    PF_KEY_LEFT      = 0x0064 + 0x80,
    PF_KEY_UP        = 0x0065 + 0x80,
    PF_KEY_RIGHT     = 0x0066 + 0x80,
    PF_KEY_DOWN      = 0x0067 + 0x80,
    PF_KEY_PAGE_UP   = 0x0068 + 0x80,
    PF_KEY_PAGE_DOWN = 0x0069 + 0x80,
    PF_KEY_HOME      = 0x006A + 0x80,
    PF_KEY_END       = 0x006B + 0x80,
    PF_KEY_INSERT    = 0x006C + 0x80
  };

  /*! Non printable ASCII characters */
  enum {
    PF_KEY_ASCII_NUL = 0x000,  // Null char.
    PF_KEY_ASCII_SOH = 0x001,  // Start of Header
    PF_KEY_ASCII_STX = 0x002,  // Start of Text
    PF_KEY_ASCII_ETX = 0x003,  // End of Text
    PF_KEY_ASCII_EOT = 0x004,  // End of Transmission
    PF_KEY_ASCII_ENQ = 0x005,  // Enquiry
    PF_KEY_ASCII_ACK = 0x006,  // Acknowledgment
    PF_KEY_ASCII_BEL = 0x007,  // Bell
    PF_KEY_ASCII_BS  = 0x008,  // Backspace
    PF_KEY_ASCII_HT  = 0x009,  // Horizontal Tab
    PF_KEY_ASCII_LF  = 0x00A,  // Line Feed
    PF_KEY_ASCII_VT  = 0x00B,  // Vertical Tab
    PF_KEY_ASCII_FF  = 0x00C,  // Form Feed
    PF_KEY_ASCII_CR  = 0x00D,  // Carriage Return
    PF_KEY_ASCII_SO  = 0x00E,  // Shift Out
    PF_KEY_ASCII_SI  = 0x00F,  // Shift In
    PF_KEY_ASCII_DLE = 0x010,  // Data Link Escape
    PF_KEY_ASCII_DC1 = 0x011,  // XON - Device Control 1
    PF_KEY_ASCII_DC2 = 0x012,  // Device Control 2
    PF_KEY_ASCII_DC3 = 0x013,  // XOFF - Device Control 3
    PF_KEY_ASCII_DC4 = 0x014,  // Device Control 4
    PF_KEY_ASCII_NAK = 0x015,  // Negativ Acknowledgemnt
    PF_KEY_ASCII_SYN = 0x016,  // Synchronous Idle
    PF_KEY_ASCII_ETB = 0x017,  // End of Trans. Block
    PF_KEY_ASCII_CAN = 0x018,  // Cancel
    PF_KEY_ASCII_EM  = 0x019,  // End of Medium
    PF_KEY_ASCII_SUB = 0x01A,  // Substitute
    PF_KEY_ASCII_ESC = 0x01B,  // Escape
    PF_KEY_ASCII_FS  = 0x01C,  // File Separator
    PF_KEY_ASCII_GS  = 0x01D,  // Group Separator
    PF_KEY_ASCII_RS  = 0x01E,  // Reqst to Send - Rec. Sep.
    PF_KEY_ASCII_US  = 0x01F,  // Unit Separator
    PF_KEY_ASCII_SP  = 0x020   // Space
  };

  /*! Contains the fields processed by the event handler */
  class InputControl : public RefCount, public NonCopyable
  {
  public:
    InputControl(const InputControl &previous);
    InputControl(int w, int h);
    /*! Get the current state for key */
    INLINE bool getKey(uint32 key) const;
    /*! Indicates that a key is not pressed */
    INLINE void upKey(uint32 key);
    /*! Indicates that a key is pressed */
    INLINE void downKey(uint32 key);
    /*! Get the current input control state */
    void processEvents(void);
    double time;              //!< Current time when capturing the events
    double dt;                //!< Delta from previous event processing
    int mouseXRel, mouseYRel; //!< Position of the mouse
    int w, h;                 //!< Dimension of the window (if changed)
    int isResized;            //!< True if the user resized the window
    vector<uint8> keyPressed; //!< All keys that have been pressed
    vector<uint8> keyReleased;//!< All keys that have been released
  private:
    enum { MAX_KEYS = 256u };
    enum { KEY_ARRAY_SIZE = MAX_KEYS / sizeof(int32) };
    fixed_array<uint32,KEY_ARRAY_SIZE> keys;
    PF_CLASS(InputControl);
  };

  INLINE bool InputControl::getKey(uint32 key) const {
    PF_ASSERT(key < MAX_KEYS);
    const uint32 entry = key / 32; // which bitfield? (int32 == 32 bits)
    const uint32 bit = key % 32;   // which bit in the bitfield?
    return (this->keys[entry] & (1u << bit)) != 0;
  }
  INLINE void InputControl::upKey(uint32 key) {
    PF_ASSERT(key < MAX_KEYS);
    const uint32 entry = key / 32, bit = key % 32;
    this->keys[entry] &= ~(1u << bit);
  }
  INLINE void InputControl::downKey(uint32 key) {
    PF_ASSERT(key < MAX_KEYS);
    const uint32 entry = key / 32, bit = key % 32;
    this->keys[entry] |=  (1u << bit);
  }

  /*! Call back prototype for OGL */
  typedef void (*WinProc)();
  /*! Open a new OGL window */
  void WinOpen(int w, int h);
  /*! Close the opened window */
  void WinClose(void);
  /*! Get a proc address for OGL */
  WinProc WinGetProcAddress(const char *name);
  /*! Tell if a OGL extension is supported or not */
  int WinExtensionSupported(const char *ext);
  /*! Swap the buffers */
  void WinSwapBuffers(void);

} /* namespace pf */

#endif /* __PF_WINDOWING_HPP__ */

