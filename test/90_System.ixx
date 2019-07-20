
        SUITE_SETUP() {
            // FIXME refactor this out
            g_state.error = false;
            g_state.Hmode = false;
            g_state.line = 1;
            g_state.lines.clear();
            for(size_t i = 0; i < 20; ++i) {
                std::stringstream ss;
                ss << "Line " << (i + 1);
                g_state.lines.push_back(ss.str());
            }
            g_state.registers.clear();
            g_state.diagnostic = "";
            g_state.dirty = false;
            g_state.readCharFn = FAIL_readCharFn;
            g_state.writeStringFn = NULL_writeStringFn;
        } SUITE_SETUP_END();
        DEF_TEST(EditFileAndPrintAndExit) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(E test\twolines.txt
,p
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        break;
                    case 1:
                        ASSERT(s == "This is line 1.\n");
                        (*statew)++;
                        break;
                    case 2:
                        ASSERT(s == "This is line 2.\n");
                        (*statew)++;
                        break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();
        DEF_TEST(OutOfInputExits) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(E test\twolines.txt
,p
#Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        break;
                    case 1:
                        ASSERT(s == "This is line 1.\n");
                        (*statew)++;
                        break;
                    case 2:
                        ASSERT(s == "This is line 2.\n");
                        (*statew)++;
                        break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                delete numLinesRead;
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MarkedLineGetsRecalled) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    auto s = R"(E test\twolines.txt
1kq
2
'qp
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(MarkLineAfterCommaWithOffsetThenMarkLineFromRegisterWithOffsetThenPrint) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
1,.-1kq
'qp
1,'q+kx
'xp
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(EqualsDefaultAndAddressed) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
=
1kq
'q=
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "2\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "1\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(EqualsDefaultDoesntAffectDot) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\tenlines.txt
2
=
p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    case 2: ASSERT(s == "10\n"); (*numLinesRead)++; break;
                    case 3: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(RAppendsAtTheEndByDefault) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\tenlines.txt
r test\twolines.txt
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: (*numLinesRead)++; break;
                    case 2: ASSERT(s == "Line 1\n"); (*numLinesRead)++; break;
                    case 3: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    case 4: ASSERT(s == "Line 3\n"); (*numLinesRead)++; break;
                    case 5: ASSERT(s == "Line 4\n"); (*numLinesRead)++; break;
                    case 6: ASSERT(s == "Line 5\n"); (*numLinesRead)++; break;
                    case 7: ASSERT(s == "Line 6\n"); (*numLinesRead)++; break;
                    case 8: ASSERT(s == "Line 7\n"); (*numLinesRead)++; break;
                    case 9: ASSERT(s == "Line 8\n"); (*numLinesRead)++; break;
                    case 10: ASSERT(s == "Line 9\n"); (*numLinesRead)++; break;
                    case 11: ASSERT(s == "Line 10\n"); (*numLinesRead)++; break;
                    case 12: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 13: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(ERF0RCombo) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
r
f test\tenlines.txt
0r
,p
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    switch(*numLinesRead) {
                    case 0: (*numLinesRead)++; break;
                    case 1: (*numLinesRead)++; break;
                    case 2: (*numLinesRead)++; break;
                    case 3: ASSERT(s == "Line 1\n"); (*numLinesRead)++; break;
                    case 4: ASSERT(s == "Line 2\n"); (*numLinesRead)++; break;
                    case 5: ASSERT(s == "Line 3\n"); (*numLinesRead)++; break;
                    case 6: ASSERT(s == "Line 4\n"); (*numLinesRead)++; break;
                    case 7: ASSERT(s == "Line 5\n"); (*numLinesRead)++; break;
                    case 8: ASSERT(s == "Line 6\n"); (*numLinesRead)++; break;
                    case 9: ASSERT(s == "Line 7\n"); (*numLinesRead)++; break;
                    case 10: ASSERT(s == "Line 8\n"); (*numLinesRead)++; break;
                    case 11: ASSERT(s == "Line 9\n"); (*numLinesRead)++; break;
                    case 12: ASSERT(s == "Line 10\n"); (*numLinesRead)++; break;
                    case 13: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 14: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    case 15: ASSERT(s == "This is line 1.\n"); (*numLinesRead)++; break;
                    case 16: ASSERT(s == "This is line 2.\n"); (*numLinesRead)++; break;
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Loop();
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(IsAttyTrueBehaviourNoError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(!ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();


        DEF_TEST(IsAttyTrueBehaviourOnError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
)" "\01" R"(
=
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "?\n"); break; // ?
                    case 3: ASSERT(s == "2\n"); break; // 2
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(IsAttyFalseBehaviourOnError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                TEST_isatty = false;
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
)" "\01" R"(
=
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "?\n"); break; // ?
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
                TEST_isatty = true;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 2);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(IsAttyFalseBehaviourNoError) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                TEST_isatty = false;
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
Q
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
                TEST_isatty = true;
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                    ASSERT(!ex.ExitedWithError());
                }
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(HModeReportsSameErrorReportedBy_h) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
)" "\01" R"(
h
H
)" "\01" R"(
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                auto line3 = std::make_shared<std::string>();
                g_state.writeStringFn = [numLinesRead, line3](std::string const& s) {
                    ++(*numLinesRead);
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "?\n"); break; // ?
                    case 3: ASSERT(s.size() && s != "?\n"); *line3 = s; break;
                    case 4: ASSERT(s != "?\n"); ASSERT(s == *line3); break; // w/e error code there was
                    default:
                        fprintf(stderr, "Unexpected string: %s", s.c_str());
                        fprintf(stderr, "Read %d already\n", *numLinesRead);
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                }
                ASSERT( (*numLinesRead) == 4);
            } TEST_RUN_END();
        } END_TEST();

