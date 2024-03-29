/*
Copyright 2019-2023 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cstdio>
#include <cstdlib>
#include <regex>
#include <string>
#include <list>
#include <iostream>
#include <map>
#include <sstream>
#include <functional>
#include <cstdarg>
#include <optional>
#include <deque>
#include <iomanip>

#include "swapfile.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

#include "common.h"
#include "cprintf.h"
#include "shell.h"

extern "C" const char LICENSE[];

volatile std::atomic<bool> ctrlc = false;
volatile std::atomic<bool> ctrlint = false;
HANDLE TheConsoleStdin = NULL;
UINT oldConsoleOutputCP = 0, oldConsoleInputCP = 0;
void __cdecl RestoreConsoleCP()
{
    if(oldConsoleOutputCP) SetConsoleOutputCP(oldConsoleOutputCP);
    if(oldConsoleInputCP) SetConsoleCP(oldConsoleInputCP);
}

enum class LineEndings
{
    CrLf = 0,
    Lf = 1
};

struct GState {
    std::string filename;
    int line;
    std::string PROMPT = "* ";
    Swapfile swapfile;
    std::map<char, LinePtr> registers;
    size_t nlines;
    std::string diagnostic;
    bool dirty = false;
    std::function<char()> readCharFn;
    std::function<void(std::string)> writeStringFn;
    bool error = false;
    bool Hmode = false;
    int zWindow = 1;
    bool showPrompt = false;
    std::regex regexp;
    std::string fmt;
    std::list<LinePtr> gLines;
    WCHAR conInBuf[4096];
    CHAR conInBufUTF8[4096 * 5];
    std::string lastBang;
    bool suppress = false;
    LineEndings lineEndings = LineEndings::CrLf;

    GState(decltype(filename) _filename = ""
        , decltype(line) _line = 0
        , decltype(PROMPT) _PROMPT = "* "
        , decltype(swapfile)&& _swapfile = {Swapfile::IN_MEMORY_SWAPFILE}
        , decltype(registers) _registers = {}
        , decltype(nlines) _nlines = 0
        , decltype(diagnostic) _diagnostic = ""
        , decltype(dirty) _dirty = false
        , decltype(readCharFn) _readCharFn = {}
        , decltype(writeStringFn) _writeStringFn = {}
        , decltype(error) _error = false
        , decltype(Hmode) _Hmode = false
        , decltype(zWindow) _zWindow = 1
        , decltype(showPrompt) _showPrompt = false
        , decltype(regexp) _regexp = std::regex{".", std::regex_constants::ECMAScript}
        , decltype(fmt) _fmt = ""
        , decltype(gLines) _gLines = {}
        , decltype(lastBang) _lastBang = {}
        , decltype(suppress) _suppress = false
        , decltype(lineEndings) _lineEndings = LineEndings::CrLf
    )
        : filename(_filename)
        , line(_line)
        , PROMPT(_PROMPT)
        , swapfile(std::move(_swapfile))
        , registers(_registers)
        , nlines(_nlines)
        , diagnostic(_diagnostic)
        , dirty(_dirty)
        , readCharFn(_readCharFn)
        , writeStringFn(_writeStringFn)
        , error(_error)
        , Hmode(_Hmode)
        , zWindow(_zWindow)
        , showPrompt(_showPrompt)
        , regexp(_regexp)
        , fmt(_fmt)
        , gLines(_gLines)
        , lastBang(_lastBang)
        , suppress(_suppress)
        , lineEndings(_lineEndings)
    {}

    void operator()(decltype(filename) _filename = ""
        , decltype(line) _line = 0
        , decltype(PROMPT) _PROMPT = "* "
        , int _swapfile = Swapfile::IN_MEMORY_SWAPFILE
        , decltype(registers) _registers = {}
        , decltype(nlines) _nlines = 0
        , decltype(diagnostic) _diagnostic = ""
        , decltype(dirty) _dirty = false
        , decltype(readCharFn) _readCharFn = {}
        , decltype(writeStringFn) _writeStringFn = {}
        , decltype(error) _error = false
        , decltype(Hmode) _Hmode = false
        , decltype(zWindow) _zWindow = 1
        , decltype(showPrompt) _showPrompt = false
        , decltype(regexp) _regexp = std::regex{".", std::regex_constants::ECMAScript}
        , decltype(fmt) _fmt = ""
        , decltype(gLines) _gLines = {}
        , decltype(lastBang) _lastBang = {}
        , decltype(suppress) _suppress = {}
    ) {
        filename = _filename;
        line = _line;
        PROMPT = _PROMPT;
        swapfile.type(_swapfile);
        registers = _registers;
        nlines = _nlines;
        diagnostic = _diagnostic;
        dirty = _dirty;
        readCharFn = _readCharFn;
        writeStringFn = _writeStringFn;
        error = _error;
        Hmode = _Hmode;
        zWindow = _zWindow;
        showPrompt = _showPrompt;
        regexp = _regexp;
        fmt = _fmt;
        gLines = _gLines;
        lastBang = _lastBang;
        suppress = _suppress;
    } // GState::operator()
} g_state;

struct JakEDException : public std::runtime_error
{
    JakEDException(std::string const& s)
        : runtime_error(s)
    {}
};

// FIXME refactor these somehow to be inside ConsoleReader
//       the main loop now clears these in case it picked up
//       on CTRL_C_EVENT before the console reader
static int lastWasD = 0;
static char lastChar = '\0';
static std::deque<int> buffer = {};
struct ConsoleReader {

    int operator()()
    // WOW, this is a long story...
    {
        while(!buffer.empty()) {
            auto c = buffer.front();
            buffer.pop_front();

            if(lastWasD == 2) {
                lastWasD = 0;
                return lastChar;
            }
    
            if(c == (char)10 && lastWasD == 1) {
                lastWasD = 0;
                return c;
            }
    
            if(lastWasD == 1 && c != (char)10) {
                lastWasD = 2;
                lastChar = c;
                return (char)13;
            }
    
            // OF COURSE we're on windows and <CR> == [0xD, 0xA]
            // Let's pretend we're always ignoring that character.
            if(c == (char)13) {
                cprintf<CPK::CTRLC2>("char 13\n");
                lastWasD = true;
                continue;
            }

            cprintf<CPK::CTRLC>("yielding %c [%X] out of %zd left\n", c, c, buffer.size());

            return c;
        }
        // So, if there is no STDIN, just return EOF
        if(feof(stdin)) {
            cprintf<CPK::CTRLC>("stdin was closed somehow\n");
            return EOF;
        }
        // Next, if we're not connected to a terminal,
        // use fgetc(stdin) like normal. Behaves in the
        // expected way.
        if(!ISATTY(fileno(stdin))) {
            return fgetc(stdin); // maybe if I use ReadFileW here, I can process ^Z?
        }
        // Aaaaand here it gets interesting.
        char c = (char)0;
        DWORD rv = 0;
        do {
            // Clear GetLastError() because as we know nobody
            // ever does.
            SetLastError(0);
            // This and ReadFileA are those functions which
            // actually get an ERROR_OPERATION_ABORTED error
            // if they are interrupted (as defined for ^C and ^BRK)
            cprintf<CPK::CTRLC2>("reading\n");
            // maybe read something to the extent of a full line? ReadConsoleW returns after \n
#ifdef CTRLV
            CONSOLE_READCONSOLE_CONTROL lpCRC;
            memset(&lpCRC, 0, sizeof(CONSOLE_READCONSOLE_CONTROL));
            lpCRC.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
            lpCRC.nInitialChars = 0;
            lpCRC.dwCtrlWakeupMask = 1 << ('V' - 'A' + 1);
            lpCRC.dwControlKeyState = 0;
#endif

            auto success = ReadConsoleW(
                    TheConsoleStdin,
                    &g_state.conInBuf,
                    sizeof(g_state.conInBuf)/sizeof(g_state.conInBuf[0]) - 1,
                    &rv,
#ifdef CTRLV
                    &lpCRC
#else
                    NULL
#endif
                    );
            g_state.conInBuf[sizeof(g_state.conInBuf) / sizeof(g_state.conInBuf[0]) - 1] = L'\0';
            if(GetLastError() != 0) {
                cprintf<CPK::CTRLC>("ERROR_OPERATION_ABORTED is %lu\n", ERROR_OPERATION_ABORTED);
                cprintf<CPK::CTRLC>("ERROR_INVALID_ARGUMENT is 87\n");
                cprintf<CPK::CTRLC2>("GLE: %lu\n", GetLastError());
            }
            if constexpr(IsKeyEnabled<CPK::CTRLC>::value) {

                fprintf(stderr, "Read %lu chars: ", rv);
                for(size_t i = 0; i < rv; ++i)
                    fprintf(stderr, "\\x%04X", g_state.conInBuf[i]);
                fprintf(stderr, "\n");
            }
#ifdef CTRLV
            cprintf<CPK::CTRLC>("CRC: %u %u %X %X\n", lpCRC.nLength, lpCRC.nInitialChars, lpCRC.dwCtrlWakeupMask, lpCRC.dwControlKeyState);
            cprintf<CPK::CTRLC>("Explanation of last flag:\n"
                    "\tCAPSLOCK_ON %X\n"
                    "\tENHANCED_KEY %X\n"
                    "\tLEFT_ALT_PRESSED %X\n"
                    "\tLEFT_CTRL_PRESSED %X\n"
                    "\tNUMLOCK_ON %X\n"
                    "\tRIGHT_ALT_PRESSED %X\n"
                    "\tRIGHT_CTRL_PRESSED %X\n"
                    "\tSCROLLLOCK_ON %X\n"
                    "\tSHIFT_PRESSED %X\n",
                    CAPSLOCK_ON,
                    ENHANCED_KEY,
                    LEFT_ALT_PRESSED,
                    LEFT_CTRL_PRESSED,
                    NUMLOCK_ON,
                    RIGHT_ALT_PRESSED,
                    RIGHT_CTRL_PRESSED,
                    SCROLLLOCK_ON,
                    SHIFT_PRESSED);
#endif
            // You can probably guess success == true and rv = 0,
            // but w/e. If that happened...
            if(GetLastError() == ERROR_OPERATION_ABORTED) {
                lastWasD = 0;
                lastChar = (char)0;
                // Clear GetLastError() because as we know nobody
                // ever does.
                SetLastError(0);
                // Set a different interrupt flag because otherwise we're
                // randomly racing with the interrupt thread. We'll wait
                // for the consoleHandler thread later.
                ctrlint = true;

                // Okay, if ctrlc is triggered, return some BS character.
                // The caller will handle the flag.
                cprintf<CPK::CTRLC>("from ctrlc, returning NUL\n");
                buffer.clear();
                return '\0';
            }

            if(GetLastError() == 0 && rv > 0) {
                // FIXME NULL obviously terminates the string somewhere
                auto cchbuf = WideCharToMultiByte(CP_UTF8, 0, g_state.conInBuf, rv, g_state.conInBufUTF8, sizeof(g_state.conInBufUTF8) - 1, NULL, NULL);
                g_state.conInBufUTF8[cchbuf] = '\0';
                for(size_t i = 0; i < cchbuf; ++i) {
                    buffer.push_back(g_state.conInBufUTF8[i]);
                }
                cprintf<CPK::CTRLC>("As utf8: %s\n", g_state.conInBufUTF8);
            }

            // In the event that ReadConsoleA fails (I never saw it fail yet)
            // then report some error and exit.
            if(!success) {
                fprintf(stderr, "ReadConsoleA failed with %lu\n", GetLastError());
                return EOF;
            }

#ifdef CTRLV
            //printf("%X\n", buf[rv - 1]);
            if(rv >= 1 && buf[rv - 1] == 0x16 &&
               (lpCRC.dwControlKeyState & LEFT_CTRL_PRESSED))
            {
              if(!(lpCRC.dwControlKeyState & RIGHT_CTRL_PRESSED)
              && !(lpCRC.dwControlKeyState & SHIFT_PRESSED)
              && !(lpCRC.dwControlKeyState & LEFT_ALT_PRESSED)
              && !(lpCRC.dwControlKeyState & ENHANCED_KEY))
              {
                buffer.pop_back(); // pop 0x16
                INPUT_RECORD ir;

                do {
                  if(CtrlC()) {
                    lastWasD = 0;
                    lastChar = (char)0;
                    // Clear GetLastError() because as we know nobody
                    // ever does.
                    SetLastError(0);
                    // Set a different interrupt flag because otherwise we're
                    // randomly racing with the interrupt thread. We'll wait
                    // for the consoleHandler thread later.
                    ctrlint = true;

                    // Okay, if ctrlc is triggered, return some BS character.
                    // The caller will handle the flag.
                    cprintf<CPK::CTRLC>("from ctrlc, returning NUL\n");
                    buffer.clear();
                    return '\0';
                  }
                  success = ReadConsoleInput(TheConsoleStdin,
                          &ir,
                          1,
                          &rv);
                  if(GetLastError() == ERROR_OPERATION_ABORTED) {
                    lastWasD = 0;
                    lastChar = (char)0;
                    // Clear GetLastError() because as we know nobody
                    // ever does.
                    SetLastError(0);
                    // Set a different interrupt flag because otherwise we're
                    // randomly racing with the interrupt thread. We'll wait
                    // for the consoleHandler thread later.
                    ctrlint = true;

                    // Okay, if ctrlc is triggered, return some BS character.
                    // The caller will handle the flag.
                    cprintf<CPK::CTRLC>("from ctrlc, returning NUL\n");
                    buffer.clear();
                    return '\0';
                  }
                  // don't care about non key presses
                  if(ir.EventType != KEY_EVENT) continue;

                  auto ker = ir.Event.KeyEvent;

                  // don't care about key up
                  if(!ker.bKeyDown) continue;

                  cprintf<CPK::CTRLC>("uChar %X\n", ker.uChar.UnicodeChar);


                  auto Z = [](wchar_t* c) {
                    DWORD tmp;
                    if(!ISATTY(_fileno(stdout))) return;
                    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                            c,
                            wcslen(c),
                            &tmp,
                            NULL);
                  };
                  // maybe literal input
                  if(ker.dwControlKeyState & LEFT_CTRL_PRESSED) {
                    switch(ker.wVirtualKeyCode)
                    {
                    case '2': buffer.push_back('\0'); Z(L"^@"); return operator()();
                    case 'A': case 'B': case 'C': case 'D': case 'E':
                    case 'F': case 'G': case 'H': case 'I': case 'J':
                    case 'K': case 'L': case 'M': case 'N': case 'O':
                    case 'P': case 'Q': case 'R': case 'S': case 'T':
                    case 'U': case 'V': case 'W': case 'X': case 'Y':
                    case 'Z':
                              buffer.push_back(ker.wVirtualScanCode - 'A' + 1);
                              {wchar_t sss[] = { L'^', ker.wVirtualKeyCode, L'\0' };
                              Z(sss);}
                              return operator()();
                    case 0xDB: buffer.push_back(27); Z(L"^["); return operator()();
                    case 0xDC: buffer.push_back(28); Z(L"^\\"); return operator()();
                    case 0xDD: buffer.push_back(29); Z(L"^]"); return operator()();
                    case '6': buffer.push_back(30); Z(L"^^"); return operator()();
                    case 0xBD: buffer.push_back(31); Z(L"^_"); return operator()();
                    case 0xBF: buffer.push_back(0x7f); Z(L"^?"); return operator()();
                    default:
                              break;
                    }
                  }

                  if(!ker.uChar.UnicodeChar
                          && !(ker.dwControlKeyState & LEFT_CTRL_PRESSED
                               && ker.wVirtualKeyCode == 0x32))
                  {
                      //printf("'2' %x %x\n", '2', ker.wVirtualKeyCode);
                      //printf("[   %x]%x\n", 0xDB, 0xDD);
                      continue;
                  }

                  WCHAR theChar = ker.uChar.UnicodeChar;
                  // echo char
                  if(ISATTY(_fileno(stdout)))
                      WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                          &theChar,
                          1,
                          &rv,
                          NULL);

                  auto cchbuf = WideCharToMultiByte(
                          CP_UTF8,
                          0,
                          &theChar,
                          1,
                          buf2, 
                          8,
                          NULL,
                          NULL);

                  for(size_t i = 0; i < cchbuf; ++i) {
                    buffer.push_back(buf2[i]);
                  }
                  return operator()();
                } while(1);
              }
            }
#endif
            // If STDIN got closed, return EOF.
            if(feof(stdin)) {
                cprintf<CPK::CTRLC>("lost stdin!!! my magic didn't work\n");
                buffer.push_back(EOF);
                return operator()();
            }
            // Finally, return the character
            return operator()();
        } while(1);
    } // operator()
} Interactive_readCharFn;

void Interactive_writeStringFn(std::string const& s)
{
    fprintf(stdout, "%s", s.c_str());
    fflush(stdout);
}

struct Range
{
    Range() : first(1), second(1) {};
    Range(Range const&) = default;
    Range& operator=(Range const&) = default;
    Range(Range&& o) : first(o.first), second(o.second) {}
    Range& operator=(Range&& o) {
        const_cast<int&>(first) = o.first;
        const_cast<int&>(second) = o.second;
        return *this;
    }
    static int Dot(int offset = 0) { return g_state.line + offset; }
    static int Dollar(int offset = 0) { return g_state.nlines + offset; }

    static Range ZERO() {
        return Range(0, 0);
    }

    static Range R(int _1, int _2)
    {
        if(_1 < 0) _1 = 0;
        if(_2 < 0) _2 = 0;
        if(_1 > g_state.nlines) _1 = g_state.nlines;
        if(_2 > g_state.nlines) _2 = g_state.nlines;
        if(_2 < _1) throw JakEDException("Backwards range");
        return Range(_1, _2);
    }

    static Range S(int _1)
    {
        return R(_1, _1);
    }

    const int first, second;

private: 
    Range(int _1, int _2)
        : first(_1)
        , second(_2)
    {}
}; // Range

int SkipUntilWS(std::string const& s, int i)
{
    while(i < s.size() && s[i] != ' ' && s[i] != '\t') ++i;
    return i;
}

int SkipWS(std::string const& s, int i)
{
    while(s[i] == ' ' || s[i] == '\t') ++i;
    return i;
}

// [number, continueFrom]
std::tuple<int, int> ReadNumber(std::string const&, int);
// [line, continueFrom]
template<bool acceptNoMatches = false>
std::tuple<int, int> ParseRegex(std::string s, int i);
// [fmt, G_OR_N, printLine]
std::tuple<std::string, int, bool> ParseReplaceFormat(std::string s, int i);
// [Range, continueFrom]
std::tuple<Range, int> ParseRange(std::string const&, int i);
// [Range, command, tail]
std::tuple<Range, char, std::string> ParseCommand(std::string s);

void ExecuteCommand(std::tuple<Range, char, std::string> command, std::string stream);

void Loop();

namespace CommandsImpl {
    std::string getShellCommand(std::string const& tail)
    {
        // don't skip whitespace
        if(tail[0] == '!') return g_state.lastBang;
        else return tail;
    }

    void r(Range range, std::string tail)
    {
        cprintf<CPK::r>("r: n before = %zd\n", g_state.nlines);
        int nthLine = range.second;
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) tail = g_state.filename;
        if(tail.empty()) throw JakEDException("No such file!");
        size_t bytes = 0;


        // navigate to insertion point
        int originalNlines = g_state.nlines;
        auto after = g_state.swapfile.head();
        while(nthLine-- > 0
                && after->next())
        {
            after = after->next();
            if(CtrlC()) {
                cprintf<CPK::CTRLC>("r: ctrlc invoked while skipping\n");
                cprintf<CPK::r>("r: ctrlc invoked while skipping\n");
                return;
            }
        }
        bool bomChecked = false;
        //g_state.nlines = 0;
        auto continueFrom = after->next();
        cprintf<CPK::r>("will continue after %s with %s\n", (after) ? (static_cast<std::string>(after->text()).c_str()) : "<EOF>", (continueFrom) ? static_cast<std::string>(continueFrom->text()).c_str() : "<EOF>");

        if(tail[0] == '!') {
            // TODO refactor common code out into function which operates on lines
            int linesInserted = 0;
            auto sink = [&after, continueFrom, &bomChecked, &bytes, &linesInserted](std::string const& s) {
                // note: this is called from a separate thread
                if(CtrlC()) return;
                bytes += s.size() + 1;
                ++linesInserted;
                cprintf<CPK::r>("Adding %s after %s and before %s\n",
                        s.c_str(),
                        (after) ? static_cast<std::string>(after->text()).c_str() : "EOF",
                        (continueFrom) ? static_cast<std::string>(continueFrom->text()).c_str() : "EOF");
                auto inserted = g_state.swapfile.line(s);
                inserted->link(continueFrom);
                after->link(inserted);
                after = inserted;
                ++g_state.nlines;
            };

            std::exception_ptr ex;
            try {
                // blocking
                g_state.lastBang = getShellCommand(tail.substr(1));
                Process::SpawnAndWait(
                        g_state.filename,
                        g_state.lastBang,
                        {},
                        sink);
            } catch(...) {
                ex = std::current_exception();
            }
            if(!g_state.suppress)
                g_state.writeStringFn("!\n");
            cprintf<CPK::r>("read %d lines\n", linesInserted);
            if(linesInserted) {
                cprintf<CPK::r>("set dirty flag\n");
                g_state.dirty = true;
            }

            std::stringstream undoBuffer;
            undoBuffer << range.second + 1 << "," << (range.second + linesInserted) << "d";
            auto undoHead = g_state.swapfile.line(undoBuffer.str());
            g_state.swapfile.undo(undoHead);

            std::stringstream bytesAsString;
            bytesAsString << bytes << std::endl;
            if(!g_state.suppress)
                g_state.writeStringFn(bytesAsString.str());
            g_state.line = range.second + linesInserted;

            if(ex) std::rethrow_exception(ex);

            return;
        }
        FILE* f = fopen(tail.c_str(), "r");
        struct AtEnd {
            FILE* f;
            AtEnd(FILE*& ff) : f(ff) {}
            ~AtEnd() { if(f) fclose(f); }
        } atEnd(f);
        if(f) {
            std::stringstream line;
            int c;
            while(!feof(f) && (c = fgetc(f)) != EOF) {
                if(CtrlC()) {
                    cprintf<CPK::CTRLC>("r: ctrlc invoked while reading\n");
                    cprintf<CPK::r>("r: ctrlc invoked while reading\n");
                    after->link(continueFrom);
                    return;
                }
                ++bytes;
                if(c == '\n') { // TODO properly determine if line endings are crlf and count bytes correctly
                    auto s = line.str();
                    // clear UTF8 BOM if it's there
                    if(!bomChecked && bytes >= 3
                            && s.find("\xEF" "\xBB" "\xBF") == 0)
                    {
                        s = s.substr(3);
                        bomChecked = true;
                    }
                    auto inserted = g_state.swapfile.line(s);
                    cprintf<CPK::r>("linking %s -> %s\n", ((after) ? static_cast<std::string>(after->text()).c_str() : "<EOF>"), (static_cast<std::string>(inserted->text()).c_str()));
                    after->link(inserted);
                    after = inserted;
                    ++g_state.nlines;
                    cprintf<CPK::r>("> %s\n", line.str().c_str()); // FIXME some utf8 doesn't get right for some reason; might be utf16
                    line.str(std::string());
                } else {
                    line << (char)(c & 0xFF);
                }
            } // read loop (f, c)
            if(line.str() != "") {
                auto inserted = g_state.swapfile.line(line.str());
                after->link(inserted);
                after = inserted;
                cprintf<CPK::r>("linking %s -> %s\n", ((after) ? static_cast<std::string>(after->text()).c_str() : "<EOF>"), (static_cast<std::string>(inserted->text()).c_str()));
                ++g_state.nlines;
            }
            after->link(continueFrom);
            cprintf<CPK::r>("linking %s -> %s\n", ((after) ? static_cast<std::string>(after->text()).c_str() : "<EOF>"), (continueFrom) ? (static_cast<std::string>(continueFrom->text()).c_str()) : "<EOF>");
            fclose(f);
            g_state.dirty = true;
            g_state.line = range.second + g_state.nlines - originalNlines;
            {
                std::stringstream output;
                output << bytes << std::endl;
                if(!g_state.suppress)
                    g_state.writeStringFn(output.str());
            }
            std::stringstream undoCommandBuf;
            undoCommandBuf << range.second + 1 << "," << (range.second + g_state.nlines - originalNlines) << "d";
            auto undoCommand = g_state.swapfile.line(undoCommandBuf.str());
            g_state.swapfile.undo(undoCommand);
            cprintf<CPK::r>("r: n after = %zd, line = %d\n", g_state.nlines, g_state.line);
        } else { // if !f
            throw JakEDException("No such file!");
        }
    } // r

    std::string getFileName(std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) tail = g_state.filename;
        if(tail.empty()) throw JakEDException("No such file!");
        return tail;
    }

    void E(Range range, std::string tail)
    {
        tail = getFileName(tail);
        if(tail[0] == '!') {
            cprintf<CPK::r>("E: forwarding shell command to r. Not setting filename\n");
        } else {
            g_state.filename = tail;
        }
        g_state.registers.clear();
        auto undoBuffer = g_state.swapfile.line("1,$c");
        undoBuffer->link(g_state.swapfile.head()->next());
        g_state.swapfile.cut(g_state.swapfile.head()->next());
        g_state.swapfile.head()->link();
        g_state.nlines = 0;
        r(Range::S(0), tail);
        g_state.swapfile.undo(undoBuffer);
        g_state.dirty = (tail[0] == '!');
    } // E

    void e(Range range, std::string tail)
    {
        if(g_state.dirty) throw JakEDException("file has modifications");
        return E(range, tail);
    }

    void H(Range range, std:: string tail)
    {
        g_state.Hmode = !g_state.Hmode;
    }

    void P(Range range, std:: string tail)
    {
        g_state.showPrompt = !g_state.showPrompt;
    }

    void h(Range range, std:: string tail)
    {
        std::stringstream ss;
        ss << g_state.diagnostic << std::endl;
        g_state.writeStringFn(ss.str());
    }

    void Q(Range r, std::string tail)
    {
        cprintf<CPK::Q>("in Q %d\n", (int)g_state.error);
        throw application_exit(g_state.error);
    }

    void q(Range range, std::string tail)
    {
        if(g_state.dirty) throw JakEDException("file has modifications");
        return Q(range, tail);
    }

    void l(Range r, std::string tail)
    {
        int first = std::max(1, r.first);
        int lineNumber = first;
        int second = std::max(1, r.second) - first + 1;
        auto i = g_state.swapfile.head();
        while(first--) i = i->next();
        first = r.first;
        while(i && second--) {
            std::stringstream ss;
            if(tail[0] == 'n') {
                ss << first++ << '\t';
            }
            for(auto c : static_cast<std::string>(i->text())) {
                if(isprint((unsigned char)c)) {
                    if(c == '\t') {
                        ss << "\\t";
                    } else if(c == '\n') {
                        ss << "\\n";
                    } else {
                        ss << c;
                    }
                } else {
                    switch(c) {
                    case 0:     ss << "^@"; break;
                    case 1: case 2: case 3: case 4: case 5:
                    case 6: case 7: case 8: case 9: case 10:
                    case 11: case 12: case 13: case 14: case 15:
                    case 16: case 17: case 18: case 19: case 20:
                    case 21: case 22: case 23: case 24: case 25:
                    case 26: 
                                ss << '^' << (char)((int)'A' + c - 1);
                                break;
                    case 27:    ss << "^["; break;
                    case 28:    ss << "^\\"; break;
                    case 29:    ss << "^]"; break;
                    case 30:    ss << "^^"; break;
                    case 31:    ss << "^_"; break;
                    case 0x7f:  ss << "^?"; break;
                    default: {
                                 // breaks unicode
#if 0
                               char buf[4];
                               sprintf(buf, "%X", (int)c);
                               ss << "\\x" << c;
                               break;
#else
                               ss << c;
#endif
                             }
                    } // switch(c)
                } // else if non printable
            } // for(c:static_cast<std::string>(i->text()))
            ss << '$' << std::endl;
            i = i->next();
            g_state.writeStringFn(ss.str());
            if(CtrlC()) return;
        }
        g_state.line = r.second;
    } // l


    void p(Range r, std::string tail)
    {
        int first = std::max(1, r.first);
        int lineNumber = first;
        int second = std::max(1, r.second) - first + 1;
        auto i = g_state.swapfile.head();
        while(first--) i = i->next();
        first = r.first;
        while(i && second--) {
            std::stringstream ss;
            if(tail[0] == 'n') {
                ss << first++ << '\t';
            }
            ss << static_cast<std::string>(i->text()) << std::endl;
            i = i->next();
            g_state.writeStringFn(ss.str());
            if(CtrlC()) return;
        }
        g_state.line = r.second;
    } // p

    void n(Range r, std::string tail)
    {
        return p(r, "n");
    }


    void NOP(Range r, std::string)
    {
        /*NOP*/
    }

    void k(Range r, std::string tail)
    {
        if(tail.empty()) throw JakEDException("missing argument");
        if(tail[0] < 'a' || tail[0] > 'z') throw JakEDException("Invalid register. Registers must be a lowercase ASCII letter");
        int idx = r.second;
        if(idx < 1 || idx > g_state.nlines)
            throw JakEDException("Address out of bounds");
        auto it = g_state.swapfile.head();
        while(idx-- > 0
                && it->next())
        {
            it = it->next();
            //printf("Marking: %d %s\n", idx, static_cast<std::string>(it->text()).c_str());
        }
        g_state.registers[tail[0]] = it;
    } // k

    void f(Range r, std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) {
            std::stringstream ss;
            ss << g_state.filename << "\n";
            g_state.writeStringFn(ss.str());
            return;
        }
        if(tail[0] == '!') throw JakEDException("Invalid filename");
        size_t i = tail.size();
        while(tail[i - 1] == ' ') --i;
        if(i < tail.size()) tail = tail.substr(0, i);
        g_state.filename = tail;
    } // f

    void EQUALS(Range r, std::string tail)
    {
        std::stringstream ss;
        if(g_state.nlines > 0) {
            if(r.second < 1 || r.second > g_state.nlines) throw JakEDException("invalid range");
            ss << r.second << std::endl;
            g_state.writeStringFn(ss.str());
        } else {
            ss << 0 << std::endl;
            g_state.writeStringFn(ss.str());
        }
    }

    void commonW(Range r, std::string fname, const char* mode)
    {
        cprintf<CPK::W>("R: [%d, %d]\n", r.first, r.second);
        if(r.first < 1 || r.second < 1 || r.first > g_state.nlines || r.second > g_state.nlines) throw JakEDException("Invalid range");
        fname = getFileName(fname);
        FILE* f;

        size_t nBytes = 0;

        auto i1 = g_state.swapfile.head();
        auto first = r.first, second = r.second - r.first + 1;
        while(first-- > 0) {
            cprintf<CPK::W>("skipping %s\n", (i1) ? static_cast<std::string>(i1->text()).c_str() : "<EOF>");
            i1 = i1->next();
            if(CtrlC()) throw JakEDException("Interrupted");
        }
        if(i1) cprintf<CPK::W>("Writing from %s\n", static_cast<std::string>(i1->text()).c_str());

        if(fname[0] == '!') {
            int i = 0;
            auto emitter = [&i1, &second, &i, &nBytes]() -> int {
                do {
                    if(second == 0) {
                        return EOF;
                    }
                    if(i == i1->length() + 1
                            && g_state.lineEndings == LineEndings::CrLf)
                    {
                        ++nBytes;
                        ++i;
                        return '\n';
                    } else if(i > i1->length()) { // +1 intentional, see below
                        second--;
                        i = 0;
                        i1 = i1->next();
                        continue;
                    }
                    ++nBytes;
                    if(i == i1->length()) {
                        ++i;
                        switch(g_state.lineEndings) {
                        case LineEndings::CrLf:
                            return '\r';
                        case LineEndings::Lf:
                            return '\n';
                        }
                    }
                    if(i < i1->length()) {
                        return (char)i1->text().data()[i++];
                    }
                } while(1);
            };
            std::exception_ptr ex;
            try {
                g_state.lastBang = getShellCommand(fname.substr(1));
                Process::SpawnAndWait(
                        g_state.filename,
                        g_state.lastBang,
                        emitter,
                        {});
            } catch(...) {
                ex = std::current_exception();
            }
            if(!g_state.suppress)
                g_state.writeStringFn("!\n");
            std::stringstream ss;
            ss << nBytes << std::endl;
            if(!g_state.suppress)
                g_state.writeStringFn(ss.str());
            return;
        }

        f = fopen(fname.c_str(), mode);
        if(!f) throw JakEDException("Cannot open file for writing");
        if(g_state.filename.empty()) g_state.filename = fname;

        while(second-- > 0 && i1) {
            if(i1) cprintf<CPK::W>("Writing %s\n", static_cast<std::string>(i1->text()).c_str());
            nBytes += i1->length() + /*LF*/1 + (g_state.lineEndings == LineEndings::CrLf);
            switch(g_state.lineEndings)
            {
                case LineEndings::CrLf:
                    fprintf(f, "%s\r\n", static_cast<std::string>(i1->text()).c_str());
                    break;
                case LineEndings::Lf:
                    fprintf(f, "%s\n", static_cast<std::string>(i1->text()).c_str());
                    break;
            }
            i1 = i1->next();
            if(CtrlC()) break;
        }
        fclose(f);
        //g_state.swapfile.gc();
        g_state.dirty = CtrlC(); //false;
        std::stringstream ss;
        ss << nBytes << std::endl;
        if(!g_state.suppress)
            g_state.writeStringFn(ss.str());
    } // commonW

    void w(Range r, std::string tail)
    {
        bool doQuit = false;
        if(tail[0] == 'q') {
            doQuit = true;
            tail = tail.substr(SkipWS(tail, 1));
        }
        commonW(r, tail, "wb");
        if(doQuit) q(r, "");
    }

    void W(Range r, std::string tail)
    {
        bool doQuit = false;
        if(tail[0] == 'q') {
            doQuit = true;
            tail = tail.substr(SkipWS(tail, 1));
        }
        commonW(r, tail, "ab");
        if(doQuit) q(r, "");
    }

    void z(Range r, std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
        int i = 0;
        if(!tail.empty()) {
            int newWindowSize = 0;
            std::tie(newWindowSize, i) = ReadNumber(tail, 0);
            if(i > 0) {
                if(newWindowSize <= 0) throw JakEDException("Parse error: invalid window size");
                g_state.zWindow = newWindowSize;
            }
        }
        p(Range::R(r.second, r.second + g_state.zWindow - 1), tail.substr(i));
    } // z

    void a(Range r, std::string)
    {
        cprintf<CPK::a>("line = %d, nlines = %zd\n", g_state.line, g_state.nlines);
        auto after = g_state.swapfile.head();
        auto idx = std::max(0, r.second);
        while(idx-- > 0
                && after->next())
        {
            after = after->next();
        }
        auto linkMeAtTheEnd = after->next();
        size_t nLines = 0;
        std::stringstream ss;
        while(1) {
            int cc = g_state.readCharFn();
            char c = cc;
            cprintf<CPK::a>("read a %x %c\n", c, c);
            cprintf<CPK::a>("ss = %s\n", ss.str().c_str());
            if(CtrlC()) return;
            if(c == '\n' || cc == EOF) {
                if(ss.str() == "." || (cc == EOF && ss.str() == "")) {
                    cprintf<CPK::a>("broke at .\n");
                    break;
                }
                cprintf<CPK::a>("pushing line\n");
                nLines++;
                auto inserted = g_state.swapfile.line(ss.str());
                after->link(inserted);
                after = inserted;
                g_state.nlines++;
                ss.str("");
                continue;
            }
            cprintf<CPK::a>("pushing char\n");
            ss << c;
        } // read loop
        if(nLines) {
            after->link(linkMeAtTheEnd);
            g_state.dirty = true;
            g_state.line = r.second + nLines;
            ss.str("");
            ss << r.second + 1 << "," << (r.second + nLines) << "d";
            auto inserted = g_state.swapfile.line(ss.str());
            g_state.swapfile.undo(inserted);
        }
    } // a

    void i(Range r, std::string)
    {
        auto pos = r.second;
        if(pos < 1 || pos > g_state.nlines) throw JakEDException("Invalid range");
        return a(Range::S(pos - 1), "");
    }

    void deleteLines(Range r, bool setCutBuffer)
    {
        // clobber registers
        bool toErase[26] = { false, false, false,
            false, false, false, false, false,
            false, false, false, false, false, false,
            false, false, false, false, false, false,
            false, false, false, false, false, false};
        std::vector<std::list<LinePtr>::iterator> clobbered; // during g// execution

        cprintf<CPK::d>("d: invoked with range [%d,%d]\n", r.first, r.second);
        auto it = g_state.swapfile.head();
        auto idx = r.first;
        size_t linesDeleted = r.first > 0;
        while(idx-- > 1
                && it->next())
        {
            it = it->next();
        }
        // it->next() is where we start deleting
        auto beforeDelete = it->Copy();
        it = it->next();
        idx = r.second - r.first + 1;
        while(idx-- > 0) {
            cprintf<CPK::d>("d: %d lines left to delete\n", idx+1);
            for(auto kv = g_state.registers.begin(); kv != g_state.registers.end(); ++kv) {
                if(kv->second
                        && kv->second == it)
                {
                    cprintf<CPK::regs>("marking %c to erase\n", kv->first);
                    cprintf<CPK::d>("marking %c to erase\n", kv->first);
                    toErase[kv->first - 'a'] = true;
                }
            }
            for(auto i = g_state.gLines.begin(); i != g_state.gLines.end(); ++i) {
                if(*i == it) clobbered.push_back(i);
            }
            if(idx > 0) {
                ++linesDeleted;
                it = it->next();
            }
        }
        auto empty = LinePtr();
        auto beforeContinue = (it) ? it->Copy() : empty;
        auto continueFromHere = (beforeContinue) ? beforeContinue->next() : empty;
        if(beforeContinue) beforeContinue->link();

        if(setCutBuffer) {
            g_state.swapfile.cut(beforeDelete->next());
        }
        std::stringstream undoCommand;
        undoCommand << (r.first - 1) << "a";
        auto inserted = g_state.swapfile.line(undoCommand.str());
        inserted->link(beforeDelete->next());
        g_state.swapfile.undo(inserted);

        beforeDelete->link(continueFromHere);

        for(char c = 'a'; c != 'z'; ++c) {
            if(toErase[c - 'a']) {
                cprintf<CPK::regs>("erasing r%c\n", c);
                cprintf<CPK::a>("erasing r%c\n", c);
                g_state.registers.erase(g_state.registers.find(c));
            }
        }
        for(auto i : clobbered) {
            g_state.gLines.erase(i);
        }

        g_state.dirty = true;
        g_state.line = r.first;
        g_state.nlines -= linesDeleted;
        cprintf<CPK::d>("d: gstate.line=%d gstate.nlines=%d\n", g_state.line, g_state.nlines);
    } // deleteLines

    void d(Range r, std::string)
    {
        return deleteLines(r, true);
    }

    void j(Range r, std::string tail)
    {
        if(r.first >= r.second) throw JakEDException("j(oin) needs a range of at least two lines");
        if(r.first < 1) throw JakEDException("Invalid range");
        deleteLines(r, false);
        auto oldLines = g_state.swapfile.undo()->next();
        auto oldLinesDup = (oldLines) ? oldLines->Copy() : LinePtr();
        std::stringstream ss;
        while(oldLines) {
            cprintf<CPK::j>("%s\n", static_cast<std::string>(oldLines->text()).c_str());
            ss << static_cast<std::string>(oldLines->text());
            if(oldLines->next()) ss << tail;
            oldLines = oldLines->next();
        }
        auto newLine = g_state.swapfile.line(ss.str());
        auto idx = r.first - 1;
        auto it = g_state.swapfile.head();
        while(idx-- > 0
                && it->next())
        {
            it = it->next();
        }
        newLine->link( (it) ? it->next() : it );
        it->link(newLine);
        cprintf<CPK::j>("   inserting %s before %d\n", ss.str().c_str(), r.first);
        g_state.line = r.first;

        ss.str("");
        ss << r.first << "," << r.second << "c";
        auto newUndoCommand = g_state.swapfile.line(ss.str());
        newUndoCommand->link(oldLinesDup);
        g_state.swapfile.undo(newUndoCommand);
        g_state.nlines += 1; // the inserted one
    } // j

    void c(Range r, std::string)
    {
        deleteLines(r, true);
        auto oldLines = g_state.swapfile.undo()->next();
        auto currentNumLines = g_state.nlines;
        a(Range::S(r.first - 1), "");
        std::stringstream ss;
        ss << r.first << "," << (r.first + g_state.nlines - currentNumLines - 1) << "c";
        auto newUndoHead = g_state.swapfile.line(ss.str());
        newUndoHead->link(oldLines);
        g_state.swapfile.undo(newUndoHead);
    } // c

    // See doc/UndoAndSwapFile.md#s----algorithm
    // I was lazy to name my variables accordingly, and that doc
    // explains better what's what and why
    void s(Range r, std::string tail)
    {
        // TODO clobber registers and gLines
        auto it = g_state.swapfile.head(); // 60
        int idx = r.first;
        while(idx-- > 1 && it) { // 60
            it = it->next();
            if(CtrlC()) return;
        }

        if(!it || idx > 0) throw JakEDException("Invalid range");

        auto pushedDot = g_state.line;
        std::exception_ptr ex; // 10
        g_state.line = r.first; // 20
        int line = 0;
        int i = 0;
        tail = tail.substr(SkipWS(tail, i));
        // 30 set up call which depends on global
        if(!tail.empty()) {
            int lineToMatch = g_state.line;
            --g_state.line;
            if(g_state.line <= 0) g_state.line = g_state.nlines;
            try {
                std::tie(line, i) = ParseRegex(tail, i); // 30
                if(line != lineToMatch) throw JakEDException("Line does not match"); // let exception propagate since we didn't do anything yet
            } catch(...) {
                // restore initial state since we didn't touch anything yet
                g_state.line = pushedDot;
                std::rethrow_exception(std::current_exception());
            }
        } // if(!tail.empty())
        g_state.line = r.first; // 20 again
        std::string fmt;
        int G_OR_N = 0;
        bool printLine;
        tail = tail.substr(i);
        if(tail.empty()) {
            std::tie(fmt, G_OR_N, printLine) = ParseReplaceFormat(g_state.fmt, 0); // 35
        } else {
            std::tie(fmt, G_OR_N, printLine) = ParseReplaceFormat(tail, 0); // 35
            g_state.fmt = tail;
        }
        cprintf<CPK::regex>("fmt = %s, G_OR_N = %d, printLine = %s\n",
                fmt.c_str(),
                G_OR_N,
                (printLine) ? "true" : "false");

        auto uh = g_state.swapfile.line(""); // 40
        auto up = uh->Copy(); // 50
        std::stringstream patternSpace;

        for(; g_state.line <= r.second; pushedDot = -1) { // 70
            if(CtrlC()) {
                ex = std::make_exception_ptr(JakEDException("Interrupted"));
                break;
            }
            auto ax = it->next(); // 80
            if(!ax) {
                ex = std::make_exception_ptr(JakEDException("Internal error - EOF encountered"));
                break;
            }
            auto cx = static_cast<std::string>(ax->text()); // 90
            if(!std::regex_search(cx, g_state.regexp)) { // 100
                ex = std::make_exception_ptr(JakEDException("Line does not match")); // 110
            }
            // 120 -----v
            std::regex_constants::match_flag_type flags = 
                std::regex_constants::match_flag_type::format_sed;
            if(G_OR_N == -1) {
            } else if(G_OR_N == 0) {
                flags = flags | std::regex_constants::match_flag_type::format_first_only;
            } else {
                flags = flags | std::regex_constants::match_flag_type::format_first_only;
            }
            try {
                if(G_OR_N < 0) {
                    std::smatch m;
                    std::string other;
                    if(printLine && (other = cx, true))
                    while(std::regex_search(other, m, g_state.regexp)) 
                    {
                        patternSpace << m[0].str();
                        other = std::string(m[0].second, other.cend());
                    }
                    cx = std::regex_replace(cx, g_state.regexp, fmt, flags);
                } else if (G_OR_N == 0) {
                    std::smatch m;
                    if(printLine) {
                        std::regex_search(cx, m, g_state.regexp);
                        patternSpace << m[0].str();
                    }
                    cx = std::regex_replace(cx, g_state.regexp, fmt, flags);
                } else { /* replace Nth only */
                    std::stringstream result; // intermediate result
                    std::smatch m; // last match
                    idx = 0; // count the matches
                    // first match MUST match
                    auto hr = std::regex_search(cx, m, g_state.regexp);
                    if(!hr) throw JakEDException("Internal error: regex that first match doesn't match anymore in s///");
                    // loop
                    while(1) {
                        cprintf<CPK::regex>("Loop %d begun; pattern space: %s\n", idx+1, m[0].str().c_str());
                        // are we there yet? (yes, s///1 == s///<void>
                        if(++idx == G_OR_N) {
                            // save this for /p
                            patternSpace << m[0].str();
                            // execute the replacement on what's left of the string and append it to our result
                            result << std::regex_replace(cx, g_state.regexp, fmt, flags);
                            // cx is result, per #120
                            cx = result.str();
                            cprintf<CPK::regex>("Loop %d ended;\n    result: %s\n    cx: %s\n    patternSpace: %s\n", idx, result.str().c_str(), cx.c_str(), patternSpace.str().c_str());
                            // exit
                            break;
                        }

                        // m[0] is the full match, second is past-the-end
                        // so append from the begining to where the match
                        // ends; we will be looping starting from m[0].second
                        result << std::string(cx.cbegin(), m[0].second);
                        cx.assign(m[0].second, cx.cend());
                        //n.b. m is not valid at this point
                        cprintf<CPK::regex>(
                                "Loop %d ended;\n"
                                "    result: %s\n"
                                "    cx: %s\n",
                                idx,
                                result.str().c_str(),
                                cx.c_str());
                        // repeat search
                        hr = std::regex_search(cx, m, g_state.regexp);
                        if(!hr) throw JakEDException("No such match");
                        continue;
                    } // loop to find N
                } // if G_OR_N(<0,=0,>0)
            } catch(...) {
                ex = std::current_exception();
                break;
            }
            // 120 -----^
            ax = g_state.swapfile.line(cx); // 130
            ax->link(it->next()->next()); // 140
            it->next()->link(); // 150
            up->link(it->next()); // 160
            it->link(ax); // 170;

            it = it->next(); // 180
            up = up->next(); // 190
            if(ex) break; // 200
            g_state.line++; // 205
            g_state.dirty = true;

            if(printLine) {
                patternSpace << std::endl;
                g_state.writeStringFn(patternSpace.str());
            }
        } // for(...) // 70

        std::stringstream undoBuffer;
        undoBuffer << r.first << "," << g_state.line << "c";
        auto ul = g_state.swapfile.line(undoBuffer.str()); // 210
        g_state.swapfile.undo(ul); // 220
        ul->link(uh->next()); // 230

        g_state.line--;
        if(ex) {
            if(pushedDot >= 0) g_state.line = pushedDot; // pop!
            std::rethrow_exception(ex); // 240
        }
    } // s

    void x(Range r, std::string)
    {
        auto it = g_state.swapfile.head();
        int idx = r.second;
        while(idx-- > 0 && it) {
            it = it->next();
            if(CtrlC()) return;
        }

        if(!it || idx > 0) throw JakEDException("Invalid range");

        auto cutBuffer = g_state.swapfile.cut();
        auto nextOne = it->next();
        int lines = 0;
        it->link(cutBuffer);
        while(cutBuffer) {
            auto line = g_state.swapfile.line(static_cast<std::string>(cutBuffer->text()));
            line->link(nextOne);
            it->link(line);
            it = line;
            cutBuffer = cutBuffer->next();
            ++lines;
            if(CtrlC()) break;
        }

        if(lines) {
            std::stringstream undoBuffer;
            undoBuffer << r.second + 1 << "," << r.second + lines << "d";
            auto line = g_state.swapfile.line(undoBuffer.str());
            g_state.swapfile.undo(line);

            g_state.nlines += lines;

            g_state.line = r.second + lines;

            g_state.dirty = true;
        }
    } // x

    void y(Range r, std::string)
    {
        if(r.first < 1) throw JakEDException("Invalid range");
        size_t linesYanked = 0;
        auto it = g_state.swapfile.head();
        auto idx = r.first;
        while(idx-- > 1
                && it->next())
        {
            it = it->next();
        }

        auto cutHead = g_state.swapfile.line("");
        auto cutOrigin = cutHead->Copy();
        // it->next() is where we start yanking
        it = it->next();
        idx = r.second - r.first + 1;
        while(idx-- > 0 && it) {
            auto line = g_state.swapfile.line(static_cast<std::string>(it->text()));
            cutHead->link(line);
            cutHead = line;
            ++linesYanked;
            it = it->next();
            if(CtrlC()) break;
        }
        if(linesYanked) {
            g_state.swapfile.cut(cutOrigin->next());
            g_state.line = r.second;
        }
    } // y

    void m(Range r, std::string tail)
    {
        if(r.first < 1 || r.second > g_state.nlines) throw JakEDException("Invalid range");

        tail = tail.substr(SkipWS(tail, 0));
        int i;
        Range destination;
        std::tie(destination, i) = ParseRange(tail, 0);
        if(destination.second < 0 || destination.second > g_state.nlines) throw JakEDException("Invalid target range");
        if(destination.second == r.second) {
            g_state.line = destination.second;
            return; // NOP
        }
        std::stringstream undoBuffer;
        auto newLine = 0;
        cprintf<CPK::MandT>("M,N P = %d,%d %d\n", r.first, r.second, destination.second);
        if(destination.second > r.second) {
            auto nLines = r.second - r.first + 1;
            undoBuffer << destination.second - nLines + 1
                << ","
                << destination.second
                << "m"
                << r.first - 1;
            newLine = destination.second;
        } else if(destination.second < r.first) {
            auto nLines = r.second - r.first + 1;
            undoBuffer << destination.second + 1
                << ","
                << destination.second + nLines - 1
                << "m"
                << r.second;
            newLine = destination.second + r.second - r.first + 1;
        } else {
            throw JakEDException("Destination overlaps with source");
        }

        int idx = r.first;
        auto beforeCut = g_state.swapfile.head();
        while(idx-- > 1
                && beforeCut)
        {
            beforeCut = beforeCut->next();
            if(CtrlC()) {
                return;
            }
        }
        int count = r.second - r.first + 1;
        auto beforeNext = beforeCut->Copy();
        while(count--
                && beforeNext)
        {
            beforeNext = beforeNext->next();
            if(CtrlC()) {
                return;
            }
        }

        idx = destination.second;
        auto afterThis = g_state.swapfile.head();
        while(idx--
                && afterThis)
        {
            afterThis = afterThis->next();
            if(CtrlC()) {
                return;
            }
        }

        if(beforeCut != afterThis) {
            auto continueFrom = afterThis->next();
            afterThis->link(beforeCut->next());
            beforeCut->link(beforeNext->next());
            beforeNext->link(continueFrom);
        }

        g_state.line = newLine;
        g_state.dirty = true;

        auto inserted = g_state.swapfile.line(undoBuffer.str());
        g_state.swapfile.undo(inserted);
    } // m

    void t(Range r, std::string tail)
    {
        if(r.first < 1 || r.second > g_state.nlines) throw JakEDException("Invalid range");

        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) tail = ".";
        int i;
        Range destination;
        std::tie(destination, i) = ParseRange(tail, 0);
        cprintf<CPK::MandT>("M,N P = %d,%d %d\n", r.first, r.second, destination.second);
        if(destination.second < 0 || destination.second > g_state.nlines) throw JakEDException("Invalid target range");
        if(destination.second >= r.first && destination.second < r.second) {
            throw JakEDException("Destination overlaps source");
        }

        size_t numInsertedLines = 0;

        int idx = r.first;
        auto beforeCut = g_state.swapfile.head();
        while(idx-- > 1
                && beforeCut)
        {
            beforeCut = beforeCut->next();
            if(CtrlC()) {
                return;
            }
        }

        idx = destination.second;
        auto afterThis = g_state.swapfile.head();
        while(idx--
                && afterThis)
        {
            afterThis = afterThis->next();
            if(CtrlC()) {
                return;
            }
        }

        int count = r.second - r.first + 1;
        auto it = beforeCut->next();
        while(count--
                && it)
        {
            auto inserted = g_state.swapfile.line(static_cast<std::string>(it->text()));
            inserted->link(afterThis->next());
            afterThis->link(inserted);

            ++numInsertedLines;
            afterThis = inserted;
            it = it->next();
            if(CtrlC()) {
                break;
            }
        }

        cprintf<CPK::MandT>("numInsertedLines: %zd\n", numInsertedLines);
        if(numInsertedLines) {
            std::stringstream undoBuffer;
            undoBuffer << (destination.second + 1)
                << ","
                << (destination.second + numInsertedLines)
                << "d";

            g_state.nlines += numInsertedLines;
            g_state.line = destination.second + numInsertedLines;
            
            g_state.dirty = true;

            auto inserted = g_state.swapfile.line(undoBuffer.str());
            g_state.swapfile.undo(inserted);
        }
    } // t

    // TODO move this to be part of the parser instead of part of the g// family
    std::vector<std::string> ReadMultipleLines(std::optional<std::string> first = std::nullopt)
    {
        bool readMore = false;
        decltype(ReadMultipleLines(first)) rval;
        if(first) {
            if((*first)[(*first).size() - 1] == '\\')
            {
                (*first) = (*first).substr(0, (*first).size() - 1);
                readMore = true;
            }
            rval.push_back(*first);
        }
        while(readMore) {
            if(CtrlC()) throw JakEDException("Interrupted");
            std::stringstream buf;
            int ii = 0;
            while((ii = g_state.readCharFn()) != EOF) {
                cprintf<CPK::g>("ii%d\n", ii);
                char c = (char)(ii & 0xFF);
                // If we got interrupted, clear the buf buffer
                if(CtrlC()) {
                    throw JakEDException("Interrupted");
                }
                //cprintf<CPK::g>("Read %x\n", c);
                if(c == '\n') break;
                buf << c;
            } // read loop
            cprintf<CPK::g>("Read: %s\n", buf.str().c_str());
            auto line = buf.str();
            if(line[line.size() - 1] == '\\')
            {
                cprintf<CPK::g>("Reading more\n");
                line = line.substr(0, line.size() - 1);
                readMore = true;
            } else {
                readMore = false;
            }
            rval.push_back(line);
        } // while(readMore)
        return rval;

    } // ReadMultipleLines

    struct GFlags {
        enum {
            None = 0,
            Interactive = (1 << 0),
            Reverse = (1 << 1)
        };

        template<int flags>
        constexpr static bool TestLine(int left, int right)
        {
            if constexpr((flags & Reverse) == 0) {
                return left == right;
            } else {
                return !(left == right);
            }
        }
    };

    template<int flags>
    void gImpl(Range r, std::string tail)
    {
        int first = r.first,
            last = r.second;
        if(first < 1) first = 1;
        if(last > g_state.nlines) last = g_state.nlines;
        if(first > g_state.nlines) first = g_state.nlines;
        if(last < 1) last = 1;
        cprintf<CPK::g>("Initial range is [%d,%d]\n", first, last);

        auto savedDot = g_state.line;
        bool exitWithInvalidTail = false;

        // parse regex
        g_state.line = first - 1;
        if(g_state.line < 1) g_state.line = Range::Dollar();
        int i = 0;
        int nextMatch = 0;
        try {
            cprintf<CPK::g>("Testing regex %s\n", tail.c_str());
            std::tie(nextMatch, i) = ParseRegex<!!(flags & GFlags::Reverse)>(tail, i);
            g_state.line = nextMatch;
            cprintf<CPK::g>("Next line is @%d\n", nextMatch);
        } catch(JakEDException& ex) {
            cprintf<CPK::g>("Regex failed\n");
            g_state.line = savedDot;
            if(CtrlC()) {
                g_state.line = savedDot;
                throw JakEDException("Interrupted");
            }
            if constexpr((flags & GFlags::Reverse) == 0) {
                cprintf<CPK::g>("Done\n");
                return;
            } else {
                cprintf<CPK::g>("Setting next match to -1\n");
                nextMatch = -1;
            }
        }

        decltype(ReadMultipleLines(tail.substr(i))) commandList;
        if constexpr((flags & GFlags::Interactive) == 0) {
            // acquire out command list
            commandList = ReadMultipleLines(tail.substr(i));
        }

        // mark lines we'll be processing, and save undo lines as well
        auto undoList = g_state.swapfile.line("1,$c");
        g_state.swapfile.undo(undoList);
        {
            auto undoHead = undoList;
            auto it = g_state.swapfile.head();
            int idx = 0;
            while(it->next()) {
                it = it->next();
                ++idx;
                if(GFlags::TestLine<flags>(idx, nextMatch) && idx >= first && idx <= last) {
                    g_state.gLines.push_back(it);
                    cprintf<CPK::g>("Will work with line %d = %s, so far I have %zd\n",
                            idx,
                            static_cast<std::string>(it->text()).c_str(),
                            g_state.gLines.size());
                }
                if(idx == nextMatch && nextMatch >= first && nextMatch <= last) {
                    try {
                        std::tie(nextMatch, i) = ParseRegex<!!(flags & GFlags::Reverse)>("//", 0); 
                        g_state.line = nextMatch;
                        cprintf<CPK::g>("Next line is @%d\n", nextMatch);
                    } catch(JakEDException& ex) {
                        nextMatch = 0;
                        cprintf<CPK::g>("No more match\n");
                    }
                }
                if(CtrlC()) {
                    g_state.line = savedDot;
                    throw JakEDException("Interrupted");
                }
                auto inserted = g_state.swapfile.line(it->ref());
                undoHead->link(inserted);
                undoHead = inserted;
            }
            if(CtrlC()) {
                g_state.line = savedDot;
                throw JakEDException("Interrupted");
            }
        }

        decltype(g_state.readCharFn) prevReadCharFn;

        while(!g_state.gLines.empty() && !exitWithInvalidTail) {
            auto dot = g_state.line;
            // pop head
            auto lp = g_state.gLines.front()->Copy();
            g_state.gLines.pop_front();
            // set dot
            auto it = g_state.swapfile.head();
            int line = 0;
            while(it && it != lp) {
                ++line;
                it = it->next();
            }
            cprintf<CPK::g>("Will work with line %s which is now No. %d\n",
                    static_cast<std::string>(it->text()).c_str(),
                    line);
            if(!it) continue;

            if constexpr(!!(flags & GFlags::Interactive)) {
                cprintf<CPK::g>("Will send commands %s\n", static_cast<std::string>(it->text()).c_str());
                g_state.writeStringFn(static_cast<std::string>(it->text()) + "\n");
            }

            g_state.line = line;
            // hide global undo mark
            int64_t savedMark = g_state.swapfile.saveUndo();
            cprintf<CPK::g>("Hiding global undo mark %I64d\n", savedMark);
            //g_state.swapfile.undo(LinePtr());
            g_state.swapfile.restoreUndo(0); // avoid having to test how well I check for nullptr...
            try {
                if constexpr((flags & GFlags::Interactive) != 0) {
                    // acquire out command list
                    auto fakeCommandList = ReadMultipleLines("\\");
                    commandList = decltype(commandList)(fakeCommandList.begin() + 1, fakeCommandList.end());
                    if(commandList.size() == 1 && commandList.front() == "") commandList = decltype(commandList){};
                }
                cprintf<CPK::g>("Executing commands:\n");
                //for(auto&& commandLine : commandList) {
                size_t increment = 1, state = 0;
                for(size_t index = 0; index < commandList.size() && !exitWithInvalidTail; index+=increment) {
                    auto&& commandLine = commandList[index];
                    cprintf<CPK::g>(">> next command: %s\n", commandLine);
                    increment = 1;
                    auto t = ParseCommand(commandLine);
                    cprintf<CPK::g>("Executing: [%d,%d]%c%s\n",
                            std::get<0>(t).first,
                            std::get<0>(t).second,
                            std::get<1>(t),
                            std::get<2>(t).c_str());

                    switch(std::get<1>(t)) {
                    case 'g':
                    case 'G':
                    case 'v':
                    case 'V':
                        exitWithInvalidTail = true;
                        throw JakEDException("gGvV are not supported in gGvV command-list");
                    }

                    switch(std::get<1>(t)) {
                    case 'a':
                    case 'i':
                    case 'c':
                        state = 0;
                        prevReadCharFn = g_state.readCharFn;
                        g_state.readCharFn = std::function<int()>(
                                [&increment, &commandList, index, &state]() -> int {
                                if(index + increment >= commandList.size()) return EOF;
                                if(state >= commandList[index + increment].size()) {
                                    state = 0;
                                    increment++;
                                    return '\n';
                                }
                                if(index + increment >= commandList.size()) return EOF;
                                return commandList[index + increment][state++];
                        });
                        break;
                    default: break;
                    }
                    ExecuteCommand(t, "");
                    if(prevReadCharFn) {
                        g_state.readCharFn = prevReadCharFn;
                        prevReadCharFn = decltype(g_state.readCharFn)();
                    }
                    if(CtrlC()) throw JakEDException("Interrupted");
                }
                dot = g_state.line;
            } catch(JakEDException& ex) {
                // nop
                cprintf<CPK::g>("Last command failed with %s\n", ex.what());
                g_state.line = dot;
            }
            // restore global undo mark
            cprintf<CPK::g>("Restoring global undo mark %I64d\n", savedMark);
            g_state.swapfile.restoreUndo(savedMark);
            //g_state.swapfile.undo(undoList);
            if(prevReadCharFn) {
                g_state.readCharFn = prevReadCharFn;
                prevReadCharFn = decltype(g_state.readCharFn)();
            }
            if(CtrlC()) {
                cprintf<CPK::g>("Interrupted\n");
                throw JakEDException("Interrupted");
            }
        }
        cprintf<CPK::g>("Ended.\n");
        if(exitWithInvalidTail) throw JakEDException("The g, G, v and V commands may not appear in the command-list of a g, G, v or V command");
    } // g

    void Shell(Range r, std::string tail)
    {
        if(r.second != 0) throw JakEDException("Internal error: unexpected valid range");
        tail = tail.substr(SkipWS(tail, 0));
        g_state.lastBang = getShellCommand(tail);
        std::exception_ptr ex;
        try {
            Process::SpawnAndWait(
                    g_state.filename,
                    g_state.lastBang,
                    {},
                    {});
        } catch(...) {
            ex = std::current_exception();
        }
        if(!g_state.suppress)
            g_state.writeStringFn("!\n");
        if(ex) std::rethrow_exception(ex);
    }

    void Filter(Range r, std::string tail)
    {
        if(r.second == 0) throw JakEDException("Internal error: unexpected valid range");
        if(r.second > g_state.nlines) r = Range::R(r.first, g_state.nlines);
        tail = tail.substr(SkipWS(tail, 0));

        // TODO undo, preserve dot, nlines, structure, etc

        size_t nBytes = 0;
        auto uh = g_state.swapfile.line("");
        auto ul = uh->Copy();

        auto after = g_state.swapfile.head();
        auto first = r.first, second = r.second - r.first + 1;
        while(first-- > 1) {
            cprintf<CPK::W>("skipping %s\n", (after) ? static_cast<std::string>(after->text()).c_str() : "<EOF>");
            after = after->next();
            if(CtrlC()) throw JakEDException("Interrupted");
        }
        auto i1 = after->next();
        auto i2 = after;
        for(int sz = second; sz > 0 && i2; --sz) {
            if(CtrlC()) throw JakEDException("Interrupted");
            i2 = i2->next();
            if(i2) {
                uh->link(g_state.swapfile.line(i2->ref()));
                uh = uh->next();
            }
        }
        
        auto continueFrom = i2->next();
        if(i1) cprintf<CPK::W>("Writing from %s\n", static_cast<std::string>(i1->text()).c_str());

        int i = 0;
        auto emitter = [&second, &i1, &nBytes, &i]() -> int {
            do {
                if(second == 0) {
                    return EOF;
                }
                if(i == i1->length() + 1
                        && g_state.lineEndings == LineEndings::CrLf)
                {
                    ++nBytes;
                    ++i;
                    return '\n';
                } else if(i > i1->length()) { // +1 intentional, see below
                    second--;
                    i = 0;
                    i1 = i1->next();
                    continue;
                }
                ++nBytes;
                if(i == i1->length()) {
                    ++i;
                    switch(g_state.lineEndings) {
                    case LineEndings::CrLf:
                        return '\r';
                    case LineEndings::Lf:
                        return '\n';
                    }
                }
                if(i < i1->length()) {
                    return (char)i1->text().data()[i++];
                }
            } while(1);
        };

        size_t bytes = 0;
        size_t linesInserted = 0;
        auto sink = [&bytes, &linesInserted, &after, continueFrom](std::string const& s) {
            // note: this is called from a separate thread
            if(CtrlC()) return;
            bytes += s.size() + 1;
            ++linesInserted;
            cprintf<CPK::shell>("linesInserted = %zd, bytes = %zd, nBytes = %d\n", linesInserted, bytes, -1);
            cprintf<CPK::r>("Adding %s after %s and before %s\n",
                    s.c_str(),
                    (after) ? static_cast<std::string>(after->text()).c_str() : "EOF",
                    (continueFrom) ? static_cast<std::string>(continueFrom->text()).c_str() : "EOF");
            auto inserted = g_state.swapfile.line(s);
            inserted->link(continueFrom);
            after->link(inserted);
            after = inserted;
            ++g_state.nlines;
        };

        // unlink lines we're filtering
        after->link(); // before r.first
        i2->link(); // before continueFrom
        g_state.nlines -= second; // FIXME confusing; second is modified but by code that's not yet executed

        // TODO don't modify the file if nothing happened
        g_state.dirty = true;
        std::exception_ptr ex;
        try {
            g_state.lastBang = getShellCommand(tail);
            Process::SpawnAndWait(
                    g_state.filename,
                    g_state.lastBang,
                    emitter,
                    sink);
        } catch(...) {
            ex = std::current_exception();
        }


        if(linesInserted) {
            g_state.line = r.first + linesInserted - 1;
            std::stringstream undoBuffer;
            undoBuffer << r.first
                << ","
                << r.second
                << "c";
            auto undoCommand = g_state.swapfile.line(undoBuffer.str());
            undoCommand->link(ul->next());
            g_state.swapfile.undo(undoCommand);
        } else {
            std::stringstream undoBuffer;
            g_state.line = std::min(r.first - 1, 1);
            undoBuffer << r.first - 1
                << "a";
            auto undoCommand = g_state.swapfile.line(undoBuffer.str());
            undoCommand->link(ul->next());
            g_state.swapfile.undo(undoCommand);
            after->link(continueFrom);
        }
        if(!g_state.suppress)
            g_state.writeStringFn("!\n");

        cprintf<CPK::shell>("linesInserted = %zd, bytes = %zd, nBytes = %zd\n", linesInserted, bytes, nBytes);

        if(ex) std::rethrow_exception(ex);
    }

    void BANG(Range r, std::string tail)
    {
        cprintf<CPK::shell>("Range is [%d,%d]\n", r.first, r.second);
        cprintf<CPK::shell>("Tail is %s\n", tail.c_str());
        if(r.second == 0) return Shell(r, tail);
        else return Filter(r, tail);
    }

    void u(Range r, std::string tail)
    {
        auto head = g_state.swapfile.undo();
        auto savedMark = g_state.swapfile.popUndo();

        auto l = head;
        std::exception_ptr ex;
        auto oldReadCharFn = g_state.readCharFn;
        try {
            int state = 0;
            g_state.readCharFn = std::function<int()>(
                    [&state, &l]() -> int {
                    if(!l) return EOF;
                    auto ll = l->Copy();
                    if(l->IsIndirectHandle()) ll = l->deref();
                    auto&& s = ll->text();
                    if(state < s.size()) return (char)s.data()[state++];
                    if(state == s.size()) {
                        l = l->next();   
                        state = 0;
                        return '\n';
                    }
                    throw JakEDException("Internal error: invalid state");
            });
            Loop();
        } catch(application_exit&) {
            /*NOP*/
        } catch(...) {
            ex = std::current_exception();
        }
        g_state.readCharFn = oldReadCharFn;
        g_state.swapfile.restoreUndo(savedMark);
        if(ex) std::rethrow_exception(ex);
    } // u

    void COLON(Range r, std::string tail)
    {
        // TODO make this more vim like:
        // set fileencoding=unix/dos
        // set backup=~
        // etc
        const static std::map<std::string, std::pair<std::function<void(Range&, std::string&)>, std::string>> xcommands {
            { "crlf", { [](Range&, std::string&) { g_state.lineEndings = LineEndings::CrLf; }, "switch to windows CRLF line endings" } },
            { "lf", { [](Range&, std::string&) { g_state.lineEndings = LineEndings::Lf; }, "switch to UNIX LF line endings" } },
            { "license", { [](Range&, std::string&) { g_state.writeStringFn(LICENSE); }, "print the license" } },
        };

        auto ws = SkipUntilWS(tail, 0);
        std::string command = tail.substr(0, ws);
        cprintf<CPK::extended>("ws=%d command=%s, tail=%s, input=%s.\n", ws, command.c_str(), tail.substr(ws).c_str(), tail.c_str());
        tail = tail.substr(ws);

        if(command == "help") {
            g_state.writeStringFn(R"(Commands are usually of the syntax:
<optional range><command character><optional additional options>

Ranges are of the form:
N,M                 a range from the Nth line to Mth line
N                   equivalent to N,N
$                   last line
%                   equivalent to 1,$
,                   equivalent to 1,$
.                   current line number
N-P,M+Q             basic arithmetics are supported
-P,+Q               equivalent to .-P,.+Q

All commands "remember" most things, e.g.: the current line,
the last regular expression, and so on.

)");
            g_state.writeStringFn(R"(Available commands:
P                    toggle prompt
[.,.]p               print
[.,.]n               print with line numbers
e [file]             edit file
E [file]             edit file discarding buffer
[$]r [file|!shell]   read file or shell command
q                    quit
Q                    quit discarding buffer
h                    print last diagnostic message
H                    automatically print last diagnostic message
#                    ignored (used for comments)
[.]kx                marks the last line of the specified range as 'x',
                     where x is one of [a-z]
[$]=                 print the line number of the last line in range
f                    print the current file name
f file               sets the current file name
[1,$]w [file|!shell] write buffer to file or shell command
[1,$]W [file|!shell] append buffer to file or shell command
[.+1]z[1][n]         scroll from end of range a number of specified lines.
                     z is followed by the number of lines to scroll by.
                     The trailing number is optional, and remembered.
                     The command may be followed by 'n', in which case it
                     prints line numbers.
                     zn continues scrolling with the memorized N, printing
                     line numbers.
[.]i                 insert mode. Ends when it encounters a line containing
                     a single '.'. Text is inserted before last line of range.
[.]a                 same as 'i', but text is inserted after range
[.,.]c               deletes specified range and immediately enters insert mode
[.,.]d               deletes specified range
[.,+1]j[ ]           joins the lines in the range with the string immediately
                     trailing 'j'. The default joining character is a string.
[.,.]s/re/st/[pgN]   regexp search and replace
                     p: print search space when substituting
                     g: repeat substitution for all matches on a line
                     N: a number, specifying how many times to execute 
                        the substitution. Default is 1
                     the regexp syntax is POSIX BRE with \+ and \?
                     everything may be omitted, the command mostly remembers
[.]x                 inserts text from the cut buffer after the range
                     any command that deletes lines places the removed
                     text in the cut buffer
[.,.]y               yanks (copies) text into the cut buffer, without deleting
[.,.]l               lists the range unabiguously, by spelling out
                     non-printable characters (mostly), and appending a $ at
                     the end of the line
[.,.]m[.]            moves the specified range after the trailing range
[.,.]t[.]            transfers (copies) the specified range after the trailing
                     range
[1,$]g/re/list       for each line that matches 're' executes the command list
                     commands in the list are separated by a \ immediately
                     followed by a new line
[1,$]v/re/list       same as 'g', but executes for lines NOT matching 're'
[1,$]G/re/           same as 'g', but interactively asks for commands
                     an empty line means 'no op'
                     a single & means 'repeat the last command list'
[1,$]V/re/           same as 'G', but for lines NOT matching 're'
[.+1]                the 'empty command'; prints the last line of the
                     specified range
u                    undo (infinitely)
!shell               executes a shell command
                     In the command line, $ is replaced with the current
                     file name
                     'shell' may be a single '!', meaning "last command"
N,M!shell            when a range is specified, sends the text to the
                     shell command, and replaces the lines in the range
                     with the output of the shell command
:command             executes an extended (not single character) command

)");
            g_state.writeStringFn("Available extended commands:\n");
            g_state.writeStringFn(":help                prints this message\n");
            for(auto&& kv : xcommands) {
                std::stringstream ss;
                ss << ":" << std::setfill(' ') << std::setw(20) << std::left << kv.first << kv.second.second << std::endl;
                g_state.writeStringFn(ss.str());
            }
            return;
        }
        if(auto found = xcommands.find(command);
                found != xcommands.end())
        {
            (found->second.first)(r, tail);
        } else {
            std::stringstream ss;
            ss << "Unknown extended command `" << command << "'. See :help";
            throw JakEDException(ss.str());
        }

    } // COLON

} // namespace CommandsImpl

