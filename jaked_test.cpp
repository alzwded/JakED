#define main jaked_main
#include "jaked.cpp"
#undef main

struct test_error : std::exception
{
    test_error(const char* what)
        : std::exception(what)
    {}
};

#define ASSERT(X)\
    do{ bool well = (X); \
        if(!well) { \
                    std::cout << "    ....Assertion failed: " << #X << std::endl; \
                    throw test_error("failed");  \
        } \
    }while(0)


struct ATEST {
    static void NONE() {}
    static bool FAIL() { return false; }
    ATEST(std::string name, std::function<void()> setup, std::function<bool()> run, std::function<void()> teardown)
        : m_name(name), m_setup(setup), m_run(run), m_teardown(teardown)
    {}
    std::string m_name;
    std::function<void()> m_setup, m_teardown;
    std::function<bool()> m_run;

    bool operator()() {
        bool failed = false;
        std::cout << "    Running " << m_name << std::endl;
        m_setup(); 
        try {
            failed = m_run();
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
    SUITE(std::function<void()> setup, std::list<std::function<bool()>> run, std::function<void()> teardown)
        : m_setup(setup), m_run(run), m_teardown(teardown)
    {}
    std::function<void()> m_setup, m_teardown;
    std::list<std::function<bool()>> m_run;
    void SetUp() { m_setup(); }
    void TearDown() { m_teardown(); }
    bool Run() { 
        int failed = 0;
        for(auto&& test : m_run) failed = failed + test();
        std::cout << failed << " tests failed out of " << m_run.size() << std::endl;
        return failed;
    }
};

std::string argv0;

std::map<std::string, SUITE*>& GetSuites()
{
    static std::map<std::string, SUITE*> rval;
    return rval;
}

#define DEF_TEST(X) do{\
    std::string name = #X;\
    std::function<void()> setup=&ATEST::NONE, teardown = &ATEST::NONE;\
    std::function<bool()> run = &ATEST::FAIL;

#define TEST_SETUP()\
    setup = std::function<void()>([]()

#define TEST_SETUP_END()\
    )

#define TEST_TEARDOWN()\
    teardown = std::function<void()>([]()

#define TEST_TEARDOWN_END()\
    )

#define TEST_RUN() \
    run = std::function<bool()>([]() -> bool{

#define TEST_RUN_END()\
    return false;})

#define END_TEST() \
        suiterun.push_back(ATEST(name, setup, run, teardown));\
    }while(0)


#define DEF_SUITE(X) do{\
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
        GetSuites()[name] = new SUITE(setup, suiterun, teardown);\
    }while(0)

void DEFINE_SUITES() {
    DEF_SUITE(SkipWS) {
        DEF_TEST(SkipWS) {
            ASSERT(3 == SkipWS("   p", 0));
        } END_TEST();
    } END_SUITE();
    DEF_SUITE(ParseRange) {
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
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRange("pn", i);
                ASSERT(r.first == 1);
                ASSERT(r.second == 1);
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


        SUITE_TEARDOWN() {
            g_state.lines.clear();
        } SUITE_TEARDOWN_END();
    } END_SUITE();
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
