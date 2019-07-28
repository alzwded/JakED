/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "swapfile.h"

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
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

#ifndef ISATTY
# define ISATTY(X) _isatty(X)
#endif

#include "cprintf.h"

HANDLE TheConsoleStdin = NULL;

volatile std::atomic<bool> ctrlc = false;
volatile std::atomic<bool> ctrlint = false;
inline bool CtrlC()
{
    return (ctrlc = ctrlc.load())
        || (ctrlint = ctrlint.load());
}

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
        , decltype(regexp) _regexp = std::regex{".", std::regex_constants::ECMAScript})
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
        , decltype(regexp) _regexp = std::regex{".", std::regex_constants::ECMAScript})
    {
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
    }

} g_state;

static int lastWasD = 0;
static char lastChar = '\0';
int Interactive_readCharFn()
// WOW, this is a long story...
{
    if(lastWasD == 2) {
        lastWasD = 0;
        return lastChar;
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
        return fgetc(stdin);
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
        auto success = ReadConsoleA(
                TheConsoleStdin,
                &c,
                1,
                &rv,
                NULL);
        cprintf<CPK::CTRLC2>("rv %lu c %d\n", rv, c);
        cprintf<CPK::CTRLC>("rv %lu c %d\n", rv, c);
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
        }
        if(GetLastError() != 0) {
            cprintf<CPK::CTRLC2>("GLE: %lu\n", GetLastError());
        }
        // In the event that ReadConsoleA fails (I never saw it fail yet)
        // then report some error and exit.
        if(!success) {
            fprintf(stderr, "ReadConsoleA failed with %lu\n", GetLastError());
            return EOF;
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

        cprintf<CPK::CTRLC>("    read a %x\n", c);
        // Okay, if ctrlc is triggered, return some BS character.
        // The caller will handle the flag.
        if(CtrlC()) {
            cprintf<CPK::CTRLC>("from ctrlc, returning NUL\n");
            return '\0';
        }
        // If STDIN got closed, return EOF.
        if(feof(stdin)) {
            cprintf<CPK::CTRLC>("lost stdin!!! my magic didn't work\n");
            return EOF;
        }
        cprintf<CPK::CTRLC>("read %c\n", c);
        // Finally, return the character
        return c;
    } while(1);
}

void Interactive_writeStringFn(std::string const& s)
{
    fprintf(stdout, "%s", s.c_str());
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
        if(_2 < _1) throw std::runtime_error("Backwards range");
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
};

int SkipWS(std::string const& s, int i)
{
    while(s[i] == ' ' || s[i] == '\t') ++i;
    return i;
}

std::tuple<int, int> ReadNumber(std::string const&, int);