std::map<char, std::function<void(Range, std::string)>> Commands = {
    { 'P', &CommandsImpl::P },
    { 'p', &CommandsImpl::p },
    { 'n', &CommandsImpl::n },
    { 'e', &CommandsImpl::e },
    { 'E', &CommandsImpl::E },
    { 'r', &CommandsImpl::r },
    { 'q', &CommandsImpl::q },
    { 'Q', &CommandsImpl::Q },
    { 'h', &CommandsImpl::h },
    { 'H', &CommandsImpl::H },
    { '#', &CommandsImpl::NOP },
    { 'k', &CommandsImpl::k },
    { '=', &CommandsImpl::EQUALS },
    { 'f', &CommandsImpl::f },
    { 'w', &CommandsImpl::w },
    { 'W', &CommandsImpl::W },
    { 'z', &CommandsImpl::z },
    { 'i', &CommandsImpl::i },
    { 'a', &CommandsImpl::a },
    { 'c', &CommandsImpl::c },
    { 'd', &CommandsImpl::d },
    { 'j', &CommandsImpl::j },
    { 's', &CommandsImpl::s },
    { 'x', &CommandsImpl::x },
    { 'y', &CommandsImpl::y },
    { 'l', &CommandsImpl::l },
    { 'm', &CommandsImpl::m },
    { 't', &CommandsImpl::t },
    { 'g', &CommandsImpl::gImpl<CommandsImpl::GFlags::None> },
    { 'v', &CommandsImpl::gImpl<CommandsImpl::GFlags::Reverse> },
    { 'G', &CommandsImpl::gImpl<CommandsImpl::GFlags::Interactive> },
    { 'V', &CommandsImpl::gImpl<CommandsImpl::GFlags::Interactive|CommandsImpl::GFlags::Reverse> },
    { '!', &CommandsImpl::BANG },
    { 'u', &CommandsImpl::u },
    { ':', &CommandsImpl::COLON },
}; // Commands

