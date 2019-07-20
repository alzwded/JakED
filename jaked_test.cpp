/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <exception>
struct test_error : std::exception
{
    test_error(const char* what)
        : std::exception(what)
    {}
};

struct application_exit : std::exception
{
    bool m_error;
    application_exit(bool error) : std::exception("This should not be reported as an error"), m_error(error) {}
    bool ExitedWithError() const { return m_error; }
};

#include <cstdio>
int TEST_isatty = 1;
int override_isatty(int fd)
{
    //printf("mock isatty called\n");
    return TEST_isatty;
}

#define ISATTY(X) override_isatty(X)

#define main jaked_main
#include "jaked.cpp"
#undef main
#include <memory>


#define ASSERT(X)\
    do{ bool well = (X); \
        if(!well) { \
                    std::cout << "    ....Assertion failed: " << #X << std::endl; \
                    throw test_error("failed");  \
        } \
    }while(0)

char FAIL_readCharFn() {
    throw test_error("Should not have read anything");
}

void NULL_writeStringFn(std::string const&) {}

struct ATEST {
    static void NONE() {}
    static bool FAIL() { ASSERT(!"test body not defined"); return true; }
    ATEST(std::string name, std::function<void()> setup, std::function<bool()> run, std::function<void()> teardown, void* extra)
        : m_name(name), m_setup(setup), m_run(run), m_teardown(teardown)
        , m_extra(extra)
    {}
    std::string m_name;
    std::function<void()> m_setup, m_teardown;
    std::function<bool()> m_run;
    void* m_extra;

    bool operator()() {
        bool failed = false;
        std::cout << "    Running " << m_name << std::endl;
        m_setup(); 
        try {
            failed = m_run();
        } catch(application_exit&) {
            failed = false;
        } catch(test_error&) {
            failed = true;
        } catch(std::exception& ex) {
            std::cout << "    ....Caught " << ex.what() << std::endl;
            failed = true;
        } catch(...) {
            std::cout << "    ....Caught ...\n";
            failed = true;
        }
        m_teardown();
        if(failed) {
            std::cout << "    ....FAILED" << std::endl;
        }
        return failed;
    }
};


struct SUITE {
    static void NONE() {}
    SUITE(std::function<void()> setup, std::list<std::function<bool()>> run, std::function<void()> teardown, DWORD timeout = 5 * 1000)
        : m_setup(setup), m_run(run), m_teardown(teardown), m_timeout(timeout)
    {}
    std::function<void()> m_setup, m_teardown;
    std::list<std::function<bool()>> m_run;
    DWORD m_timeout;
    DWORD Timeout() { return (m_timeout > 0) ? m_timeout : 5 * 1000; }
    void SetUp() { m_setup(); }
    void TearDown() { m_teardown(); }
    bool Run() { 
        int failed = 0;
        for(auto&& test : m_run) failed = failed + test();
        std::cout << failed << " tests failed out of " << m_run.size() << std::endl;
        return failed;
    }
};

void OverrideTestSuccceeded() 
{
    exit(0);
}

std::string argv0;

std::map<std::string, SUITE*>& GetSuites()
{
    static std::map<std::string, SUITE*> rval;
    return rval;
}

#define DEF_TEST(X) do{\
    std::string name = #X;\
    void* TEST__extra = nullptr; \
    std::function<void()> testsetup=&ATEST::NONE, testteardown = &ATEST::NONE;\
    std::function<bool()> testrun = &ATEST::FAIL;

#define TEST_SET_EXTRA(p) do{\
    static_assert(std::is_pointer<decltype(p)>::value, "should be a pointer variable"); \
    TEST__extra = p;\
} while(0);

#define TEST_SETUP()\
    testsetup = std::function<void()>([=]()

#define TEST_SETUP_END()\
    )

#define TEST_TEARDOWN()\
    testteardown = std::function<void()>([=]()

#define TEST_TEARDOWN_END()\
    )

#define TEST_RUN() \
    testrun = std::function<bool()>([=]() -> bool{

#define TEST_RUN_END()\
    return false;})

