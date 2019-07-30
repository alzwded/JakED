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

        DEF_TEST(RSetsDotCorrectly) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\twolines.txt
1r test\twolines.txt
.=
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: break;
                    case 2: break;
                    case 3: ASSERT(s == "3\n"); break;
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

        DEF_TEST(zScrollingParsesCorrectly) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\tenlines.txt
1z3
z
z
z
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "Line 1\n"); break;
                    case 3: ASSERT(s == "Line 2\n"); break;
                    case 4: ASSERT(s == "Line 3\n"); break;
                    case 5: ASSERT(s == "Line 4\n"); break;
                    case 6: ASSERT(s == "Line 5\n"); break;
                    case 7: ASSERT(s == "Line 6\n"); break;
                    case 8: ASSERT(s == "Line 7\n"); break;
                    case 9: ASSERT(s == "Line 8\n"); break;
                    case 10: ASSERT(s == "Line 9\n"); break;
                    case 11: ASSERT(s == "Line 10\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT( (*numLinesRead) == 11);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(InsertPreservesRegisters) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> int {
                    // # starting to look cryptic
                    auto s = R"(E test\tenlines.txt
5kq
5i
Some text
.
'qn
)";
                    if(*state >= strlen(s)) return EOF;
                    return s[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: break; // byte count
                    case 2: ASSERT(s == "6\tLine 5\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT( (*numLinesRead) == 11);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(cClobbersRegisters) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(5kq
6kw
7ke
5,6c
Inserted Line 1
Inserted Line 2
.
'e=
'q=
'w=
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "7\n"); break;
                    case 2: ASSERT(s == "?\n"); break;
                    case 3: ASSERT(s == "?\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT(Range::Dot() == 6);
                ASSERT(g_state.dirty);
                ASSERT(*numLinesRead == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(jLinesCannotJoin0) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(0,1j
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };

                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "?\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT(Range::Dot() == 1);
                ASSERT(!g_state.dirty);
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(jLinesBad) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(1,1j
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };

                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "?\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT(Range::Dot() == 1);
                ASSERT(!g_state.dirty);
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(jDefault) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(j
1,2p
Q
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };

                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1Line 2\n"); break;
                    case 2: ASSERT(s == "Line 3\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT(g_state.dirty);
                ASSERT( (*numLinesRead) == 2);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(aLeavesTheCorrectNumberOfLinesAfterwards) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(1,$d
a
Line 1
.
=
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };

                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "1\n"); break;
                    default:
                        fprintf(stderr, "Extra string: %s", s.c_str());
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
                ASSERT(g_state.dirty);
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(d0x1x) {
            WriteFn fn({
                    "2",
                    "3",
                    "12",
                    "Line 1",
                    "Line 1",
                    "Line 2",
                    "Line 2",
                    "Line 3",
                });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(1,2d
0x
.=
1x
.=
=
1,5p
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                }
                ASSERT(g_state.dirty);
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(y0x12x) {
            WriteFn fn({
                    "2",
                    "14",
                    "14",
                    "Line 2",
                    "Line 3",
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
                    "Line 2",
                    "Line 3",
                });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(2,3y
0x
.=
12x
.=
=
1,$p
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                }
                ASSERT(g_state.dirty);
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(iOnLastLine) {
            WriteFn fn({
                    "11",
                    "Line 1",
                    "Line 2",
                    "Line 3",
                    "Line 4",
                    "Line 5",
                    "Line 6",
                    "Line 7",
                    "Line 8",
                    "Line 9",
                    "It worked",
                    "Twice",
                    "Line 10",
                });
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.line = 10;
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(i
It worked
.
11i
Twice
.
.=
,p
)";
                    if(*state >= stuff.size()) return EOF;
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = fn;
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                try {
                    Loop();
                } catch(application_exit& ex) {
                }
                ASSERT(g_state.dirty);
                ASSERT(g_state.nlines == 12);
                fn.Assert();
            } TEST_RUN_END();
        } END_TEST();