void ExecuteCommand(std::tuple<Range, char, std::string> command, std::string stream)
{
    Commands.at(std::get<1>(command))(std::get<0>(command), std::get<2>(command));
}

void exit_usage(char* msg, char* argv0)
{
    if(msg) fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Usage: %s [-hVsrWU] [-p PROMPT] [FILE]\nIf FILE begins with a bang, it will be executed as a command\n", argv0);
    exit(1);
}

void exit_version(char* argv0)
{
    fprintf(stderr, "%s %s\n%s\n", argv0, VERSION, "Copyright (c) 2019-2023, Vlad Me""\xc8""\x99""co\nAll rights reserved.");
    exit(1);
}

std::tuple<Range, int> ParseRange(std::string const&, int i);
std::tuple<Range, int> ParseFromComma(int base, std::string const& s, int i, bool wholeFileCandidate = false)
{
    Range temp;
    ++i;
    i = SkipWS(s, i);
    switch(s[i]) {
        case ',': case '.': case '+': case '-': case '$': case ';':
        case '0': case '1': case '2': case '3': case '4': case '\'':
        case '5': case '6': case '7': case '8': case '9':
        case '/': case '?':
            std::tie(temp, i) = ParseRange(s, i);
            return std::make_tuple(Range::R(base, temp.second), i);
        default:
            if(wholeFileCandidate)
                return std::make_tuple(Range::R(base, Range::Dollar()), i);
            else
                return std::make_tuple(Range::R(base, Range::Dot()), i);
    }
} // ParseFromComma