namespace CommandsImpl {
    void r(Range range, std::string tail)
    {
        cprintf<CPK::r>("r: n before = %zd\n", g_state.nlines);
        int nthLine = range.second;
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) tail = g_state.filename;
        if(tail.empty()) throw std::runtime_error("No such file!");
        size_t bytes = 0;
        if(tail[0] == '!') {
            throw std::runtime_error("shell execution is not implemented");
        }
        FILE* f = fopen(tail.c_str(), "r");
        struct AtEnd {
            FILE* f;
            AtEnd(FILE*& ff) : f(ff) {}
            ~AtEnd() { fclose(f); }
        } atEnd(f);
        if(f) {
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
            std::stringstream line;
            int c;
            bool bomChecked = false;
            //g_state.nlines = 0;
            auto continueFrom = after->next();
            cprintf<CPK::r>("will continue after %s with %s\n", (after) ? (after->text().c_str()) : "<EOF>", (continueFrom) ? continueFrom->text().c_str() : "<EOF>");
            while(!feof(f) && (c = fgetc(f)) != EOF) {
                if(CtrlC()) {
                    cprintf<CPK::CTRLC>("r: ctrlc invoked while reading\n");
                    cprintf<CPK::r>("r: ctrlc invoked while reading\n");
                    after->link(continueFrom);
                    return;
                }
                ++bytes;
                if(c == '\n') {
                    auto s = line.str();
                    // clear UTF8 BOM if it's there
                    if(!bomChecked && bytes >= 3
                            && s.find("\xEF" "\xBB" "\xBF") == 0)
                    {
                        s = s.substr(3);
                        bomChecked = true;
                    }
                    auto inserted = g_state.swapfile.line(s);
                    cprintf<CPK::r>("linking %s -> %s\n", ((after) ? after->text().c_str() : "<EOF>"), (inserted->text().c_str()));
                    after->link(inserted);
                    after = inserted;
                    ++g_state.nlines;
                    cprintf<CPK::r>("> %s\n", line.str().c_str()); // FIXME some utf8 doesn't get right for some reason; might be utf16
                    line.str(std::string());
                } else {
                    line << (char)(c & 0xFF);
                }
            }
            if(line.str() != "") {
                auto inserted = g_state.swapfile.line(line.str());
                after->link(inserted);
                after = inserted;
                cprintf<CPK::r>("linking %s -> %s\n", ((after) ? after->text().c_str() : "<EOF>"), (inserted->text().c_str()));
                ++g_state.nlines;
            }
            after->link(continueFrom);
            cprintf<CPK::r>("linking %s -> %s\n", ((after) ? after->text().c_str() : "<EOF>"), (continueFrom) ? (continueFrom->text().c_str()) : "<EOF>");
            fclose(f);
            g_state.dirty = true;
            g_state.line = range.second + g_state.nlines - originalNlines;
            {
                std::stringstream output;
                output << bytes << std::endl;
                g_state.writeStringFn(output.str());
            }
            std::stringstream undoCommandBuf;
            undoCommandBuf << range.second + 1 << "," << (range.second + g_state.nlines - originalNlines) << "d";
            auto undoCommand = g_state.swapfile.line(undoCommandBuf.str());
            g_state.swapfile.undo(undoCommand);
            cprintf<CPK::r>("r: n after = %zd, line = %d\n", g_state.nlines, g_state.line);
        } else {
            throw std::runtime_error("No such file!");
        }
    }

    std::string getFileName(std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) tail = g_state.filename;
        if(tail.empty()) throw std::runtime_error("No such file!");
        return tail;
    }

    void E(Range range, std::string tail)
    {
        tail = getFileName(tail);
        if(tail[0] == '!') {
            throw std::runtime_error("shell execution is not implemented");
        } else {
            g_state.filename = tail;
        }
        g_state.registers.clear();
        auto undoBuffer = g_state.swapfile.line("1,$c");
        undoBuffer->link(g_state.swapfile.head()->next());
        g_state.swapfile.undo(undoBuffer);
        g_state.swapfile.cut(g_state.swapfile.head()->next());
        g_state.swapfile.head()->link();
        g_state.nlines = 0;
        r(Range::S(0), tail);
        g_state.dirty = false;
    }

    void e(Range range, std::string tail)
    {
        if(g_state.dirty) throw std::runtime_error("file has modifications");
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
#ifdef JAKED_TEST
        throw application_exit(g_state.error);
#endif
        exit(g_state.error);
    }

    void q(Range range, std::string tail)
    {
        if(g_state.dirty) throw std::runtime_error("file has modifications");
        return Q(range, tail);
    }

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
            ss << i->text() << std::endl;
            i = i->next();
            g_state.writeStringFn(ss.str());
            if(CtrlC()) return;
        }
        g_state.line = r.second;
    }

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
        if(tail.empty()) throw std::runtime_error("missing argument");
        if(tail[0] < 'a' || tail[0] > 'z') throw std::runtime_error("Invalid register. Registers must be a lowercase ASCII letter");
        int idx = r.second;
        if(idx < 1 || idx > g_state.nlines)
            throw std::runtime_error("Address out of bounds");
        auto it = g_state.swapfile.head();
        while(idx-- > 0
                && it->next())
        {
            it = it->next();
            //printf("Marking: %d %s\n", idx, it->text().c_str());
        }
        g_state.registers[tail[0]] = it;
    }

    void f(Range r, std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) {
            std::stringstream ss;
            ss << g_state.filename << "\n";
            g_state.writeStringFn(ss.str());
            return;
        }
        if(tail[0] == '!') throw std::runtime_error("Writing to a shell command's STDIN is not implemented");
        size_t i = tail.size();
        while(tail[i - 1] == ' ') --i;
        if(i < tail.size()) tail = tail.substr(0, i);
        g_state.filename = tail;
    }

    void EQUALS(Range r, std::string tail)
    {
        if(r.second < 1 || r.second > g_state.nlines) throw std::runtime_error("invalid range");
        std::stringstream ss;
        ss << r.second << std::endl;
        g_state.writeStringFn(ss.str());
    }

    void commonW(Range r, std::string fname, const char* mode)
    {
        cprintf<CPK::W>("R: [%d, %d]\n", r.first, r.second);
        if(r.first < 1 || r.second < 1 || r.first > g_state.nlines || r.second > g_state.nlines) throw std::runtime_error("Invalid range");
        fname = getFileName(fname);
        FILE* f;
        if(fname[0] == '!') throw new std::runtime_error("Writing to pipe not implemented");
        else {
            f = fopen(fname.c_str(), mode);
            if(!f) throw std::runtime_error("Cannot open file for writing");
            if(g_state.filename.empty()) g_state.filename = fname;
        }

        size_t nBytes = 0;

        auto i1 = g_state.swapfile.head();
        auto first = r.first, second = r.second;
        while(first-- > 0) {
            cprintf<CPK::W>("skipping %s\n", (i1) ? i1->text().c_str() : "<EOF>");
            i1 = i1->next();
        }
        if(i1) cprintf<CPK::W>("Writing from %s\n", i1->text().c_str());
        while(second-- > 0 && i1) {
            if(i1) cprintf<CPK::W>("Writing %s\n", i1->text().c_str());
            nBytes += i1->length() + strlen("\n");
            fprintf(f, "%s\n", i1->text().c_str());
            i1 = i1->next();
        }
        fclose(f);
        g_state.swapfile.Rebuild();
        g_state.dirty = false;
        std::stringstream ss;
        ss << nBytes << std::endl;
        g_state.writeStringFn(ss.str());
    }

    void w(Range r, std::string tail)
    {
        bool doQuit = false;
        if(tail[0] == 'q') {
            doQuit = true;
            tail = tail.substr(SkipWS(tail, 1));
        }
        commonW(r, tail, "w");
        if(doQuit) q(r, "");
    }

    void W(Range r, std::string tail)
    {
        bool doQuit = false;
        if(tail[0] == 'q') {
            doQuit = true;
            tail = tail.substr(SkipWS(tail, 1));
        }
        commonW(r, tail, "a");
        if(doQuit) q(r, "");
    }

    void z(Range r, std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
        if(!tail.empty()) {
            int newWindowSize = 0;
            int i = 0;
            std::tie(newWindowSize, i) = ReadNumber(tail, 0);
            if(i == 0 || newWindowSize <= 0) throw std::runtime_error("Parse error: invalid window size");
            g_state.zWindow = newWindowSize;
        }
        p(Range::R(r.second, r.second + g_state.zWindow - 1), "");
    }

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
            char c = g_state.readCharFn();
            cprintf<CPK::a>("read a %x %c\n", c, c);
            cprintf<CPK::a>("ss = %s\n", ss.str().c_str());
            if(CtrlC()) return;
            if(c == '\n') {
                if(ss.str() == ".") {
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
        }
        if(nLines) {
            after->link(linkMeAtTheEnd);
            g_state.dirty = true;
            g_state.line = r.second + nLines;
            ss.str("");
            ss << r.second + 1 << "," << (r.second + nLines) << "d";
            auto inserted = g_state.swapfile.line(ss.str());
            g_state.swapfile.undo(inserted);
        }
    }

    void i(Range r, std::string)
    {
        auto pos = r.second;
        if(pos < 1 || pos >= g_state.nlines) throw std::runtime_error("Invalid range");
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

        size_t linesDeleted = 1;
        auto it = g_state.swapfile.head();
        auto idx = r.first;
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
            for(auto kv = g_state.registers.begin(); kv != g_state.registers.end(); ++kv) {
                if(kv->second
                        && kv->second == it)
                {
                    cprintf<CPK::regs>("marking %c to erase\n", kv->first);
                    cprintf<CPK::d>("marking %c to erase\n", kv->first);
                    toErase[kv->first - 'a'] = true;
                }
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
        undoCommand << r.first << "i";
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

        g_state.dirty = true;
        g_state.line = r.first;
        g_state.nlines -= linesDeleted;
    }

    void d(Range r, std::string)
    {
        return deleteLines(r, true);
    }

    void j(Range r, std::string tail)
    {
        if(r.first >= r.second) throw std::runtime_error("j(oin) needs a range of at least two lines");
        if(r.first < 1) throw std::runtime_error("Invalid range");
        deleteLines(r, false);
        auto oldLines = g_state.swapfile.undo()->next();
        auto oldLinesDup = (oldLines) ? oldLines->Copy() : LinePtr();
        std::stringstream ss;
        while(oldLines) {
            cprintf<CPK::j>("%s\n", oldLines->text().c_str());
            ss << oldLines->text();
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
    }

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
    }


}

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
};

