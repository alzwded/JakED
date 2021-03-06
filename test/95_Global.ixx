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

        DEF_TEST(gSRepeat) {
            auto fn = WriteFn({
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
                    auto s = R"(g/Line/s//eniL/
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

        DEF_TEST(gAddPoint5onSomeLines) {
            auto fn = WriteFn({
                "Line 1 aaa",
                "Line 2 aab",
                "Line 2.5 aab",
                "Line 3 aba",
                "Line 4 a a",
                "Line 4.5 a a",
                "Line 5 ab a",
                "Line 6 a ba",
                "Line 6.5 a ba",
                "Line 7 a b a",
                "Line 8 b aa",
                "Line 8.5 b aa",
                "Line 9 aa b",
                "Line 10 b a a",
                "Line 10.5 b a a",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/[02468]/y\
x\
s/\([0-9]\) /\1.5 /
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

        // ======================== g with aci =======================

        DEF_TEST(gAppendWrongButTestsA) {
            auto fn = WriteFn({
                "11",
                "Line 1 aaa",
                "Line 2 aab",
                "Line 2.1",
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
                    // N.b: the following command is wrong, there should
                    // be a backslash after Line 2.1 and a single dot
                    // on the next line; this test covers an infinite
                    // loop issue that should be avoided
                    auto s = R"(g/Line 2/a\
Line 2.1
=
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
                //Commands.at('p')(Range::R(1, Range::Dollar()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();


        DEF_TEST(gAppend) {
            auto fn = WriteFn({
                "4",
                "4",
                "12",
                "Line 1 aaa",
                "Line 2 aab",
                "Line 2.1",
                "Line 2.2",
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
                    auto s = R"(g/Line 2/a\
Line 2.1\
Line 2.2\
.\
.=
.=
=
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

        DEF_TEST(gAppendChange) {
            auto fn = WriteFn({
                "3",
                "3",
                "11",
                "Line 1 aaa",
                "Line 2 aab",
                "Line 2.2",
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
                    auto s = R"(g/Line 2/a\
Line 2.1\
.\
c\
Line 2.2\
.\
.=
.=
=
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

        DEF_TEST(gAppendInsert) {
            auto fn = WriteFn({
                "3",
                "13",
                "13",
                "14",
                "Line 1 aaa",
                "Line 2 aab",
                "Line 2.1",
                "Line 2.2",
                "Line 3 aba",
                "Line 4 a a",
                "Line 5 ab a",
                "Line 6 a ba",
                "Line 7 a b a",
                "Line 8 b aa",
                "Line 9 aa b",
                "Line A",
                "Line 2.1",
                "Line 2.2",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(10c
Line A
.
g/Line [2A]/a\
Line 2.2\
.\
i\
Line 2.1\
.\
.=
.=
=
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

        // ======================== testUnacceptable ======================

        DEF_TEST(gDoesntAcceptAnotherG) {
            auto fn = WriteFn({
                "?",
                "?",
                "Line 7 a b a",
                "?",
                "?"
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(.g/Line/g//p
.g//v//p
.G/Line/
V//
.v/xxx/G//
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