std::tuple<int, int> ReadNumber(std::string const& s, int i)
{
    i = SkipWS(s, i);
    int left = 0;
    while(s[i] >= '0' && s[i] <= '9') {
        left = left * 10 + s[i] - '0';
        ++i;
    }
    return std::make_tuple(left, i);
} // ReadNumber

std::tuple<Range, int> ParseFromOffset(int from, std::string const& s, int i)
{
    int left = 0;
    bool negative = (s[i] == '-');
    ++i;
    if(s[i] >= '0' && s[i] <= '9') {
        std::tie(left, i) = ReadNumber(s, i);
        left = (negative) ? from-left : from+left;
    } else if(negative) {
        left = from - 1;
        while(s[i] == '-') {
            ++i;
            --left;
        }
    } else if(!negative) {
        left = from + 1;
        while(s[i] == '+') {
            ++i;
            ++left;
        }
    } // which s[i] is [0-9]\+;-\+;+\+
    i = SkipWS(s, i);
    if(s[i] == ',') {
        return ParseFromComma(left, s, i);
    } else {
        return std::make_tuple(Range::S(left), i);
    }
} // ParseFromOffset

std::tuple<Range, int> ParseCommaOrOffset(int base, std::string const& s, int i)
{
    i = SkipWS(s, i);
    switch(s[i]) {
        case ',':
            return ParseFromComma(base, s, i);
        case '+':
        case '-':
            cprintf<CPK::parser>("Parsing offset\n");
            return ParseFromOffset(base, s, i);
        default:
            return std::make_tuple(Range::S(base), i);
    }
} // ParseCommaOrOffset