void exit_usage(char* msg, char* argv0)
{
    fprintf(stderr, "%s\n", msg);
    fprintf(stderr, "Usage: %s FILE\nIf FILE begins with a bang, it will be executed as a command\n", argv0);
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
}

std::tuple<int, int> ReadNumber(std::string const& s, int i)
{
    i = SkipWS(s, i);
    int left = 0;
    while(s[i] >= '0' && s[i] <= '9') {
        left = left * 10 + s[i] - '0';
        ++i;
    }
    return std::make_tuple(left, i);
}

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
    }
    i = SkipWS(s, i);
    if(s[i] == ',') {
        return ParseFromComma(left, s, i);
    } else {
        return std::make_tuple(Range::S(left), i);
    }
}

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

}

std::tuple<Range, int> ParseRegister(std::string const& s, int i)
{
    i = SkipWS(s, i);
    if(s[i] != '\'') throw std::runtime_error("Internal parsing error: unexpected '");
    ++i;
    if(s[i] < 'a' || s[i] > 'z') throw std::runtime_error("Syntax error: register reference expects a register. Registers may be lower-case ASCII characters");
    char r = s[i];
    ++i;
    auto found = g_state.registers.find(r);
    if(found == g_state.registers.end()) {
        std::stringstream ss;
        ss << "Register " << r << " is empty";
        throw std::runtime_error(ss.str());
    }
    auto it = g_state.swapfile.head();
    size_t index = 0;
    while(it != found->second && it) {
        cprintf<CPK::regs>("%zd [%s]\n", index+1,it->text().c_str());
        cprintf<CPK::parser>("%zd [%s]\n", index+1,it->text().c_str());
        it = it->next();
        ++index;
    }
    return ParseCommaOrOffset(index, s, i);
}

