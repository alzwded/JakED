
        SUITE_SETUP() {
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
                Commands.at('p')(Range::R(1, Range::Dollar()), "test\\twolines.txt");
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