std::tuple<Range, int> ParseRegister(std::string const& s, int i)
{
    i = SkipWS(s, i);
    if(s[i] != '\'') throw JakEDException("Internal parsing error: unexpected '");
    ++i;
    if(s[i] < 'a' || s[i] > 'z') throw JakEDException("Syntax error: register reference expects a register. Registers may be lower-case ASCII characters");
    char r = s[i];
    ++i;
    auto found = g_state.registers.find(r);
    if(found == g_state.registers.end()) {
        std::stringstream ss;
        ss << "Register " << r << " is empty";
        throw JakEDException(ss.str());
    }
    auto it = g_state.swapfile.head();
    size_t index = 0;
    while(it != found->second && it) {
        cprintf<CPK::regs>("%zd [%s]\n", index+1,static_cast<std::string>(it->text()).c_str());
        cprintf<CPK::parser>("%zd [%s]\n", index+1,static_cast<std::string>(it->text()).c_str());
        it = it->next();
        ++index;
    }
    return ParseCommaOrOffset(index, s, i);
} // ParseRegister

// [fmt, G_OR_N, p]
// G_OR_N ::= 'g'               -> -1
//          | [0-9][0-9]*       -> N
//          ;
std::tuple<std::string, int, bool> ParseReplaceFormat(std::string s, int i)
{
    size_t p = std::string::npos;
    // handle escaped /
    {
        for(int ii = i; ii < s.size(); ++ii) {
            if(s[ii] == '\\') {
                ++ii;
                continue;
            }
            if(s[ii] == '/') {
                p = ii;
                break;
            }
        }
    }
    if(p == std::string::npos) {
        // in GNU ed, a lack of / at the end means as-if p was specified.
        // They also have another extension: `s/` which means `s` with `p`,
        // but I don't plan to support that because it looks silly
        std::string fmt = s.substr(i).substr(0);
        return std::make_tuple(fmt, 0, true);
    }
    std::string fmt = s.substr(i).substr(0, p); // assume format_sed actually does what I think it does
    s = s.substr(i).substr(p + 1);
    i = i + p + 1;
    s = s.substr(SkipWS(s, 0));
    int G_OR_N = 0;
    bool print = false;
    switch(s[0]) {
    case 'p':
        print = true;
        ++i;
        break; // p
    case 'g':
        G_OR_N = -1;
        ++i;
        switch(s[1]) {
        case 'p':
            print = true;
            ++i;
            break;
        }
        break; // g
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
        int j = 0;
        std::tie(G_OR_N, j) = ReadNumber(s, 0);
        i += j;
        switch(s[j]) {
        case 'p':
            print = true;
            ++i;
            break;
        }
    } break; // N
    default:
        break;
    } // switch(s[0])

    return std::make_tuple(fmt, G_OR_N, print);
} // ParseReplaceFormat