std::tuple<int, int> ParseRegex(std::string s, int i)
{
    if(s[0] != '/' && s[0] != '?') throw std::runtime_error("Internal parse error: expected regex to start with / or ?");
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
            // This one I don't understand... aparent the ECMAScript
            // pre-parser (?!) makes $ mean EOL and BOL which implies there
            // is a \n in the stream. I have discovered $$ DOES mean $ in
            // normal POSIX land. I don't know, let's just roll with it
            cprintf<CPK::regex>("Translating $ to $$\n");
            regexText << "$$";
            break;
#if 0
        // Apparently ^ matches okay, this is not needed
        case '^':
            cprintf<CPK::regex>("Translating ^ to ^^n");
            regexText << "^^";
            break;
#endif
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
    }

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
    while(ref && fromLine--) {
        ++line;
        ref = ref->next();
        if(!ref) ref = g_state.swapfile.head()->next();
        if(CtrlC()) {
            throw std::runtime_error("Interrupted");
        }
    }
    cprintf<CPK::regex>("line %d == fromLine %d, text == %s\n", line, g_state.line, ref->text().c_str());
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
            cprintf<CPK::regex>("Checking %s\n", it->text().c_str());
            if(std::regex_search(it->text(), g_state.regexp)) {
                cprintf<CPK::regex>("Found at %d\n", line);
                return std::make_tuple(line, i + 1);
            }
            if(CtrlC()) {
                break;
            }
            if(it == ref) throw std::runtime_error("Pattern not found");
        }
    } else {
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
            cprintf<CPK::regex>("Checking %s\n", it->text().c_str());
            if(std::regex_search(it->text(), g_state.regexp)) {
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
        }
        if(lastFound == 0) throw std::runtime_error("Pattern not found");
        cprintf<CPK::regex>("Sticking with match on line %d\n", lastFound);
        return std::make_tuple(lastFound, i + 1);
    }

    throw::std::runtime_error("internal error");
}

