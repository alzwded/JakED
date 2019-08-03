        SUITE_SETUP() {
            g_state();
            auto after = g_state.swapfile.head();
            std::string strs[] = {
                "Line 1 aaa",
                "Line 2 aab",
                "Line 3 aba",
                "Line 4 a a",
                "Line 5 ab a",
                "Line 6 a ba",
                "Line 7 a b a",
                "Line 8 b aa",
                "Line 9 aa b",
                "Line 10 b a a",
            };
            for(size_t i = 0; i < 10; ++i) {
                auto line = g_state.swapfile.line(strs[i]);
                after->link(line);
                after = line;
            }
            g_state.nlines = 10;
            g_state.line = 1;
            g_state.readCharFn = FAIL_readCharFn;
            g_state.writeStringFn = NULL_writeStringFn;
        } SUITE_SETUP_END();

        DEF_TEST(SuccessfulGSall) {
            auto fn = WriteFn({
                "10",
                "eniL 1 aaa",
                "eniL 2 aab",
                "eniL 3 aba",
                "eniL 4 a a",
                "eniL 5 ab a",
                "eniL 6 a ba",
                "eniL 7 a b a",
                "eniL 8 b aa",
                "eniL 9 aa b",
                "eniL 10 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/Line/s/\(L\)\(i\)\(n\)\(e\)/\4\3\2\1/
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                g_state.line = 5;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(GSallIsSuccessfulHalfOfTheTime) {
            auto fn = WriteFn({
                "9",
                "eniL 1 aaa",
                "Line 2 aab",
                "eniL 3 aba",
                "Line 4 a a",
                "eniL 5 ab a",
                "Line 6 a ba",
                "eniL 7 a b a",
                "Line 8 b aa",
                "eniL 9 aa b",
                "Line 10 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/Line/s/\(L\)\(i\)\(n\)\(e\)\( [13579] \)/\4\3\2\1\5/
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                g_state.line = 5;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(GSallIsSuccessfulHalfOfTheTimeWithExtraPrint) {
            auto fn = WriteFn({
                "eniL 1 aaa",
                "eniL 3 aba",
                "eniL 5 ab a",
                "eniL 7 a b a",
                "eniL 9 aa b",
                "9",
                "eniL 1 aaa",
                "Line 2 aab",
                "eniL 3 aba",
                "Line 4 a a",
                "eniL 5 ab a",
                "Line 6 a ba",
                "eniL 7 a b a",
                "Line 8 b aa",
                "eniL 9 aa b",
                "Line 10 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/Line/s/\(L\)\(i\)\(n\)\(e\)\( [13579] \)/\4\3\2\1\5/\
.p
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                g_state.line = 5;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();
