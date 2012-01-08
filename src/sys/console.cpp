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

#include "console.hpp"
#include "script.hpp"
#include "set.hpp"
#include "vector.hpp"
#include "windowing.hpp"
#include "math/math.hpp"
#include <string>
#include <sstream>

namespace pf
{
  Console::Console(ScriptSystem &scriptSystem) :scriptSystem(scriptSystem) {}
  Console::~Console(void) {}

  /*! Actual implementation of the console */
  class ConsoleInternal : public Console
  {
  public:
    ConsoleInternal(ScriptSystem &scriptSystem) : Console(scriptSystem) {
      this->setHistorySize(64);
      this->cursor = 0;
    }
  private:
    /*! Implements base class method */
    virtual void update(const InputControl &input);
    virtual void setHistorySize(uint32 size);
    virtual void addCompletion(const std::string &str);
    /*! Insert a new character at the current cursor position */
    void insert(uint8 key);
    /*! Run the current line */
    void execute(void);
    /*! Completes the line if only one possibility. Otherwise, print the
     *  candidates
     */
    void complete(void);
    /*! Stores words that can be candidates for completion */
    set<std::string> completions;
    /*! History is stored as a circular buffer */
    vector<std::string> history;
    /*! Current line */
    vector<uint8> line;
    int32 cursor;       //!< Position of the cursor
    uint32 historySize; //!< Entries in the history
    uint32 historyNum;  //!< Entries provided by the user
  };

  INLINE bool isPrintable(uint32 key) { return key > 0x20 && key <= 0x80; }
  INLINE bool isSpecial(uint32 key) { return key > 0x80; }
  INLINE bool isNonPrintable(uint32 key) { return key <= 0x20; }
  INLINE bool isAlphaNumeric(uint32 key) {
    return (key >= 'a' && key <= 'z') ||
           (key >= 'A' && key <= 'Z') ||
           (key >= '0' && key <= '9');
  }
  INLINE bool isInWord(uint32 key) { return isAlphaNumeric(key) || key == '_'; }

  void ConsoleInternal::setHistorySize(uint32 size) {
    size = min(size, 1u);
    this->history.resize(size);
    this->historySize = size;
    this->historyNum = 0;
  }

  void ConsoleInternal::insert(uint8 key) {
    PF_ASSERT(isPrintable(key));
    if (cursor == int32(line.size()))
      line.push_back(key);
    else {
      PF_ASSERT(cursor < int32(line.size()));
      vector<uint8>::iterator it = line.begin() + cursor;
      line.insert(it, key);
    }
    cursor++;
  }

  void ConsoleInternal::complete(void) {
    // Nothing to complete
    if (cursor == 0) return;
    PF_ASSERT(cursor <= int32(line.size()));
    const uint8 last = line[cursor-1];
    // Not a word?
    if (isInWord(last) == false) return;
    // Compute the size of the word
    int32 index = cursor-1, sz = 1;
    while (index >= 0) {
      if (isInWord(line[index]) == false) break;
      ++sz;
      --index;
    }
    // Compute the word to complete
    std::stringstream word;
    for (int32 i = 0; i < sz; ++i)
      word << line[cursor-sz+i];
    const std::string str = word.str();
    // Find the lower and upper bound for the candidates.
    auto begin = completions.lower_bound(str);
    auto end = completions.upper_bound(str);
    // Store all words that share the same prefix than our current word
    std::vector<std::string> candidates;
    for (auto it = begin; it != end; ++it)
      if (it->find(str) != 0)
        break;
      else
        candidates.push_back(*it);
    if (candidates.size() == 0u) return;
    // If we have only one candidate, we complete the word at the current cursor
    // position ...
    if (candidates.size() == 1u) {
      const int32 candidateLength = int32(candidates[0].size());
      PF_ASSERT(candidateLength >= sz);
      for (int32 i = sz; i < candidateLength; ++i)
        this->insert(candidates[0][i]);
    }
    // ... Otherwise, we print the candidates
    else {
      // TODO
    }
  }
  void ConsoleInternal::addCompletion(const std::string &str) {
    completions.insert(str);
  }
  void ConsoleInternal::execute(void) {}
  void ConsoleInternal::update(const InputControl &control)
  {
    for (size_t i = 0; i < control.keyPressed.size(); ++i) {
      const uint8 key = control.keyPressed[i];
      if (isPrintable(key))
        this->insert(key);
      else if (isNonPrintable(key)) {
        switch (key) {
          case PF_KEY_ASCII_LF:
          case PF_KEY_ASCII_CR:
            this->execute();
            break;
          case PF_KEY_ASCII_HT:
            this->complete();
            break;
          default: break;
        };
      } else if (isSpecial(key)) {
        switch (key) {
          case PF_KEY_END: cursor = line.size(); break;
          case PF_KEY_HOME: cursor = 0; break;
          case PF_KEY_LEFT: cursor = max(int32(cursor-1), 0); break;
          case PF_KEY_RIGHT: cursor = min(cursor, int32(line.size())); break;
          default: break;
        }
      }
    }
  }

  Console *ConsoleNew(ScriptSystem &scriptSystem) {
    return PF_NEW(ConsoleInternal, scriptSystem);
  }
  void ConsoleDelete(Console *console) { PF_DELETE(console); }

} /*! namespace pf */

