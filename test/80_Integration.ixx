
        SUITE_SETUP() {
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
        DEF_TEST(LoadFile) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                    case 0:
                        (*statew)++;
                        {
                            int bytes = 0;
                            try {
                                bytes = std::atoi(s.c_str());
                            } catch(...) {
                                ASSERT(bytes > 0);
                            }
                        }
                        break;
                    default:
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
                Commands.at('E')(Range(), "test\\twolines.txt");
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(LoadFileUTF8Bom) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "17\n"); break; // byte count
                    case 2: ASSERT(s == "Line 1\n"); break;
                    case 3: ASSERT(s == "Line 2\n"); break;
                    default:
                        fprintf(stderr, "Unexpected string %s", s.c_str());
                        ASSERT(!"should not print so much");
                        break;
                    }
                };
            } TEST_SETUP_END();
            TEST_TEARDOWN() {
                setup();
            } TEST_TEARDOWN_END();
            TEST_RUN() {
                Commands.at('E')(Range(), "test\\utf8bom.txt");
                Commands.at('p')(Range::R(1, 2), "");
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();


        DEF_TEST(EditFileAndPrint) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
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
                Commands.at('E')(Range(), "test\\twolines.txt");
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(EditFileAndPrintWithNumbers) {
            auto numLinesRead = new int(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: break; case 2: ASSERT(s == "4\tLine 4\n"); break;
                    case 3: ASSERT(s == "5\tLine 5\n"); break;
                    default:
                        fprintf(stderr, "Unexpected string %d: %s", *numLinesRead, s.c_str());
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
                Commands.at('E')(Range(), "test\\tenlines.txt");
                Commands.at('p')(Range::R(4, 5), "n test\\tenlines.txt");
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(EditFileAnd_n) {
            auto numLinesRead = new int(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: break; case 2: ASSERT(s == "4\tLine 4\n"); break;
                    case 3: ASSERT(s == "5\tLine 5\n"); break;
                    default:
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
                Commands.at('E')(Range(), "test\\tenlines.txt");
                Commands.at('n')(Range::R(4, 5), "");
                ASSERT( (*numLinesRead) == 3);
            } TEST_RUN_END();
        } END_TEST();


        DEF_TEST(CommentDoesNothing) {
            auto numLinesRead = new int(0);
            TEST_SET_EXTRA(numLinesRead);
            TEST_SETUP() {
                auto statew = std::make_shared<int>(0);
                g_state.writeStringFn = [statew, numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*statew) {
                   default:
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
                Commands.at('#')(Range(), "bla bla bla");
                ASSERT( (*numLinesRead) == 0);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(SetAndRecallFilename) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "TestPassed\n"); break;
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
                Commands.at('f')(Range(), "TestPassed");
                Commands.at('f')(Range(), "");
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(zMotion) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1\n"); break;
                    case 2: ASSERT(s == "Line 2\n"); break;
                    case 3: ASSERT(s == "Line 3\n"); break;
                    case 4: ASSERT(s == "Line 4\n"); break;
                    case 5: ASSERT(s == "Line 5\n"); break;
                    case 6: ASSERT(s == "Line 6\n"); break;
                    case 7: ASSERT(s == "Line 7\n"); break;
                    case 8: ASSERT(s == "Line 8\n"); break;
                    case 9: ASSERT(s == "Line 9\n"); break;
                    case 10: ASSERT(s == "Line 10\n"); break;
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
                Commands.at('z')(Range::S(1), "3");
                Commands.at('z')(Range::S(Range::Dot(1)), "");
                Commands.at('z')(Range::S(Range::Dot(1)), "");
                Commands.at('z')(Range::S(Range::Dot(1)), "");
                ASSERT( (*numLinesRead) == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(dLines) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1\n"); break;
                    case 2: ASSERT(s == "Line 2\n"); break;
                    case 3: ASSERT(s == "Line 3\n"); break;
                    case 4: ASSERT(s == "Line 4\n"); break;
                    case 5: ASSERT(s == "Line 7\n"); break;
                    case 6: ASSERT(s == "Line 8\n"); break;
                    case 7: ASSERT(s == "Line 9\n"); break;
                    case 8: ASSERT(s == "Line 10\n"); break;
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
                Commands.at('d')(Range::R(5, 6), "");
                ASSERT(Range::Dot() == 5);
                ASSERT(g_state.dirty);
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                ASSERT( (*numLinesRead) == 8);
            } TEST_RUN_END();
        } END_TEST();
        
        DEF_TEST(cLines) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(Inserted Line 1
Inserted Line 2
.
)";
                    ASSERT(*state < stuff.size());
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1\n"); break;
                    case 2: ASSERT(s == "Line 2\n"); break;
                    case 3: ASSERT(s == "Line 3\n"); break;
                    case 4: ASSERT(s == "Line 4\n"); break;
                    case 5: ASSERT(s == "Inserted Line 1\n"); break;
                    case 6: ASSERT(s == "Inserted Line 2\n"); break;
                    case 7: ASSERT(s == "Line 7\n"); break;
                    case 8: ASSERT(s == "Line 8\n"); break;
                    case 9: ASSERT(s == "Line 9\n"); break;
                    case 10: ASSERT(s == "Line 10\n"); break;
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
                Commands.at('c')(Range::R(5, 6), "");
                ASSERT(Range::Dot() == 6);
                ASSERT(g_state.dirty);
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                ASSERT( (*numLinesRead) == 10);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(InsertALine) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(Inserted Line 1
Inserted Line 2
.
)";
                    ASSERT(*state < stuff.size());
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1\n"); break;
                    case 2: ASSERT(s == "Line 2\n"); break;
                    case 3: ASSERT(s == "Line 3\n"); break;
                    case 4: ASSERT(s == "Line 4\n"); break;
                    case 5: ASSERT(s == "Line 5\n"); break;
                    case 6: ASSERT(s == "Inserted Line 1\n"); break;
                    case 7: ASSERT(s == "Inserted Line 2\n"); break;
                    case 8: ASSERT(s == "Line 6\n"); break;
                    case 9: ASSERT(s == "Line 7\n"); break;
                    case 10: ASSERT(s == "Line 8\n"); break;
                    case 11: ASSERT(s == "Line 9\n"); break;
                    case 12: ASSERT(s == "Line 10\n"); break;
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
                Commands.at('i')(Range::S(6), "");
                ASSERT(Range::Dot() == 7);
                ASSERT(g_state.dirty);
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                ASSERT( (*numLinesRead) == 12);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(AppendSomeLine) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                auto state = std::make_shared<int>(0);
                g_state.readCharFn = [state]() -> char {
                    std::string stuff = R"(Inserted Line 1
Inserted Line 2
.
)";
                    ASSERT(*state < stuff.size());
                    return stuff[(*state)++];
                };
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Inserted Line 1\n"); break;
                    case 2: ASSERT(s == "Inserted Line 2\n"); break;
                    case 3: ASSERT(s == "Line 1\n"); break;
                    case 4: ASSERT(s == "Line 2\n"); break;
                    case 5: ASSERT(s == "Line 3\n"); break;
                    case 6: ASSERT(s == "Line 4\n"); break;
                    case 7: ASSERT(s == "Line 5\n"); break;
                    case 8: ASSERT(s == "Line 6\n"); break;
                    case 9: ASSERT(s == "Line 7\n"); break;
                    case 10: ASSERT(s == "Line 8\n"); break;
                    case 11: ASSERT(s == "Line 9\n"); break;
                    case 12: ASSERT(s == "Line 10\n"); break;
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
                Commands.at('a')(Range::S(0), "");
                ASSERT(Range::Dot() == 2);
                ASSERT(g_state.dirty);
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                ASSERT( (*numLinesRead) == 12);
            } TEST_RUN_END();
        } END_TEST();

        DEF_TEST(jLinesBasic) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1Line 2Line 3Line 4Line 5\n"); break;
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
                Commands.at('j')(Range::R(1, 5), "");
                ASSERT(Range::Dot() == 1);
                ASSERT(g_state.dirty);
                Commands.at('p')(Range::S(1), "");
                ASSERT( (*numLinesRead) == 1);
            } TEST_RUN_END();
        } END_TEST();


        DEF_TEST(jLines) {
            auto numLinesRead = std::make_shared<int>(0);
            TEST_SETUP() {
                g_state.writeStringFn = [numLinesRead](std::string const& s) {
                    (*numLinesRead)++;
                    switch(*numLinesRead) {
                    case 1: ASSERT(s == "Line 1Line 2Line 3Line 4Line 5\n"); break;
                    case 2: ASSERT(s == "Line 6, Line 7, Line 8, Line 9, Line 10\n"); break;
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
                Commands.at('j')(Range::R(1, 5), "");
                ASSERT(Range::Dot() == 1);
                ASSERT(g_state.dirty);
                Commands.at('j')(Range::R(2, Range::Dollar()), ", ");
                ASSERT(Range::Dot() == 2);
                ASSERT(g_state.dirty);
                Commands.at('p')(Range::R(1, Range::Dollar()), "");
                ASSERT( (*numLinesRead) == 2);
            } TEST_RUN_END();
        } END_TEST();

