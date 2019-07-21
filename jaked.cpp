/*
Copyright 2019 Vlad Meșco

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
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

#ifndef ISATTY
# define ISATTY(X) _isatty(X)
#endif

#define JAKED_DEBUG_CTRL_C
#undef JAKED_DEBUG_CTRL_C

HANDLE TheConsoleStdin = NULL;

// Note to self: if(ctrlc = ctrlc.load()) actually makes sure we end up seeing the value everywhere, otherwise it gets a random value in one of the threads
// There are different memory models, e.g. acquire & release with some
// strict rules thrown in there. I basically want a full memory barrier
// around this flag (because it's easy and the only other thread ever
// is the consoleHandler)
// If one just if(ctrlc), the memory gets acquired, but since there's
// no explicit release, our thread is left with that value, which should
// have been updated by the other thread. Assigning it back is a straight
// forward way to release it, despite shennanigans going on in the CPU
// cache line or w/e
std::atomic<bool> ctrlc = false;
struct {
    std::string filename;
    int line;
    std::string PROMPT = "* ";
    std::list<std::string> lines;
    std::map<char, decltype(lines)::iterator> registers;
    std::string diagnostic;
    bool dirty = false;
    std::function<char()> readCharFn;
    std::function<void(std::string)> writeStringFn;
    bool error = false;
    bool Hmode = false;
    int zWindow = 1;
    bool showPrompt = false;
} g_state;

int Interactive_readCharFn()
// WOW, this is a long story...
{
    static int lastWasD = 0;
    static char lastChar = '\0';
    if(lastWasD == 2) {
        lastWasD = 0;
        return lastChar;
    }
    // So, if there is no STDIN, just return EOF
    if(feof(stdin)) {
#ifdef JAKED_DEBUG_CTRL_C
        fprintf(stderr, "stdin was closed somehow\n");
#endif
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
        auto success = ReadConsoleA(
                TheConsoleStdin,
                &c,
                1,
                &rv,
                NULL);
        //printf("rv %d c %d\n", rv, c);
        // You can probably guess success == true and rv = 0,
        // but w/e. If that happened...
        if(GetLastError() == ERROR_OPERATION_ABORTED) {
            lastWasD = 0;
            // Clear GetLastError() because as we know nobody
            // ever does.
            SetLastError(0);
            // And try to LOCK CMPXCHG to true since it is
            // entirely unpredictable if the consoleHandler
            // Thread or the main Thread get here first.
            bool falsy = false;
            //Sleep(1);
            ctrlc.compare_exchange_strong(falsy, true);
        }
        // In the event that ReadConsoleA fails (I never saw it fail yet)
        // then report some error and exit.
        if(!success) {
            fprintf(stderr, "ReadConsoleA failed with %d\n", GetLastError());
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
            lastWasD = true;
            continue;
        }

#ifdef JAKED_DEBUG_CTRL_C
        fprintf(stderr, "    read a %x\n", c);
#endif
        // Okay, if ctrlc is triggered, return some BS character.
        // The caller will handle the flag.
        if(ctrlc = ctrlc.load()) {
#ifdef JAKED_DEBUG_CTRL_C
            fprintf(stderr, "from ctrlc, returning NUL\n");
#endif
            return '\0';
        }
        // If STDIN got closed, return EOF.
        if(feof(stdin)) {
#ifdef JAKED_DEBUG_CTRL_C
            fprintf(stderr, "lost stdin!!! my magic didn't work\n");
#endif
            return EOF;
        }
#ifdef JAKED_DEBUG_CTRL_C
        fprintf(stderr, "read %c\n", c);
#endif
        // Finally, return the character
        return c;
    } while(1);
}

void Interactive_writeStringFn(std::string const& s)
{
    printf("%s", s.c_str());
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
    static int Dollar(int offset = 0) { return g_state.lines.size() + offset; }
    static int Reg(char c) {
        auto found = g_state.registers.find(c);
        if(found == g_state.registers.end()) {
            std::stringstream ss;
            ss << "Nothing in register " << c;
            throw std::runtime_error(ss.str());
        }
        return std::distance(g_state.lines.begin(), found->second);
    }
    static int Reg(char c, int offset) {
        return Reg(c) + offset;
    }
    static Range ZERO() {
        return Range(0, 0);
    }

    static Range R(int _1, int _2)
    {
        if(_1 < 0) _1 = 0;
        if(_2 < 0) _2 = 0;
        if(_1 > g_state.lines.size()) _1 = g_state.lines.size();
        if(_2 > g_state.lines.size()) _2 = g_state.lines.size();
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
        tail = tail.substr(SkipWS(tail, 0));
        if(tail.empty()) tail = g_state.filename;
        if(tail.empty()) throw std::runtime_error("No such file!");
        size_t bytes = 0;
        if(tail[0] == '!') {
            throw std::runtime_error("shell execution is not implemented");
        }
        FILE* f = fopen(tail.c_str(), "r");
        if(f) {
            auto after = g_state.lines.begin();
            std::advance(after, range.second);
            std::stringstream line;
            int c;
            while(!feof(f) && (c = fgetc(f)) != EOF) {
                ++bytes;
                if(c == '\n') {
                    after = g_state.lines.insert(after, line.str());
                    //printf("> %s\n", line.str().c_str()); // FIXME some utf8 doesn't get right for some reason; might be utf16
                    ++after;
                    line.str(std::string());
                } else {
                    line << (char)(c & 0xFF);
                }
            }
            if(line.str() != "") {
                g_state.lines.insert(after, line.str());
                ++after;
            }
            fclose(f);
            g_state.dirty = true;
            g_state.line = (int)std::distance(g_state.lines.begin(), after);
            {
                std::stringstream output;
                output << bytes << std::endl;
                g_state.writeStringFn(output.str());
            }
            // clear UTF8 BOM if it was there
            auto firstLine = g_state.lines.begin();
            std::advance(firstLine, range.second /* -1 + 1 */);
            if(bytes > 0 && firstLine->find("\xEF" "\xBB" "\xBF") == 0) {
                *firstLine = firstLine->substr(3);
            }
            //printf("%zd %d", g_state.lines.size(), g_state.line);
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
        g_state.lines.clear();
        g_state.registers.clear();
        r(Range::S(0), tail);
        g_state.dirty = false;
        if(g_state.lines.empty()) g_state.lines.emplace_back();
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
        //printf("in Q %d\n", g_state.error);
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
        int second = std::max(1, r.second);
        auto a = g_state.lines.begin();
        auto b = g_state.lines.begin();
        std::advance(a, first - 1);
        std::advance(b, second);
        for(auto it = a; it != b; ++it) {
            std::stringstream ss;
            if(tail[0] == 'n') {
                ss << first++ << '\t';
            }
            ss << *it << std::endl;
            g_state.writeStringFn(ss.str());
            if(ctrlc = ctrlc.load()) return;
        }
        g_state.line = std::distance(g_state.lines.begin(), b);
    }

    void n(Range r, std::string tail)
    {
        int first = std::max(1, r.first);
        int second = std::max(1, r.second);
        auto a = g_state.lines.begin();
        auto b = g_state.lines.begin();
        std::advance(a, first - 1);
        std::advance(b, second);
        for(auto it = a; it != b; ++it) {
            std::stringstream ss;
            ss << first++ << '\t';
            ss << *it << std::endl;
            g_state.writeStringFn(ss.str());
            if(ctrlc = ctrlc.load()) return;
        }
        g_state.line = std::distance(g_state.lines.begin(), b);
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
        if(idx < 1 || idx > g_state.lines.size())
            throw std::runtime_error("Address out of bounds");
        auto it = g_state.lines.begin();
        std::advance(it, (idx - 1));
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
        if(r.second < 1 || r.second > g_state.lines.size()) throw std::runtime_error("invalid range");
        std::stringstream ss;
        ss << r.second << std::endl;
        g_state.writeStringFn(ss.str());
    }

    void commonW(Range r, std::string fname, const char* mode)
    {
        if(r.first < 1 || r.second < 1 || r.first > g_state.lines.size() || r.second > g_state.lines.size()) throw std::runtime_error("Invalid range");
        fname = getFileName(fname);
        FILE* f;
        if(fname[0] == '!') throw new std::runtime_error("Writing to pipe not implemented");
        else {
            f = fopen(fname.c_str(), mode);
            if(!f) throw std::runtime_error("Cannot open file for writing");
            if(g_state.filename.empty()) g_state.filename = fname;
        }

        size_t nBytes = 0;

        auto i1 = g_state.lines.begin(), i2 = g_state.lines.begin();
        std::advance(i1, r.first - 1);
        std::advance(i2, r.second);
        for(; i1 != i2; ++i1) {
            nBytes += i1->size() + strlen("\n");
            fprintf(f, "%s\n", i1->c_str());
        }
        fclose(f);
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

    void i(Range r, std::string)
    {
        auto before = g_state.lines.begin();
        std::advance(before, std::max(1, r.second) - 1);
        std::list<std::string> lines;
        std::stringstream ss;
        while(1) {
            char c = g_state.readCharFn();
            //printf("read a %x %c\n", c, c);
            //printf("ss = %s\n", ss.str().c_str());
            if(ctrlc = ctrlc.load()) return;
            if(c == '\n') {
                if(ss.str() == ".") {
                    //printf("broke at .\n");
                    break;
                }
                //printf("pushing line\n");
                lines.push_back(ss.str());
                ss.str("");
                continue;
            }
            //printf("pushing char\n");
            ss << c;
        }
        g_state.lines.insert(before, lines.begin(), lines.end());
        if(lines.size()) {
            g_state.dirty = true;
            g_state.line = r.second + lines.size() - 1;
        }
    }

    void a(Range r, std::string)
    {
        return i(Range::S(r.second + 1), "");
    }

    void c(Range r, std::string)
    {
        auto i1 = g_state.lines.begin(),
             i2 = g_state.lines.begin();
        std::advance(i1, r.first - 1);
        std::advance(i2, r.second);
        // clobber registers
        bool toErase[26] = { false, false, false,
            false, false, false, false, false,
            false, false, false, false, false, false,
            false, false, false, false, false, false,
            false, false, false, false, false, false};
        for(auto&& kv : g_state.registers) {
            //printf("checking r%c\n", kv.first);
            for(auto i = i1; i != i2; ++i) {
                if(ctrlc = ctrlc.load()) return;
                if(kv.second == i) {
                    //printf("marked r%c\n", kv.first);
                    toErase[kv.first - 'a'] = true;
                    continue;
                }
            }
        }
        for(char c = 'a'; c != 'z'; ++c) {
            if(toErase[c - 'a']) {
                //printf("erasing r%c\n", c);
                g_state.registers.erase(g_state.registers.find(c));
            }
        }
        g_state.lines.erase(i1, i2);
        //printf("executing %da\n", r.first - 1);
        return a(Range::S(r.first - 1), "");
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
};

void exit_usage(char* msg, char* argv0)
{
    printf("%s\n", msg);
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
    return ParseCommaOrOffset(std::distance(g_state.lines.begin(), found->second) + 1, s, i);
}

std::tuple<Range, int> ParseRange(std::string const& s, int i)
{
    i = SkipWS(s, i);
    int left = 0;
    switch(s[i]) {
        case ';':
            return std::make_tuple(Range::R(Range::Dot(), Range::Dollar()), i + 1);
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
        default:
            return std::make_tuple(Range::S(Range::Dot()), i);
    }
}

// [Range, command, tail]
std::tuple<Range, char, std::string> ParseCommand(std::string s)
{
    size_t i = 0;
    Range r;
    bool whitespacing = true;
    i = SkipWS(s, i);
    switch(s[i]) {
        case '\0': case 'z':
            r = Range::S(Range::Dot(1));
            break;
        case '=': case 'w': case 'W': case 'v': case 'V':
        case 'g': case 'G': case 'r': 
            r = Range::R(1, Range::Dollar());
            break;
        default:
        case '\'': case '/':
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
    //printf("%s\n", ex.what());
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
#ifdef JAKED_DEBUG_CTRL_C
        fprintf(stderr, "resetting ctrlc\n");
#endif
        // If we're on an actual terminal, throw a line-feed
        // in there to make it obvious the command got cancelled
        // (especially in the case where the person was mid-sentence)
        if(ctrlc/*acq*/ && ISATTY(fileno(stdin))) {
            FlushConsoleInputBuffer(TheConsoleStdin);
            printf("\n");
        }
        // Brutally set ctrlc to false since everything that needed
        // to be handled was. In case someone was spamming ^C,
        // w/e, that's a weird edge case I don't care about.
        // A.K.A. I heard you the first time.
        ctrlc = false; /*rel*/
        if(ISATTY(fileno(stdin)) && ISATTY(fileno(stdout)) && g_state.showPrompt) {
            printf("%s", g_state.PROMPT.c_str());
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
#ifdef JAKED_DEBUG_CTRL_C
                    fprintf(stderr, "ii%d\n", ii);
#endif
                    char c = (char)(ii & 0xFF);
                    // If we got interrupted, clear the line buffer
                    if(ctrlc = ctrlc.load()) {
#ifdef JAKED_DEBUG_CTRL_C
                        fprintf(stderr, "qwe\n");
#endif
                        ss.str("");
                        break;
                    }
                    //printf("Read %x\n", c);
                    if(c == '\n') break;
                    ss << c;
                }
                // If we got interrupted, start a new loop
                if(ctrlc = ctrlc.load()) break;
#ifdef JAKED_DEBUG_CTRL_C
                        printf("ftw\n");
#endif
                auto s = ss.str();

                if( ii == EOF && s.empty()) {
#ifdef JAKED_DEBUG_CTRL_C
                        fprintf(stderr, "s is empty\n");
#endif
                    command = 'Q';
                } else {
                    std::tie(r, command, tail) = ParseCommand(s);
#ifdef JAKED_DEBUG_CTRL_C
                        fprintf(stderr, "parsed command\n");
#endif
                }
                // If we got interrupted, start a new loop
                if(ctrlc = ctrlc.load()) break;
#ifdef JAKED_DEBUG_CTRL_C
                fprintf(stderr, "Will execute %c\n", command);
#endif
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
        case CTRL_CLOSE_EVENT: printf("a\n");
        case CTRL_BREAK_EVENT:
        case CTRL_C_EVENT:
            // Use LOCK CMPXCHG because it's a race between
            // whatever thread this is and the main thread.
            // I don't know why, ask Windows...
            //ctrlc.compare_exchange_strong(falsy, true);
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
    Commands.at('E')(Range::ZERO(), file);

    Loop();
    return 0;
}
