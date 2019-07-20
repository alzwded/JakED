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

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

#ifndef ISATTY
# define ISATTY(X) _isatty(X)
#endif

int Interactive_readCharFn()
{
    if(feof(stdin)) return EOF;
    return fgetc(stdin);
}

void Interactive_writeStringFn(std::string const& s)
{
    printf("%s", s.c_str());
}

volatile DWORD ctrlc = 0;
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
} g_state;

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
            g_state.dirty = true;
            g_state.line = (int)std::distance(g_state.lines.begin(), after);
            {
                std::stringstream output;
                output << bytes << std::endl;
                g_state.writeStringFn(output.str());
            }
            //printf("%zd %d", g_state.lines.size(), g_state.line);
        } else {
            //g_state.writeStringFn("No such file!\n");
            throw std::runtime_error("No such file!");
        }
    }

    void E(Range range, std::string tail)
    {
        tail = tail.substr(SkipWS(tail, 0));
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

    void p(Range r, std::string)
    {
        int first = std::min(1, r.first);
        int second = std::min(1, r.second);
        auto a = g_state.lines.begin();
        auto b = g_state.lines.begin();
        std::advance(a, r.first - 1);
        std::advance(b, r.second);
        for(auto it = a; it != b; ++it) {
            std::stringstream ss;
            ss << *it << std::endl;
            g_state.writeStringFn(ss.str());
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
        if(tail.empty()) {
            std::stringstream ss;
            ss << g_state.filename << "\n";
            g_state.writeStringFn(ss.str());
            return;
        }
        tail = tail.substr(SkipWS(tail, 0));
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
}

std::map<char, std::function<void(Range, std::string)>> Commands = {
    { 'p', &CommandsImpl::p },
    { 'e', &CommandsImpl::e },
    { 'E', &CommandsImpl::e },
    { 'r', &CommandsImpl::r },
    { 'q', &CommandsImpl::q },
    { 'Q', &CommandsImpl::Q },
    { 'h', &CommandsImpl::h },
    { 'H', &CommandsImpl::H },
    { '#', &CommandsImpl::NOP },
    { 'k', &CommandsImpl::k },
    { '=', &CommandsImpl::EQUALS },
    { 'f', &CommandsImpl::f },
};

void exit_usage(char* msg, char* argv0)
{
    printf("%s\n", msg);
    fprintf(stderr, "Usage: %s FILE\nIf FILE begins with a bang, it will be executed as a command\n", argv0);
    exit(1);
}

std::tuple<Range, int> ParseRange(std::string const&, int i);
std::tuple<Range, int> ParseFromComma(int base, std::string const& s, int i)
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
            return std::make_tuple(Range::R(base, Range::Dollar()), i);
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
            return ParseFromComma(1, s, i);
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
        case 'q': case 'Q':
        case '\n':
            return std::make_tuple(r, s[i], s.substr(SkipWS(s, i+1)));
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
        Range r;
        char command;
        std::string tail;
        try {
            do {
                std::stringstream ss;
                if(ctrlc) return;
                int ii = 0;
                while((ii = g_state.readCharFn()) != EOF) {
                    char c = (char)(ii & 0xFF);
                    //printf("Read %x\n", c);
                    if(c == '\n') break;
                    ss << c;
                    if(ctrlc) return;
                }
                auto s = ss.str();
                if(ctrlc) break;

                if( ii == EOF && s.empty()) {
                    command = 'Q';
                } else {
                    std::tie(r, command, tail) = ParseCommand(s);
                }
                if(ctrlc) break;
                //fprintf(stderr, "Will execute %c\n", command);
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

        while(InterlockedCompareExchange(&ctrlc, 0, 1) != 0){};
    }
}

BOOL WINAPI consoleHandler(DWORD signal)
{
    switch(signal) {
        case CTRL_C_EVENT:
            while(InterlockedCompareExchange(&ctrlc, 1, 0) != 1){};
        default:
            return TRUE;
    }
}

int main(int argc, char* argv[])
{
    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
    if(console) {
        DWORD mode;
        GetConsoleMode(console, &mode);
        if(SetConsoleMode(console, mode|ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT)) {
            if(!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
                fprintf(stderr,"INTERNAL ERROR: Could not initialize windows control handler\nContinuing anyway.\nWARNING: CTRL-C will exit the application");
            }
        }
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
