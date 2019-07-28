
        SUITE_SETUP() {
            g_state();
            auto after = g_state.swapfile.head();
            for(size_t i = 0; i < 10; ++i) {
                std::stringstream ss;
                ss << "Line " << i;
                auto line = g_state.swapfile.line(ss.str());
                after->link(line);
                after = line;
            }
            g_state.nlines = 10;
            g_state.line = 1;
        } SUITE_SETUP_END();
        
        DEF_TEST(RangeOnlyIsParsedAsRangePrint) {
            TEST_RUN() {
                Range r;
                char command;
                std::string tail;
                std::tie(r, command, tail) = ParseCommand("1,$");
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.nlines);
                ASSERT(r.second == CountLines());
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
                ASSERT(r.second == g_state.nlines);
                ASSERT(r.second == CountLines());
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
                ASSERT(tail == " something");
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

        DEF_TEST(ParseComma_n) {
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
                std::tie(r, command, tail) = ParseCommand(",n");
                ASSERT(r.first == 1);
                ASSERT(r.second == g_state.nlines);
                ASSERT(r.second == CountLines());
                ASSERT(command == 'n');
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
                ASSERT(r.second == g_state.nlines);
                ASSERT(r.second == CountLines());
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
                ASSERT(r.second == g_state.nlines);
                ASSERT(r.second == CountLines());
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(Parse_z) {
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
                std::tie(r, command, tail) = ParseCommand("z3");
                ASSERT(command == 'z');
                ASSERT(r.second == Range::Dot(1));
                ASSERT(tail == "3");
            } TEST_RUN_END();
        } END_TEST();