template<bool acceptNoMatches>
std::tuple<int, int> ParseRegex(std::string s, int i)
{
    if(s[0] != '/' && s[0] != '?') throw JakEDException("Internal parse error: expected regex to start with / or ?");
    ++i;
    std::stringstream regexText;
    cprintf<CPK::regex>("Grabbing expression\n");
    while(s[i] != '\0' && s[i] != '/' && s[i] != '?') {
        cprintf<CPK::regex>("at %c\n", s[i]);
        // ECMAScript is the only one that supports word boundaries
        // MSDN has a nice page outlining the capabilities (C++ regex quick ref)
        // so let's translate ECMAScript regexes to BRE
        switch(s[i]) {
        case '$':
            if(s[i+1] == '/' || s[i+1] == '?') {
                // This one I don't understand... aparent the ECMAScript
                // pre-parser (?!) makes $ mean EOL and BOL which implies there
                // is a \n in the stream. I have discovered $$ DOES mean $ in
                // normal POSIX land. I don't know, let's just roll with it
                cprintf<CPK::regex>("Translating $ to $$\n");
                regexText << "$$";
            } else {
                regexText << "\\$";
            }
            break;
        case '^':
            if(s[i-1] == '/' || s[i-1] == '?') {
                // Apparently ^ matches okay
                regexText << "^";
            } else {
                // except when it should be literal
                cprintf<CPK::regex>("Translating ^ to \\^\n");
                regexText << "\\^";
            }
            break;
        case '(':
            cprintf<CPK::regex>("Translating ( to \\(\n");
            regexText << "\\(";
            break;
        case ')':
            cprintf<CPK::regex>("Translating ) to \\)\n");
            regexText << "\\)";
            break;
        case '{':
            cprintf<CPK::regex>("Translating { to \\{\n");
            regexText << "\\{";
            break;
        case '}':
            cprintf<CPK::regex>("Translating } to \\}\n");
            regexText << "\\}";
            break;
        case '+':
            cprintf<CPK::regex>("Translating + to \\+\n");
            regexText << "\\+";
            break;
        case '?':
            cprintf<CPK::regex>("Translating ? to \\?\n");
            regexText << "\\?";
            break;
        case '\\':
            ++i;
            cprintf<CPK::regex>("Escaping %c\n", s[i]);
            if(s[i] == '\0') break;
            // translations to POSIX
            switch(s[i]) {
            case '<':
                cprintf<CPK::regex>("Translating \\< to \\b\n");
                regexText << "\\b";
                break;
            case '>':
                cprintf<CPK::regex>("Translating \\> to \\b\n");
                regexText << "\\b";
                break;
            case '(':
                cprintf<CPK::regex>("Translating \\( to (\n");
                regexText << "(";
                break;
            case ')':
                cprintf<CPK::regex>("Translating \\) to )\n");
                regexText << ")";
                break;
            case '{':
                cprintf<CPK::regex>("Translating \\{ to {\n");
                regexText << "{";
                break;
            case '}':
                cprintf<CPK::regex>("Translating \\} to }\n");
                regexText << "}";
                break;
            case '+':
                cprintf<CPK::regex>("Translating \\+ to +\n");
                regexText << "+";
                break;
            case '?':
                cprintf<CPK::regex>("Translating \\? to ?\n");
                regexText << "?";
                break;
            default:
                regexText << '\\' << s[i];
                break;
            }
            break;
        default:
            regexText << s[i];
            break;
        }
        ++i;
    } // while(s[i] != '\0' && s[i] != '/' && s[i] != '?') 

    if(!regexText.str().empty()) {
        cprintf<CPK::regex>("Text is %s\n", regexText.str().c_str());
        // ECMAScript is the only one that supports word boundaries
        // MSDN has a nice page outlining the capabilities (C++ regex quick ref)
        g_state.regexp = std::regex(regexText.str(), std::regex_constants::ECMAScript);
    } else {
        cprintf<CPK::regex>("Repeating regex\n");
    }

    auto ref = g_state.swapfile.head();
    auto fromLine = g_state.line;
    int line = 0;
    cprintf<CPK::regex>("Finding line %d\n", fromLine);
    while(ref && (fromLine--)) {
        ++line;
        ref = ref->next();
        if(!ref) ref = g_state.swapfile.head()->next();
        if(CtrlC()) {
            throw JakEDException("Interrupted");
        }
    }
    cprintf<CPK::regex>("line %d == fromLine %d, text == %s\n", line, g_state.line, static_cast<std::string>(ref->text()).c_str());
    auto it = ref->Copy();
    if(s[0] == '/') {
        cprintf<CPK::regex>("Searching forward\n");
        while(it) {
            it = it->next();
            ++line;
            if(!it) {
                cprintf<CPK::regex>("wrapping\n");
                it = g_state.swapfile.head()->next();
                line = 1;
            }
            cprintf<CPK::regex>("Checking %s\n", static_cast<std::string>(it->text()).c_str());
            if(std::regex_search(static_cast<std::string>(it->text()), g_state.regexp)) {
                cprintf<CPK::regex>("Found at %d\n", line);
                return std::make_tuple(line, i + 1);
            }
            if(CtrlC()) {
                break;
            }
            if(it == ref) {
                if constexpr(acceptNoMatches) {
                    cprintf<CPK::regex>("Reverse match not found\n");
                    return std::make_tuple(-1, i + 1);
                } else {
                    throw JakEDException("Pattern not found");
                }
            }
        }
    } else { // if s[0] != '/', i.e. it's '?'
        cprintf<CPK::regex>("Searching backwards\n");
        int lastFound = 0;
        bool flag = false;
        while(it) {
            if(it == ref) {
                if(flag) {
                    cprintf<CPK::regex>("Alreayd visited self, breaking\n");
                    break;
                }
                cprintf<CPK::regex>("Vising self once\n");
                flag = true;
            }
            cprintf<CPK::regex>("Checking %s\n", static_cast<std::string>(it->text()).c_str());
            if(std::regex_search(static_cast<std::string>(it->text()), g_state.regexp)) {
                cprintf<CPK::regex>("Found a match on line %d\n", line);
                lastFound = line;
            }
            if(CtrlC()) {
                break;
            }
            it = it->next();
            ++line;
            if(!it) {
                cprintf<CPK::regex>("wrapping\n");
                it = g_state.swapfile.head()->next();
                line = 1;
            }
        } // while(it)
        if constexpr(acceptNoMatches) {
            cprintf<CPK::regex>("Reverse match not found\n");
            return std::make_tuple(-1, i + 1);
        } else {
            if(lastFound == 0) throw JakEDException("Pattern not found");
        }
        cprintf<CPK::regex>("Sticking with match on line %d\n", lastFound);
        return std::make_tuple(lastFound, i + 1);
    } // else if s[0] != '/'

    throw::std::runtime_error("internal error");
} // ParseRegex

