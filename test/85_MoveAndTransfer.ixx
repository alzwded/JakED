        auto checkRegisters = []() {
            // registers are preserved
            ASSERT(g_state.registers.find('q') != g_state.registers.end());
            ASSERT(!!g_state.registers.at('q'));
            ASSERT(static_cast<std::string>(g_state.registers.at('q')->text()) == "Line 3");
            ASSERT(g_state.registers.find('w') != g_state.registers.end());
            ASSERT(!!g_state.registers.at('w'));
            ASSERT(static_cast<std::string>(g_state.registers.at('w')->text()) == "Line 4");
            ASSERT(g_state.registers.at('q')->next() == g_state.registers.at('w'));
            ASSERT(g_state.registers.find('e') != g_state.registers.end());
            ASSERT(!!g_state.registers.at('e'));
            ASSERT(static_cast<std::string>(g_state.registers.at('e')->text()) == "Line 5");
            ASSERT(g_state.registers.at('w')->next() == g_state.registers.at('e'));
        };

        SUITE_SETUP() {
            // FIXME refactor this out
            g_state();
            auto after = g_state.swapfile.head();
            for(size_t i = 0; i < 10; ++i) {
                std::stringstream ss;
                ss << "Line " << i + 1;
                auto line = g_state.swapfile.line(ss.str());
                after->link(line);
                after = line;
            }
            g_state.nlines = 10;
            g_state.line = 1;
            g_state.readCharFn = FAIL_readCharFn;
            g_state.writeStringFn = NULL_writeStringFn;
            g_state.registers['q'] = g_state.swapfile.head()
                    ->next()  // 1
                    ->next()  // 2
                    ->next(); // 3
            g_state.registers['w'] = g_state.registers['q']->next(); // 4
            g_state.registers['e'] = g_state.registers['w']->next(); // 5
        } SUITE_SETUP_END();

        DEF_TEST(MoveAfterSelf) {
            WriteFn fn({
                "5",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5m5
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MoveAfter2) {
            WriteFn fn({
                "5",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5m2
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MoveAfter0) {
            WriteFn fn({
                "3",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 1",
                "Line 2",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5m0
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MoveAfter$) {
            WriteFn fn({
                "10",
                "Line 1",
                "Line 2",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
                "Line 3",
                "Line 4",
                "Line 5",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5m$
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MoveAfter1) {
            WriteFn fn({
                "4",
                "Line 1",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 2",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5m1
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MoveAfter9) {
            WriteFn fn({
                "9",
                "Line 1",
                "Line 2",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5m9
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TransferAfterSelf) {
            WriteFn fn({
                "8",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5t5
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 13);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TransferAfter2) {
            WriteFn fn({
                "5",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5t2
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 13);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TransferAfter0) {
            WriteFn fn({
                "3",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5t0
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 13);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TransferAfter$) {
            WriteFn fn({
                "13",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
                "Line 3",
                "Line 4",
                "Line 5",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5t$
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 13);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TransferAfter1) {
            WriteFn fn({
                "4",
                "Line 1",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5t1
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 13);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(TransferAfter9) {
            WriteFn fn({
                "12",
                "Line 1",
                "Line 2",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 6",
                "Line 7",
                "Line 8",
                "Line 9",
                "Line 3",
                "Line 4",
                "Line 5",
                "Line 10",
            });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(3,5t9
.=
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit&) {
                }
                fn.Assert();
                checkRegisters();
                ASSERT(g_state.nlines == 13);
            } TEST_RUN_END();
        } END_TEST();
