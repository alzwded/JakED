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