std::tuple<Range, int> ParseRange(std::string const& s, int i)
{
    i = SkipWS(s, i);
    int left = 0;
    switch(s[i]) {
        case '%':
            return std::make_tuple(Range::R(1, Range::Dollar()), i + 1);
        case ';':
            return ParseFromComma(Range::Dot(), s, i, true);
        case '$':
            return std::make_tuple(Range::S(Range::Dollar()), i + 1);
        case ',':
            return ParseFromComma(1, s, i, true);
        case '.':
            ++i;
            return ParseCommaOrOffset(Range::Dot(), s, i);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            std::tie(left, i) = ReadNumber(s, i);
            return ParseCommaOrOffset(left, s, i);
        }
        case '+': case '-':
            return ParseFromOffset(Range::Dot(), s, i);
        case '\'':
            return ParseRegister(s, i);
        case '/':
        case '?':
            std::tie(left, i) = ParseRegex(s, i);
            return ParseCommaOrOffset(left, s, i);
        default:
            return std::make_tuple(Range::S(Range::Dot()), i);
    } // switch(s[i])
} // ParseRange

// [Range, command, tail]
std::tuple<Range, char, std::string> ParseCommand(std::string s)
{
    size_t i = 0;
    Range r;
    i = SkipWS(s, i);
    switch(s[i]) {
        case 'j':
            r = Range::R(Range::Dot(), Range::Dot(1));
            break;
        case '\0': case 'z':
            r = Range::S(Range::Dot(1));
            break;
        case '=': case 'w': case 'W': case 'v': case 'V':
        case 'g': case 'G': case 'r': 
            r = Range::R(1, Range::Dollar());
            break;
        default:
        case '/': case '?': case '\'':
        case '+': case '-': case ',': case ';': case '$':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            std::tie(r, i) = ParseRange(s, i);
            break;
        case '!': case ':':
            r = Range::S(0); // rangeless means simple shell escape
            break;
    } // switch range first char
    switch(s[i]) {
        case '\0':
            return std::make_tuple(r, 'p', std::string());
        case 'p': case 'g': case 'G': case 'v': case 'V':
        case 'u': case 'P': case 'H': case 'h': case '#':
        case 'k': case 'i': case 'a': case 'c': case 'd':
        case 'j': case 'm': case 't': case 'y': case '!':
        case 'x': case 'r': case 'l': case 'z': case '=':
        case 'W': case 'e': case 'E': case 'f': case 'w':
        case 'q': case 'Q': case 'n': case 's': case ':':
        case '\n':
            //return std::make_tuple(r, s[i], s.substr(SkipWS(s, i+1)));
            return std::make_tuple(r, s[i], s.substr(i + 1));
        default:
            throw JakEDException("Syntax error");
    } // switch command
} // ParseCommand

