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

        DEF_TEST(undoMoveAfter2) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7m9
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

        DEF_TEST(undoInsert) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3i
This line should be deleted.
As should this one.
.
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

        DEF_TEST(undoTransferBefore) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7t0
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

        DEF_TEST(undoTransferBefore2) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7t1
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

        DEF_TEST(undoTransferAfter) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7t$
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

        DEF_TEST(undoTransferAfter2) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7t9
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

        DEF_TEST(undoTransferSelf) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3t3
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

        DEF_TEST(undoJoin) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7j
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

        DEF_TEST(undoSuperJoin) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(,j*
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

        DEF_TEST(undoAppnendMiddle) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3a
Delete me
.
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

        DEF_TEST(undoAppnendEnd) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"($a
Delete me
.
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

        DEF_TEST(undoAppnend1) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(1a
Delete me
.
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

        DEF_TEST(undoAppnend0) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(0a
Delete me
.
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

        DEF_TEST(undoCrosspileEnd) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7y
$x
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

        DEF_TEST(undoCrosspile0) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7y
0x
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

        DEF_TEST(undoCrosspileMiddle) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7y
5x
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

        DEF_TEST(undoChange) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,7c
fix me
.
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

        DEF_TEST(undoChangeAll) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(,c
fix me
fix me
.
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

        DEF_TEST(undo_g_c) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(g/[2468]$/c\
fix me\
fix me\
.
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

        DEF_TEST(undo_s) {
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3s/Line/fix/
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

        DEF_TEST(undo_sall) {
            auto fn = WriteFn({
                "?",
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
                    auto s = R"(%s/[1-8]/fix/
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

        DEF_TEST(multiple_undo) {
            auto fn = WriteFn({
                // first undo, reinsert line 1
                "Line 1 aaa",
                "Line 4 a a",
                "Line 5 ab a",
                // second undo, reinsert line 3
                "Line 1 aaa",
                "Line 3 aba",
                "Line 4 a a",
                // third undo, reinsert line 2
                "Line 1 aaa",
                "Line 2 aab",
                "Line 3 aba",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(2d
2d
1d
u
1,3p
u
1,3p
u
1,3p
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

        DEF_TEST(multiple_global_undo_and_branch) {
            auto fn = WriteFn({
                // first undo, reinsert line 1
                "Line 1 aaa",
                "Line 4 a a",
                "Line 5 ab a",
                // second undo, reinsert line 3
                "Line 1 aaa",
                "Line 3 aba",
                "Line 4 a a",
                // third undo, undo delete "test"
                "L 1 aaa",
                "test",
                "L 3 aba",
                // fourth undo, undo subst and append
                "Line 1 aaa",
                "Line 3 aba",
                "Line 4 a a",
                // fifth undo, reinsert line 2
                "Line 1 aaa",
                "Line 2 aab",
                "Line 3 aba",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(2d
2d
1d
u
1,3p
u
1,3p
g/Line/s//L/\
a\
test
g/test/d
u
1,3p
u
1,3p
u
1,3p
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
