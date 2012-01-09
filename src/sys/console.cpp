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
  Console::Console(ScriptSystem &scriptSystem, ConsoleDisplay &display) :
    scriptSystem(scriptSystem), display(display) {}
  Console::~Console(void) {}

  /*! Actual implementation of the console */
  class ConsoleInternal : public Console
  {
  public:
    ConsoleInternal(ScriptSystem &scriptSystem, ConsoleDisplay &display) :
      Console(scriptSystem, display)
    {
      this->setHistorySize(64);
      this->cursor = 0;
    }
  private:
    /*! The line itself */
    typedef vector<uint8> Line;
    /*! Implements base class method */
    virtual void update(const InputControl &input);
    virtual void setHistorySize(int32 size);
    virtual void addCompletion(const std::string &str);
    virtual void addHistory(const std::string &cmd);
    /*! Process the previous command in the history (instead of the current one) */
    void previousHistory(void);
    /*! Process the next command in the history (instead of the current one) */
    void nextHistory(void);
    /*! Append a new command in the history */
    void addHistory(const Line &cmd);
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
    vector<Line> history;
    /*! Current line */
    Line line;
    int32 cursor;      //!< Position of the cursor
    int32 historySize; //!< Entries in the history
    int32 historyNum;  //!< Total number of command entered by the user
    int32 historyCurr; //!< Index of the history entry we are editing
  };

  INLINE bool isPrintable(uint32 key) {
    return (key > 0x20 && key <= 0x80) ||
            key == ' ';
  }
  INLINE bool isSpecial(uint32 key) { return key > 0x80; }
  INLINE bool isNonPrintable(uint32 key) { return key <= 0x20; }
  INLINE bool isAlphaNumeric(uint32 key) {
    return (key >= 'a' && key <= 'z') ||
           (key >= 'A' && key <= 'Z') ||
           (key >= '0' && key <= '9');
  }
  INLINE bool isInWord(uint32 key) { return isAlphaNumeric(key) || key == '_'; }

  void ConsoleInternal::setHistorySize(int32 size) {
    size = max(size, 1);
    this->history.resize(size);
    this->historySize = size;
    this->historyNum = 0;
    this->historyCurr = 0;
  }

  void ConsoleInternal::previousHistory(void) {
    // Save the current line on its history position
    history[historyCurr % historySize] = line;
    // Do not edit a line that are not stored anymore
    const int32 minIndex = max(historyNum - historySize, 0);
    historyCurr = max(historyCurr-1, minIndex);
    line = history[historyCurr % historySize];
    cursor = line.size();
  }

  void ConsoleInternal::nextHistory(void) {
    // Save the current line on its history position
    history[historyCurr % historySize] = line;
    historyCurr = max(min(historyCurr+1, historyNum),0);
    line = history[historyCurr % historySize];
    cursor = line.size();
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
    int32 index = cursor-1, sz = 0;
    while (index >= 0) {
      if (isInWord(line[index]) == false) break;
      ++sz;
      --index;
    }
    if (sz == 0) return;
    // Compute the word to complete
    std::stringstream word;
    for (int32 i = 0; i < sz; ++i)
      word << line[cursor-sz+i];
    const std::string str = word.str();
    // Find the lower and upper bound for the candidates.
    auto candidate = completions.lower_bound(str);
    // Store all words that share the same prefix than our current word
    vector<std::string> candidates;
    const size_t strSize = str.size();
    for (;;) {
      if (candidate == completions.end()) break;
      if (candidate->size() < strSize) break;
      bool samePrefix = true;
      for (size_t i = 0; i < strSize; ++i)
        if (str[i] != (*candidate)[i]) {
          samePrefix = false;
          break;
        }
      if (!samePrefix) break;
      candidates.push_back(*candidate);
      ++candidate;
    }
    if (candidates.size() == 0u) return;
    // Complete the word with the shortest candidate
    const int32 candidateLength = int32(candidates[0].size());
    PF_ASSERT(candidateLength >= sz);
    for (int32 i = sz; i < candidateLength; ++i)
      this->insert(candidates[0][i]);
    // More than one candidate. We print all of them
    if (candidates.size() > 1u)
      for (uint32 i = 0; i < candidates.size(); ++i)
        this->display.out(*this, candidates[i]);
  }

  void ConsoleInternal::addCompletion(const std::string &str) {
    completions.insert(str);
  }

  void ConsoleInternal::addHistory(const std::string &cmd) {
    Line line;
    for (size_t i = 0; i < cmd.size(); ++i) line.push_back(cmd[i]);
    this->addHistory(line);
  }

  void ConsoleInternal::addHistory(const Line &line) {
    if (line.size() == 0) return;
    history[historyNum % historySize] = line;
    historyNum++;
  }

  void ConsoleInternal::execute(void) {
    line.push_back(0); // terminate the string
    std::string toRun = std::string((const char *) &this->line[0]);
    this->addHistory(toRun); // append the command in the history
    ScriptStatus status;
    scriptSystem.run(toRun.c_str(), status);
    if (status.success == false)
      this->display.out(*this, status.msg);
    line.resize(0); // start a new command
    cursor = 0;
    historyCurr = historyNum;
  }

  void ConsoleInternal::update(const InputControl &control) {
    // Process all pressed keys
    for (size_t i = 0; i < control.keyPressed.size(); ++i) {
      const uint8 key = control.keyPressed[i];
      if (isPrintable(key))
        this->insert(key);
      else if (isNonPrintable(key)) {
        switch (key) {
          case PF_KEY_ASCII_SP: this->insert(' '); break;
          case PF_KEY_ASCII_HT: this->complete(); break;
          case PF_KEY_ASCII_LF:
          case PF_KEY_ASCII_CR:
            this->execute();
            break;
          case PF_KEY_ASCII_BS:
            if (line.size() > 0) {
              cursor = max(int32(cursor-1), 0);
              line.resize(line.size() - 1);
            }
            break;
          default: break;
        };
      } else if (isSpecial(key)) {
        switch (key) {
          case PF_KEY_END: cursor = line.size(); break;
          case PF_KEY_HOME: cursor = 0; break;
          case PF_KEY_LEFT: cursor = max(int32(cursor-1), 0); break;
          case PF_KEY_RIGHT: cursor = min(cursor, int32(line.size())); break;
          case PF_KEY_UP: this->previousHistory(); break;
          case PF_KEY_DOWN: this->nextHistory(); break;
          default: break;
        }
      }
    }

    // Display the line
    line.push_back(0); // terminate the string
    std::string toDisplay = std::string((const char *) &this->line[0]);
    line.pop_back();
    display.line(*this, toDisplay);
  }

  Console *ConsoleNew(ScriptSystem &scriptSystem, ConsoleDisplay &display) {
    return PF_NEW(ConsoleInternal, scriptSystem, display);
  }

} /*! namespace pf */

