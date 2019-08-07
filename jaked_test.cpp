/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <exception>
#include <optional>
#include <vector>
#include <string>
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

inline size_t CountLines()
{
    size_t n = 0;
    for(auto i = g_state.swapfile.head();
        i->next();
        i = i->next())
    {
        ++n;
    }
    return n;
}
struct WriteFn {
    std::shared_ptr<size_t> state;
    std::vector<std::optional<std::string>> lines;
    WriteFn(decltype(lines) const& lines_ = {})
        : state(new size_t{0})
        , lines(lines_)
    {}
    void operator()(std::string s) {
        if(*state >= lines.size()) {
            fprintf(stderr, "    ....Unexpected string: %s", s.c_str());
            ASSERT(!"Read too much!");
            return;
        }
        if(lines[*state]) {
            if(s != (*lines[*state]+"\n")) {
                fprintf(stderr, "    ....Expected: %s\n", lines[*state]->c_str());
                fprintf(stderr, "    ....Got: %s", s.c_str());
            }
            ASSERT(s == (*lines[*state]+"\n"));
        }
        ++(*state);
    }
    void Assert() const {
        if(*state != lines.size()) {
            fprintf(stderr, "    ....Expected to have read %zd\n", lines.size());
            fprintf(stderr, "    ....Actually read %zd\n", *state);
        }
        ASSERT(*state == lines.size());
    }
};



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
        } catch(test_error& ex) {
            std::cout << "    ....Caught " << ex.what() << std::endl;
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

    DEF_SUITE(30_Regex) {
#       include "test/30_Regex.ixx"
    } END_SUITE();

    DEF_SUITE(50_ParseRange) {
#       include "test/50_ParseRange.ixx"
    } END_SUITE();

    DEF_SUITE(60_ParseCommand) {
#       include "test/60_ParseCommand.ixx"
    } END_SUITE();

    DEF_SUITE(80_Integration) {
#       include "test/80_Integration.ixx"
    } END_SUITE();

    DEF_SUITE(81_Substitute) {
#       include "test/81_Substitute.ixx"
    } END_SUITE();

    DEF_SUITE(85_MoveAndTransfer) {
#       include "test/85_MoveAndTransfer.ixx"
    } END_SUITE();

    DEF_SUITE(90_System) {
#       include "test/90_System.ixx"
    } END_SUITE();

    DEF_SUITE(95_Global) {
#       include "test/95_Global.ixx"
    } END_SUITE();
}

VOID CALLBACK killSelf(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    if(IsDebuggerPresent()) return;
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

    g_state.swapfile.type(1);
    auto* suite = GetSuites()[argv[1]];
    if(suite) {
        HANDLE hTimer;
        int allowedMaxTime = suite->Timeout();
        if(!IsDebuggerPresent()) {
            CreateTimerQueueTimer(
                    &hTimer,
                    NULL, 
                    &killSelf, 
                    NULL, 
                    allowedMaxTime, 
                    0, 
                    WT_EXECUTEONLYONCE);
        }

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
