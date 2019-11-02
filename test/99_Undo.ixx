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

        DEF_TEST(undoGlobalSubstitution) {
            auto fn = WriteFn({
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
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/./s/./a/g
u
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
                g_state.line = 1;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(undoMoveBefore) {
            auto fn = WriteFn({
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
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7m0
H
u
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
                g_state.line = 1;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(undoMoveBefore2) {
            auto fn = WriteFn({
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
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7m1
H
u
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
                g_state.line = 1;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(undoMoveAfter) {
            auto fn = WriteFn({
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
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7m$
u
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
                g_state.line = 1;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(undoDeleteSome) {
            auto fn = WriteFn({
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
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7d
u
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
                g_state.line = 1;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(undoDeleteAll) {
            auto fn = WriteFn({
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
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(,d
u
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
                g_state.line = 1;
                Loop();
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();
