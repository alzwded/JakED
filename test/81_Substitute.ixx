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

        DEF_TEST(SimpleSuccessfulSubstitution) {
            TEST_SETUP() {
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), "/aa/cc/");
                ASSERT(g_state.line == 2);
                ASSERT(!!g_state.swapfile.head());
                ASSERT(!!g_state.swapfile.head()->next());
                ASSERT(!!g_state.swapfile.head()->next()->next());
                ASSERT(g_state.swapfile.head()->next()->next()->text() == "Line 2 ccb");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SimpleSuccessfulSG) {
            TEST_SETUP() {
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), "/a/c/g");
                ASSERT(g_state.line == 2);
                ASSERT(!!g_state.swapfile.head());
                ASSERT(!!g_state.swapfile.head()->next());
                ASSERT(!!g_state.swapfile.head()->next()->next());
                ASSERT(g_state.swapfile.head()->next()->next()->text() == "Line 2 ccb");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SimpleSuccessfulSubstitutionLine1) {
            TEST_SETUP() {
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(1), "/aa/cc/");
                ASSERT(g_state.line == 1);
                ASSERT(!!g_state.swapfile.head());
                ASSERT(!!g_state.swapfile.head()->next());
                ASSERT(g_state.swapfile.head()->next()->text() == "Line 1 cca");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SimpleSuccessfulSGLine1) {
            TEST_SETUP() {
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(1), "/a/c/g");
                ASSERT(g_state.line == 1);
                ASSERT(!!g_state.swapfile.head());
                ASSERT(!!g_state.swapfile.head()->next());
                ASSERT(g_state.swapfile.head()->next()->text() == "Line 1 ccc");
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SimpleSuccessfulSubstitutionLine10) {
            auto fn = WriteFn({
                "Line 10 b c a"
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(10), "/a/c/");
                ASSERT(g_state.line == 10);
                Commands.at('p')(Range::S(Range::Dot()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SimpleSuccessfulSGLine10) {
            auto fn = WriteFn({
                "Line 10 b c c"
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(10), "/a/c/g");
                ASSERT(g_state.line == 10);
                Commands.at('p')(Range::S(Range::Dot()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MultiLineSubst) {
            auto fn = WriteFn({
                "Line line 1 aaa",
                "Line line 2 aab",
                "Line line 3 aba",
                "Line line 4 a a",
                "Line line 5 ab a",
                "Line line 6 a ba",
                "Line line 7 a b a",
                "Line line 8 b aa",
                "Line line 9 aa b",
                "Line line 10 b a a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::R(1, 10), "/ine/& l&/");
                ASSERT(g_state.line == 10);
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MultiLineFailFailsElegantly) {
            auto fn = WriteFn({
                "Line 7 d b a",
                "Line 1 daa",
                "Line 2 dab",
                "Line 3 dba",
                "Line 4 d a",
                "Line 5 db a",
                "Line 6 d ba",
                "Line 7 d b a",
                "Line 8 b aa",
                "Line 9 aa b",
                "Line 10 b a a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Commands.at('s')(Range::R(1, 10), R"(/\([0-9]\) a/\1 d/")");
                } catch(JakEDException& ex) {
                }
                Commands.at('p')(Range::S(Range::Dot()), "");
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(DotIsNotSetIfNoLinesAffected) {
            auto fn = WriteFn({
                "Line 5 ab a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                g_state.line = 5;
                try {
                    Commands.at('s')(Range::R(1, 10), R"(/XXJXJJXJXX/a/")");
                } catch(JakEDException& ex) {
                    //fprintf(stderr, "    ....Caught expected exception: %s\n", ex.what());
                }
                Commands.at('p')(Range::S(Range::Dot()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(DotIsSetCorrectlyIfNoLinesAffectedThisTimeCheckingTheMainLoop) {
            auto fn = WriteFn({
                " ab a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                g_state.line = 5;
                try {
                    Commands.at('s')(Range::R(1, 10), R"(/Line [12345]//")");
                } catch(JakEDException& ex) {
                    //fprintf(stderr, "    ....Caught expected exception: %s\n", ex.what());
                }
                Commands.at('p')(Range::S(Range::Dot()), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(RegexMatchesFurtherLineThanCurrentIsAFailure) {
            auto fn = WriteFn({
                " ab a",
                "Line 6 a ba",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                g_state.line = 5;
                std::exception_ptr errorActuallyOccurred;
                try {
                    Commands.at('s')(Range::R(1, 10), R"(/Line [12345789]//")"); // 6 is missing
                } catch(JakEDException& ex) {
                    errorActuallyOccurred = std::current_exception();
                }
                ASSERT(!!errorActuallyOccurred);
                Commands.at('p')(Range::R(Range::Dot(), Range::Dot(1)), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteNth) {
            auto fn = WriteFn({
                "Li*e 2 aab",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), R"(/./*/3)");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteNthAndPrint) {
            auto fn = WriteFn({
                "n",
                "Li*e 2 aab",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), R"(/./*/3p)");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteAllAndPrint) {
            auto fn = WriteFn({
                "Line 2 aab",
                "**********",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), R"(/./*/gp)");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteFirstAndPrint) {
            auto fn = WriteFn({
                "L",
                "*ine 2 aab",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), R"(/./*/p)");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteItsComplicatedAndPrint) {
            auto fn = WriteFn({
                "ine 2 a",
                "L 2 aab",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), R"(/..e\( [0-9 ]*\(a\)\)/\1/p)");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteItsComplicatedGloballyAndPrint) {
            auto fn = WriteFn({
                "ne 5ab a",
                "Li 5  a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(5), R"(/..\( \+\(.\)\)/\1/gp)");
                ASSERT(g_state.line == 5);
                Commands.at('p')(Range::S(5), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteItsComplicatedSecondAndPrint) {
            auto fn = WriteFn({
                "ab a",
                "Line 5  a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(5), R"(/..\( \+\(.\)\)/\1/2p)");
                ASSERT(g_state.line == 5);
                Commands.at('p')(Range::S(5), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SubstituteItsComplicatedFirstAndPrint) {
            auto fn = WriteFn({
                "ne 5",
                "Li 5 ab a",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(5), R"(/..\( \+\(.\)\)/\1/1p)");
                ASSERT(g_state.line == 5);
                Commands.at('p')(Range::S(5), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TestRepeat) {
            auto fn = WriteFn({
                "L*n* 2 **b",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
                g_state.regexp = std::regex(R"([aeiou])", std::regex_constants::ECMAScript);
                g_state.fmt = R"(*/g)";
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(2), "");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TestSSetsRepeat) {
            auto fn = WriteFn({
                "L*n* 2 **b",
            });
            TEST_SETUP() {
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('s')(Range::S(1), R"(/[aeiou]/*/g)");
                Commands.at('s')(Range::S(2), "");
                ASSERT(g_state.line == 2);
                Commands.at('p')(Range::S(2), "");
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();
