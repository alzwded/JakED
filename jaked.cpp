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

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

volatile DWORD ctrlc = 0;
struct {
    std::string FILE;
    int line;
    std::string PROMPT = "* ";
    std::list<std::string> lines;
    std::map<char, decltype(lines)::iterator> registers;
    std::string diagnostic;
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

namespace CommandsImpl {
    void r(Range r, std::string tail)
    {
        throw std::runtime_error("not implemented");
    }

    void q(Range r, std::string tail)
    {
        throw std::runtime_error("not implemented");
    }

    void Q(Range r, std::string tail)
    {
        exit(0);
    }

    void p(Range r, std::string)
    {
        int first = std::min(1, r.first);
        int second = std::min(1, r.second);
        auto a = g_state.lines.begin();
        auto b = g_state.lines.begin();
        std::advance(a, r.first - 1);
        std::advance(b, r.second - 1);
        for(auto it = a; it != b; ++it) {
            printf("%s\n", it->c_str());
        }
    }

}

int SkipWS(std::string const& s, int i)
{
    while(s[i] == ' ' || s[i] == '\t') ++i;
    return i;
}

std::map<char, std::function<void(Range, std::string)>> Commands = {
    { 'p', &CommandsImpl::p },
    { 'r', &CommandsImpl::r },
    { 'q', &CommandsImpl::q },
    { 'Q', &CommandsImpl::Q },
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
        case '0': case '1': case '2': case '3': case '4':
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
        case '+': case '-': case ',': case ';': case '$':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            std::tie(r, i) = ParseRange(s, i);
            break;
        case '\'':
            std::tie(r, i) = ParseRange(s, i);
            break;
    }
    switch(s[i]) {
        case '\0':
            return std::make_tuple(r, 'p', std::string());
        case 'p': case 'g': case 'G': case 'v': case 'V':
        case 'u': case 'P': case 'H': case 'h': case '#':
        case 'k': case 'i': case 'a': case 'c': case 'd':
        case 'j': case 'm': case 't': case 'y':
        case 'x': case 'r': case 'l': case 'z': case '=':
        case 'W': case 'e': case 'E': case 'f': case 'w':
        case 'q': case 'Q':
        case '\n':
            return std::make_tuple(r, s[i], s.substr(SkipWS(s, i+1)));
        default:
            throw std::runtime_error("Syntax error");
    }
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
                while(std::cin.good()) {
                    char c;
                    std::cin >> c;
                    if(c == '\n') break;
                    ss << c;
                    if(ctrlc) return;
                }
                auto s = ss.str();

                std::tie(r, command, tail) = ParseCommand(s);
                if(ctrlc) break;
                Commands.at(command)(r, tail);
                g_state.diagnostic = "";
            } while(0);
        } catch(std::exception& ex) {
            printf("?\n");
            g_state.diagnostic = ex.what();
        } catch(...) {
            printf("?\n");
            g_state.diagnostic = "???";
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
        if(SetConsoleMode(console, mode|ENABLE_PROCESSED_INPUT)) {
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

    std::string file = argv[1];
    if(file.empty()) exit_usage("No such file!", argv[0]);
    Commands.at('r')(Range::ZERO(), file);

    Loop();
    return 0;
}
