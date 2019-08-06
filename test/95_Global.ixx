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

        // ======================== g ================================

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

        DEF_TEST(GunderlineEverything) {
            auto fn = WriteFn({
                "Line 1 aaa",
                "==========",
                "Line 2 aab",
                "==========",
                "Line 3 aba",
                "==========",
                "Line 4 a a",
                "==========",
                "Line 5 ab a",
                "===========",
                "Line 6 a ba",
                "===========",
                "Line 7 a b a",
                "============",
                "Line 8 b aa",
                "===========",
                "Line 9 aa b",
                "===========",
                "Line 10 b a a",
                "=============",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/./t.\
s/./=/g
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

        DEF_TEST(Gunderline3To5) {
            auto fn = WriteFn({
                "Line 1 aaa",
                "Line 2 aab",
                "Line 3 aba",
                "==========",
                "Line 4 a a",
                "==========",
                "Line 5 ab a",
                "===========",
                "Line 6 a ba",
                "Line 7 a b a",
                "Line 8 b aa",
                "Line 9 aa b",
                "Line 10 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5g/./t.\
s/./=/g
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

        DEF_TEST(GSallIsSuccessfulHalfOfTheTimeWithinRange) {
            auto fn = WriteFn({
                "5",
                "Line 1 aaa",
                "Line 2 aab",
                "eniL 3 aba",
                "Line 4 a a",
                "eniL 5 ab a",
                "Line 6 a ba",
                "Line 7 a b a",
                "Line 8 b aa",
                "Line 9 aa b",
                "Line 10 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5g/Line/s/\(L\)\(i\)\(n\)\(e\)\( [13579] \)/\4\3\2\1\5/
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

        // ======================== v ================================

        DEF_TEST(SuccessfulVSall) {
            auto fn = WriteFn({
                //"10",
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
                    auto s = R"(v/xxx/s/\(L\)\(i\)\(n\)\(e\)/\4\3\2\1/
#.=
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

        DEF_TEST(VSallIsSuccessfulHalfOfTheTime) {
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
                    auto s = R"(v/Line [2468]/s/\(L\)\(i\)\(n\)\(e\)\( [13579] \)/\4\3\2\1\5/
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

        DEF_TEST(VSallIsSuccessfulHalfOfTheTimeWithExtraPrint) {
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
                    auto s = R"(v/Line [2468]/s/\(L\)\(i\)\(n\)\(e\)\( [13579] \)/\4\3\2\1\5/\
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

        DEF_TEST(VunderlineEverything) {
            auto fn = WriteFn({
                "Line 1 aaa",
                "==========",
                "Line 2 aab",
                "==========",
                "Line 3 aba",
                "==========",
                "Line 4 a a",
                "==========",
                "Line 5 ab a",
                "===========",
                "Line 6 a ba",
                "===========",
                "Line 7 a b a",
                "============",
                "Line 8 b aa",
                "===========",
                "Line 9 aa b",
                "===========",
                "Line 10 b a a",
                "=============",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(v/xxx/t.\
s/./=/g
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

        DEF_TEST(Vunderline3To5) {
            auto fn = WriteFn({
                "Line 1 aaa",
                "Line 2 aab",
                "Line 3 aba",
                "==========",
                "Line 4 a a",
                "==========",
                "Line 5 ab a",
                "===========",
                "Line 6 a ba",
                "Line 7 a b a",
                "Line 8 b aa",
                "Line 9 aa b",
                "Line 10 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5v/xxx/t.\
s/./=/g
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

        DEF_TEST(VSallIsSuccessfulHalfOfTheTimeWithinRange) {
            auto fn = WriteFn({
                "4",
                "Line 1 aaa",
                "Line 2 aab",
                "eniL 3 aba",
                "eniL 4 a a",
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
                    auto s = R"(2,4v/aa/s/\(L\)\(i\)\(n\)\(e\)\( [0-9] \)/\4\3\2\1\5/
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

        // ======================== G ================================

        DEF_TEST(SimpleGInteractive) {
            auto fn = WriteFn({
                "Line 2 aab",
                "Line 3 aba",
                "Line 4 a a",
                "4",
                "Line 1 aaa",
                "Line 2 bbb",
                "Line 2&3 aba",
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
                    auto s = R"(2,4G/Line/
s/a/b/g
s/3/2\&3/
d
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

        DEF_TEST(GInteractiveWithBlankAndLongerCommandList) {
            auto fn = WriteFn({
                "Line 2 aab",
                "Line 3 aba",
                "Line 4 a a",
                "6",
                "Line 1 aaa",
                "Line 2 bbb",
                "Line 3 aba",
                "Line 4 a a",
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
                    auto s = R"(2,4G/Line/
s/a/b/g

t.\
t.\
d
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



        // ======================== V ================================

        DEF_TEST(VInteractiveWithnAndLongerCommandList) {
            auto fn = WriteFn({
                "Line 2 aab",
                "2\tLine 2 aab",
                "3\tLine 3 aba",
                "Line 4 a a",
                "6",
                "Line 1 aaa",
                "Line 2 aab",
                "Line 3 aba",
                "Line 4 a a",
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
                    auto s = R"(2,4V/[13579]/
.,+n
t.\
t.\
d
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