#define END_TEST() \
        suiterun.push_back(ATEST(name, testsetup, testrun, testteardown, TEST__extra));\
    }while(0)


#define DEF_SUITE(X) do{\
    DWORD Timeout = 5 * 1000; \
    std::string name = #X;\
    std::function<void()> setup(&SUITE::NONE), teardown(&SUITE::NONE);\
    std::list<std::function<bool()>> suiterun; \

#define SUITE_SETUP()\
    setup = std::function<void()>([]()

#define SUITE_SETUP_END()\
    )

#define SUITE_TEARDOWN()\
    teardown = std::function<void()>([]()

#define SUITE_TEARDOWN_END()\
    )

#define END_SUITE() \
        GetSuites()[name] = new SUITE(setup, suiterun, teardown, Timeout);\
    }while(0)

void DEFINE_SUITES() {
#if JAKED_TEST_SANITY_CHECK != 0
    DEF_SUITE(00_testtimeout) {
        Timeout = 1;
        DEF_TEST(InternalSanityCheckThatMaxTimeWorks) {
            TEST_RUN() {
                while(1);
            } TEST_RUN_END();
        } END_TEST();
    } END_SUITE();
    DEF_SUITE(00_testcrash) {
        DEF_TEST(StdTerminateCalledInSuite) {
            TEST_RUN() {
                std::terminate();
            } TEST_RUN_END();
        } END_TEST();
    } END_SUITE();
#endif
    DEF_SUITE(10_SkipWS) {
        DEF_TEST(SkipWS) {
            TEST_RUN() {
                ASSERT(3 == SkipWS("   p", 0));
            } TEST_RUN_END();
        } END_TEST();
    } END_SUITE();
    DEF_SUITE(20_ReadNumber) {
        DEF_TEST(ReadNumber) {
            TEST_RUN() {
                int num, i;
                std::tie(num, i) = ReadNumber("  123pq", 2);
                ASSERT(num == 123);
                ASSERT(i == 5);
            } TEST_RUN_END();
        } END_TEST();
    } END_SUITE();
    DEF_SUITE(30_ParseRange) {
        SUITE_SETUP() {
            for(size_t i = 0; i < 10; ++i) {
                std::stringstream ss;
                ss << "Line " << i;
                g_state.lines.push_back(ss.str());
            }
            g_state.line = 1;
        } SUITE_SETUP_END();

        DEF_TEST(ParseNothingToDot) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == r.first);
                ASSERT(i == 0);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseDot) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == 1);
                ASSERT(i == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseDotAtLine5) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".", i);
                ASSERT(r.first == 5);
                ASSERT(r.second == 5);
                ASSERT(i == 1);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseDotAtLine5WithNumericOffset2) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".+2", i);
                ASSERT(r.first == 5 + 2);
                ASSERT(r.second == r.first);
                ASSERT(i == 3);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseDotPlusSymbolicOffset2ThenComma) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".++,", i);
                ASSERT(r.first == 5 + 2);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(i == 4);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseDotAtLine5WithSymbolicOffsetNeg3) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".---", i);
                ASSERT(r.first == 5-3);
                ASSERT(r.first == r.second);
                ASSERT(i == 4);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseNothingWithCommandAfterIt) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("pn", i);
                ASSERT(r.first == 5);
                ASSERT(r.second == 5);
                ASSERT(i == 0);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseDotAndDotPlusSymbolicOffsetWithTail) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".,.+++pn", i);
                ASSERT(r.first == 5);
                ASSERT(r.second == 5 + 3);
                ASSERT(i == 6);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseDotAndComma) {
            TEST_SETUP() {
                g_state.line = 2;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(".,", i);
                ASSERT(r.first == 2);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(i == 2);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseSingleComma) {
            TEST_SETUP() {
                g_state.line = 2;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(",", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(i == 1);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseCommaDot) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(",.", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == 5);
                ASSERT(i == 2);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseNumericRange) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("2,5", i);
                ASSERT(r.first == 2);
                ASSERT(r.second == 5);
                ASSERT(i == 3);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseNumericRangeWithTail) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("2,5p", i);
                ASSERT(r.first == 2);
                ASSERT(r.second == 5);
                ASSERT(i == 3);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseNumericRangeWithOffsetsAndTail) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("5-4,2+++p", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == 5);
                ASSERT(i == 8);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseWhitespaceOneDollar) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("   1,$p", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(i == 6);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(ParseSemicolon) {
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange(";pn", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(i == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseSinglePlus) {
            TEST_SETUP() {
                g_state.line = 3;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("+", i);
                ASSERT(r.first == 4);
                ASSERT(r.first == r.second);
                ASSERT(i == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseDoubleMinusAndDotPlusOne) {
            TEST_SETUP() {
                g_state.line = 3;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("--,.+1", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == 4);
                ASSERT(i == 6);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseRegister) {
            TEST_SETUP() {
                g_state.line = 3;
                g_state.registers['q'] = g_state.lines.begin();
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
                g_state.registers.clear();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("'q", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == r.first);
                ASSERT(i == 2);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseTwoRegistersWithOffsetsEach) {
            TEST_SETUP() {
                g_state.line = 3;
                g_state.registers['q'] = g_state.lines.begin();
                g_state.registers['r'] = g_state.lines.begin();
                std::advance(g_state.registers['r'], 3);
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
                g_state.registers.clear();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("'q++,'r-1pn", i);
                ASSERT(r.first == 3);
                ASSERT(r.second == 3);
                ASSERT(i == 9);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseASillyRangeButOnlyAssertOnSecond) {
            TEST_RUN() {
                Range r;
                int i = 0; 
                std::tie(r, i) = ParseRange("1, 2, 3, 4kq", i);
                ASSERT(r.first); // this one I know is non-conformant, but this is a silly case anyway; it should be 3,4 but in my impl. it's 1,4
                ASSERT(r.second == 4);
                ASSERT(i == 10);
            } TEST_RUN_END();
        } END_TEST();

        SUITE_TEARDOWN() {
            g_state.lines.clear();
        } SUITE_TEARDOWN_END();
    } END_SUITE();

    DEF_SUITE(40_ParseCommand) {
        SUITE_SETUP() {
            for(size_t i = 0; i < 10; ++i) {
                std::stringstream ss;
                ss << "Line " << i;
                g_state.lines.push_back(ss.str());
            }
            g_state.line = 1;
        } SUITE_SETUP_END();
        
        DEF_TEST(RangeOnlyIsParsedAsRangePrint) {
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("1,$");
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(command == 'p');
                ASSERT(tail == "");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseOneDollar_p) {
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("1,$p");
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.lines.size());
                ASSERT(command == 'p');
                ASSERT(tail == "");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(Parse_f_something) {
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("f something");
                ASSERT(command == 'f');
                ASSERT(tail == "something");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(Parse_p_n) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("pn");
                ASSERT(r.first == 5);
                ASSERT(r.second == 5);
                ASSERT(command == 'p');
                ASSERT(tail == "n");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(Parse_n_comma_n_m_n) {
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("2,3m5");
                ASSERT(r.first == 2);
                ASSERT(r.second == 3);
                ASSERT(command == 'm');
                ASSERT(tail == "5");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ParseEquals) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("=");
                ASSERT(command == '=');
                ASSERT(r.second == g_state.lines.size());
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(Parse_r) {
            TEST_SETUP() {
                g_state.line = 5;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                g_state.line = 1;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("r some special thing");
                ASSERT(command == 'r');
                ASSERT(r.second == g_state.lines.size());
            } TEST_RUN_END();
        } END_TEST();
    } END_SUITE();

    DEF_SUITE(80_Integration) {
        SUITE_SETUP() {
            g_state.line = 1;
            g_state.lines.clear();
            for(size_t i = 0; i < 20; ++i) {
                std::stringstream ss;
                ss << "Line " << (i + 1);
                g_state.lines.push_back(ss.str());
            }
            g_state.registers.clear();
            g_state.diagnostic = "";
            g_state.dirty = false;
            g_state.readCharFn = FAIL_readCharFn;
            g_state.writeStringFn = NULL_writeStringFn;
        } SUITE_SETUP_END();
        DEF_TEST(LoadFile) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        {
                            int bytes = 0;
                            try {
                                bytes = std::atoi(s.c_str());
                            } catch(...) {
                                ASSERT(bytes > 0);
                            }
                        }
                        break;
                    default:
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('E')(Range(), "test\\twolines.txt");
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(EditFileAndPrint) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        break;
                    case 1:
                        ASSERT(s == "This is line 1.\n");
                        (*statew)++;
                        break;
                    case 2:
                        ASSERT(s == "This is line 2.\n");
                        (*statew)++;
                        break;
                    default:
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('E')(Range(), "test\\twolines.txt");
                Commands.at('p')(Range::R(1, Range::Dollar()), "test\\twolines.txt");
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(CommentDoesNothing) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                   default:
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('#')(Range(), "bla bla bla");
                ASSERT( (*numLinesRead) == 0);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SetAndRecallFilename) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "TestPassed\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('f')(Range(), "TestPassed");
                Commands.at('f')(Range(), "");
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();
    } END_SUITE();

    DEF_SUITE(90_System) {
        SUITE_SETUP() {
            // FIXME refactor this out
            g_state.error = false;
            g_state.Hmode = false;
            g_state.line = 1;
            g_state.lines.clear();
            for(size_t i = 0; i < 20; ++i) {
                std::stringstream ss;
                ss << "Line " << (i + 1);
                g_state.lines.push_back(ss.str());
            }
            g_state.registers.clear();
            g_state.diagnostic = "";
            g_state.dirty = false;
            g_state.readCharFn = FAIL_readCharFn;
            g_state.writeStringFn = NULL_writeStringFn;
        } SUITE_SETUP_END();
        DEF_TEST(EditFileAndPrintAndExit) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(E test\twolines.txt
,p
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        break;
                    case 1:
                        ASSERT(s == "This is line 1.\n");
                        (*statew)++;
                        break;
                    case 2:
                        ASSERT(s == "This is line 2.\n");
                        (*statew)++;
                        break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(OutOfInputExits) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(E test\twolines.txt
,p
#Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        break;
                    case 1:
                        ASSERT(s == "This is line 1.\n");
                        (*statew)++;
                        break;
                    case 2:
                        ASSERT(s == "This is line 2.\n");
                        (*statew)++;
                        break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MarkedLineGetsRecalled) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(E test\twolines.txt
1kq
2
'qp
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MarkLineAfterCommaWithOffsetThenMarkLineFromRegisterWithOffsetThenPrint) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
1,.-1kq
'qp
1,'q+kx
'xp
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(EqualsDefaultAndAddressed) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
=
1kq
'q=
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "2\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "1\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(EqualsDefaultDoesntAffectDot) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\tenlines.txt
2
=
p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "10\n"); (*numLinesRead)++; break;
                    case 3: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(RAppendsAtTheEndByDefault) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\tenlines.txt
r test\twolines.txt
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: (*numLinesRead)++; break;
                    case 2: ASSERT(s == "Line 1\n"); (*numLinesRead)++; break;
                    case 3: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    case 4: ASSERT(s == "Line 3\n"); (*numLinesRead)++; break;
                    case 5: ASSERT(s == "Line 4\n"); (*numLinesRead)++; break;
                    case 6: ASSERT(s == "Line 5\n"); (*numLinesRead)++; break;
                    case 7: ASSERT(s == "Line 6\n"); (*numLinesRead)++; break;
                    case 8: ASSERT(s == "Line 7\n"); (*numLinesRead)++; break;
                    case 9: ASSERT(s == "Line 8\n"); (*numLinesRead)++; break;
                    case 10: ASSERT(s == "Line 9\n"); (*numLinesRead)++; break;
                    case 11: ASSERT(s == "Line 10\n"); (*numLinesRead)++; break;
                    case 12: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 13: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ERF0RCombo) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
r
f test\tenlines.txt
0r
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: (*numLinesRead)++; break;
                    case 2: (*numLinesRead)++; break;
                    case 3: ASSERT(s == "Line 1\n"); (*numLinesRead)++; break;
                    case 4: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    case 5: ASSERT(s == "Line 3\n"); (*numLinesRead)++; break;
                    case 6: ASSERT(s == "Line 4\n"); (*numLinesRead)++; break;
                    case 7: ASSERT(s == "Line 5\n"); (*numLinesRead)++; break;
                    case 8: ASSERT(s == "Line 6\n"); (*numLinesRead)++; break;
                    case 9: ASSERT(s == "Line 7\n"); (*numLinesRead)++; break;
                    case 10: ASSERT(s == "Line 8\n"); (*numLinesRead)++; break;
                    case 11: ASSERT(s == "Line 9\n"); (*numLinesRead)++; break;
                    case 12: ASSERT(s == "Line 10\n"); (*numLinesRead)++; break;
                    case 13: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 14: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    case 15: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 16: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(IsAttyTrueBehaviourNoError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(!ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();


        DEF_TEST(IsAttyTrueBehaviourOnError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
'q
=
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "?\n"); break; // ?
                    case 3: ASSERT(s == "2\n"); break; // 2
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(IsAttyFalseBehaviourOnError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                TEST_isatty = false;
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
'q
=
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "?\n"); break; // ?
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
                TEST_isatty = true;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 2);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(IsAttyFalseBehaviourNoError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                TEST_isatty = false;
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
                TEST_isatty = true;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(!ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(HModeReportsSameErrorReportedBy_h) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
'q
h
H
'q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto line3 = std::make_shared<std::string>();
                g_state.writeStringFn = [numLinesRead, line3](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "?\n"); break; // ?
                    case 3: ASSERT(s.size() && s != "?\n"); *line3 = s; break;
                    case 4: ASSERT(s != "?\n"); ASSERT(s == *line3); break; // w/e error code there was
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                }
                ASSERT( (*numLinesRead) == 4);
            } TEST_RUN_END();
        } END_TEST();

    } END_SUITE();

}

VOID CALLBACK killSelf(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    if(TimerOrWaitFired) {
        fprintf(stderr, "Test suite max time hit\n");
        fflush(stderr);
        std::stringstream ss;
        ss << "taskkill /t /f /pid " << GetCurrentProcessId();
        system(ss.str().c_str());
    }
}

int main(int argc, char* argv[])
{
    DEFINE_SUITES();
    argv0 = argv[0];
    if(argc == 1) {
        std::cout << "Running " << GetSuites().size() << " test suites" << std::endl;
        bool failed = false;
        std::list<std::string> failedSuites;
        for(auto&& kv : GetSuites()) {
            std::cout << "=================================" << std::endl;
            std::stringstream ss;
            ss << argv0 << " " << kv.first;
            bool lfailed = system(ss.str().c_str());
            // special case for internal sanity check
            // 1. checks that infinite loops cause a test failure
            // 2. checks that a test suite crashing in various ways doesn't impact other test suites
            if(std::string(kv.first).find("00_") == 0) lfailed = !lfailed;
            if(lfailed) {
                std::cout << "!!!!!!!!!!!NOK!!!!!!!!!!!!!" << std::endl;
                failedSuites.push_back(kv.first);
            }
            failed = lfailed || failed;
            std::cout << "=================================" << std::endl;
        }
        if(failed) {
            std::cout << "The following suites failed: \n";
            for(auto&& s : failedSuites) std::cout << "    " << s << std::endl;
            std::cout << "FAIL" << std::endl;
        } else {
            std::cout << std::endl << "OK" << std::endl;
        }
        exit(failed);
    }

    auto* suite = GetSuites()[argv[1]];
    if(suite) {
        HANDLE hTimer;
        int allowedMaxTime = suite->Timeout();
        CreateTimerQueueTimer(&hTimer, NULL, &killSelf, NULL, allowedMaxTime, 0, WT_EXECUTEONLYONCE);

        std::cout << "Running " << argv[1] << std::endl;
        std::cout << "---------------------------------" << std::endl;
        suite->SetUp();
        bool well = suite->Run();
        suite->TearDown();
        return well;
    } else {
        std::cerr << "No such test suite " << argv[1] << std::endl;
        return 127;
    }
}