std::tuple<Range, int> ParseRange(std::string const& s, int i)
{
    i = SkipWS(s, i);
    int left = 0;
    switch(s[i]) {
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
    }
}

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
    }
    switch(s[i]) {
        case '\0':
            return std::make_tuple(r, 'p', std::string());
        case 'p': case 'g': case 'G': case 'v': case 'V':
        case 'u': case 'P': case 'H': case 'h': case '#':
        case 'k': case 'i': case 'a': case 'c': case 'd':
        case 'j': case 'm': case 't': case 'y': case '!':
        case 'x': case 'r': case 'l': case 'z': case '=':
        case 'W': case 'e': case 'E': case 'f': case 'w':
        case 'q': case 'Q': case 'n':
        case '\n':
            //return std::make_tuple(r, s[i], s.substr(SkipWS(s, i+1)));
            return std::make_tuple(r, s[i], s.substr(i + 1));
        default:
            throw std::runtime_error("Syntax error");
    }
}

void ErrorOccurred(std::exception& ex)
{
    g_state.error = true;
    g_state.diagnostic = ex.what();
    cprintf<CPK::error>("%s\n", ex.what());
    if(g_state.Hmode) {
        std::stringstream ss;
        ss << g_state.diagnostic << std::endl;
        g_state.writeStringFn(ss.str());
    } else {
        g_state.writeStringFn("?\n");
    }
    if(!ISATTY(_fileno(stdin))) CommandsImpl::Q(Range(), "");
}

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
                }
                // If we got interrupted, start a new loop
                if(CtrlC()) break;
                cprintf<CPK::CTRLC>("Interrupted after read loop\n");
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
        } catch(application_exit& e) {
            std::rethrow_exception(std::current_exception());
#endif
        } catch(std::exception& ex) {
            ErrorOccurred(ex);
        } catch(...) {
            std::runtime_error ex("???");
            ErrorOccurred(ex);
        }
    }
}

BOOL consoleHandler(DWORD signal)
{
    bool falsy = false;
    switch(signal) {
        case CTRL_CLOSE_EVENT: cprintf<CPK::CTRLC>("a\n");
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
}

int main(int argc, char* argv[])
{
    //TheConsoleStdin = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    TheConsoleStdin = GetStdHandle(STD_INPUT_HANDLE);

    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)&consoleHandler, TRUE)) {
        fprintf(stderr,"INTERNAL ERROR: Could not initialize windows control handler\nContinuing anyway.\nWARNING: CTRL-C will exit the application");
    }

    if(argc == 1) {
        exit_usage("No such file!", argv[0]);
    }

    if(argc > 2) {
        exit_usage("Too many arguments!", argv[0]);
    }

#ifdef JAKED_DEBUG
    if(std::string(argv[1]) == "-d") {
        __debugbreak();
    }
#endif

    g_state.readCharFn = &Interactive_readCharFn;
    g_state.writeStringFn = &Interactive_writeStringFn;

    std::string file = argv[1];
    if(file.empty()) exit_usage("No such file!", argv[0]);
    Commands.at('r')(Range::ZERO(), file);

    Loop();
    return 0;
}