void ErrorOccurred(std::exception& ex)
{
    g_state.error = true;
    g_state.diagnostic = ex.what();
    cprintf<CPK::error>("%s\n", ex.what());
    g_state.writeStringFn("?\n"); // compliance, H mode still prints out the question mark, and then prints out the message
    if(g_state.Hmode) {
        std::stringstream ss;
        ss << g_state.diagnostic << std::endl;
        g_state.writeStringFn(ss.str());
    }
    if(!ISATTY(_fileno(stdin))) CommandsImpl::Q(Range(), "");
} // ErrorOccurred

void Loop()
{
    while(1) {
        cprintf<CPK::CTRLC>("resetting ctrlc\n");
        // If we're on an actual terminal, throw a line-feed
        // in there to make it obvious the command got cancelled
        // (especially in the case where the person was mid-sentence)
        if(CtrlC() && ISATTY(fileno(stdin))) {
            cprintf<CPK::CTRLC2>("flushing\n");
            FlushConsoleInputBuffer(TheConsoleStdin);
            lastWasD = false;
            lastChar = (char)0;
            buffer.clear();
            fprintf(stdout, "\n");
        }
        if(CtrlC()) {
            // wait for the interrupt thread; it will get here eventually
            while(!ctrlc.load());
            ctrlc = false;
            // this one doesn't matter; this thread would set it, and
            // obviously it cannot do that right now.
            ctrlint = false;
        }
        if(ISATTY(fileno(stdin)) && ISATTY(fileno(stdout)) && g_state.showPrompt) {
            fprintf(stdout, "%s", g_state.PROMPT.c_str());
            fflush(stdout);
        }

        Range r;
        char command;
        std::string tail;
        try {
            do {
                std::stringstream ss;
                int ii = 0;
                while((ii = g_state.readCharFn()) != EOF) {
                    cprintf<CPK::CTRLC>("ii%d\n", ii);
                    char c = (char)(ii & 0xFF);
                    // If we got interrupted, clear the line buffer
                    if(CtrlC()) {
                        cprintf<CPK::CTRLC>("interrupted after reading ii\n");
                        ss.str("");
                        break;
                    }
                    cprintf<CPK::parser>("Read %x\n", c);
                    if(c == '\n') break;
                    ss << c;
                } // read loop
                // If we got interrupted, start a new loop
                if(CtrlC()) {
                    cprintf<CPK::CTRLC>("Interrupted after read loop\n");
                }
                auto s = ss.str();

                if( ii == EOF && s.empty()) {
                    cprintf<CPK::CTRLC>("s is empty\n");
                    command = 'Q';
                } else {
                    std::tie(r, command, tail) = ParseCommand(s);
                    cprintf<CPK::CTRLC>("parsed command\n");
                }
                // If we got interrupted, start a new loop
                if(CtrlC()) break;
                cprintf<CPK::CTRLC>("Will execute %c\n", command);
                Commands.at(command)(r, tail);
                g_state.diagnostic = "";
            } while(0);
#ifdef JAKED_TEST
        } catch(test_error& e) {
            std::rethrow_exception(std::current_exception());
#endif
        } catch(application_exit& e) {
            std::rethrow_exception(std::current_exception());
        } catch(std::exception& ex) {
            ErrorOccurred(ex);
        } catch(...) {
            std::runtime_error ex("???");
            ErrorOccurred(ex);
        }
    } // while(1)
} // Loop

BOOL consoleHandler(DWORD signal)
{
    bool falsy = false;
    switch(signal) {
        case CTRL_CLOSE_EVENT: 
            quick_exit(127);
        case CTRL_BREAK_EVENT:
        case CTRL_C_EVENT:
            // Use LOCK CMPXCHG because it's a race between
            // whatever thread this is and the main thread.
            // I don't know why, ask Windows...
            ctrlc = true;
            return TRUE;
        default:
            return FALSE;
    }
} // consoleHandler

void ParseArgs(int& argc, char**& argv)
{
    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            for(int j = 1; j < strlen(argv[i]); ++j) {
                switch(argv[i][j]) {
                    case 's':
                        g_state.suppress = true;
                        break;
                    case 'p':
                        g_state.showPrompt = true;
                        if(j + 1 < strlen(argv[i])) {
                            ++j;
                            std::stringstream ss;
                            ss << argv[i][j] << ' ';
                            g_state.PROMPT = ss.str();
                        } else {
                            ++i;
                            if(i >= argc) {
                                exit_usage("bad arguments", argv[0]);
                            } else {
                                g_state.PROMPT = argv[i];
                                j = strlen(argv[i]);
                            }
                        }
                        break;
                    case 'W':
                        g_state.lineEndings = LineEndings::CrLf;
                        break;
                    case 'U':
                        g_state.lineEndings = LineEndings::Lf;
                        break;
                    case 'd':
#ifdef JAKED_DEBUG
                        __debugbreak();
#endif
                        break;
                    case 'r':
                        exit_usage("-r not implemented", argv[0]);
                    case 'V':
                        exit_version(argv[0]);
                    case 'h':
                        exit_usage(nullptr, argv[0]);
                    default:
                        exit_usage("bad arguments", argv[0]);
                }
            }
        } else {
            std::string file = argv[i];
            g_state.filename = file.substr(SkipWS(file, 0));
        }
    }
} // ParseArgs

int main(int argc, char* argv[])
{
    //TheConsoleStdin = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    TheConsoleStdin = GetStdHandle(STD_INPUT_HANDLE);

    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)&consoleHandler, TRUE)) {
        fprintf(stderr,"INTERNAL ERROR: Could not initialize windows control handler\nContinuing anyway.\nWARNING: CTRL-C will exit the application");
    }

    if(ISATTY(_fileno(stdout))) {
        oldConsoleOutputCP = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
    }
    // Interactive_readCharFn needs to be updated to work with ReadConsoleW
    // and WideCharToMultiByte
    if(ISATTY(_fileno(stdin))) {
        oldConsoleInputCP = GetConsoleCP();
        SetConsoleCP(CP_UTF8);
    }
    at_quick_exit(RestoreConsoleCP);

    ParseArgs(argc, argv);

    g_state.readCharFn = Interactive_readCharFn;
    g_state.writeStringFn = &Interactive_writeStringFn;

    if(!g_state.filename.empty()) {
        try {
            Commands.at('r')(Range::ZERO(), g_state.filename);
            g_state.dirty = false;
            if(g_state.filename[0] == '!') {
                g_state.filename = "";
            }
        } catch(JakEDException& ex) {
            ErrorOccurred(ex);
        } catch(std::exception& ex) {
            ErrorOccurred(ex);
        } catch(...) {
            std::runtime_error ex("???");
            ErrorOccurred(ex);
        }
    }

    try {
        Loop();
    } catch(application_exit& ae) {
        exit(ae.m_error);
    }
    return 0;
}
