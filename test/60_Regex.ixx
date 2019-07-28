
        SUITE_SETUP() {
            g_state();
            auto after = g_state.swapfile.head();
            std::string strs[] = {
                /*  1 */ "aaa",
                /*  2 */ "aab",
                /*  3 */ "aba",
                /*  4 */ "a a",
                /*  5 */ "ab a",
                /*  6 */ "a ba",
                /*  7 */ "a b a",
                /*  8 */ "b aa",
                /*  9 */ "aa b",
                /* 10 */ "b a a",
            };
            for(size_t i = 0; i < 10; ++i) {
                std::stringstream ss;
                ss << "Line " << (i + 1) << strs[i];
                auto line = g_state.swapfile.line(ss.str());
                after->link(line);
                after = line;
            }
            g_state.nlines = 10;
            g_state.line = 1;
            g_state.readCharFn = FAIL_readCharFn;
            g_state.writeStringFn = NULL_writeStringFn;
        } SUITE_SETUP_END();

        DEF_TEST(searchForwardNoWrap) {
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/aa/d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 4);
                ASSERT("/aa/d"[i] == 'd');
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(repeatForward) {
            TEST_SETUP() {
                g_state.regexp = std::regex("aa", std::regex_constants::basic);
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("//d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 2);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchForwardWrap1) {
            TEST_SETUP() {
                g_state.line = 10;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/ab/d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 4);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchForwardWrap2) {
            TEST_SETUP() {
                g_state.line = 9;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/ab/d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 4);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchForwardWordBoundary) {
            TEST_SETUP() {
                g_state.line = 1;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/\\<aa\\>/d", i);
                ASSERT(r.second == 8);
                ASSERT(i == 8);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchForwardWrapToSelf) {
            TEST_SETUP() {
                g_state.line = 8;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/\\<aa\\>/d", i);
                ASSERT(r.second == 8);
                ASSERT(i == 8);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchBackWrapToSelf) {
            TEST_SETUP() {
                g_state.line = 8;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("?\\<aa\\>?d", i);
                ASSERT(r.second == 8);
                ASSERT(i == 8);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchBack1) {
            TEST_SETUP() {
                g_state.line = 8;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("?aa?d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 4);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchBackRepeat) {
            TEST_SETUP() {
                g_state.line = 8;
                g_state.regexp = std::regex("aa", std::regex_constants::basic);
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("??d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 2);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchBack2) {
            TEST_SETUP() {
                g_state.line = 8;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("?b a a?d", i);
                ASSERT(r.second == 10);
                ASSERT(i == 7);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchForwardNumericRepeat) {
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/a\\{1,2\\}/d", i);
                ASSERT(r.second == 2);
                ASSERT(i == 10);
                ASSERT("/a\\{1,2\\}/d"[i] == 'd');
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(searchForwardRawCurlyBrackets) {
            TEST_SETUP() {
                auto i = g_state.swapfile.head();
                while(i->next()) i = i->next();
                auto l = g_state.swapfile.line("1 Line with {} not at the end");
                i->link(l);
                i = l;
                l = g_state.swapfile.line("1 Line with {}");
                i->link(l);
                g_state.nlines = 12;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/ {}$/d", i);
                ASSERT(r.second == 12);
                ASSERT(i == 6);
                ASSERT("/ {}$/d"[i] == 'd');
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(matchBOL) {
            TEST_SETUP() {
                auto i = g_state.swapfile.head();
                while(i->next()) i = i->next();
                auto l = g_state.swapfile.line("1 Line with {} not at the end");
                i->link(l);
                i = l;
                l = g_state.swapfile.line("1 Line with {}");
                i->link(l);
                g_state.nlines = 12;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Range r;
                int i = 0;
                std::tie(r, i) = ParseRegex("/^1.*$/d", i);
                ASSERT(r.second == 11);
                ASSERT(i == 7);
                ASSERT("/^1.*$/d"[i] == 'd');
            } TEST_RUN_END();
        } END_TEST();
