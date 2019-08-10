#include "../shell.h"
#include "../common.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <functional>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>


volatile std::atomic<bool> ctrlc = false;
volatile std::atomic<bool> ctrlint = false;

struct Reader {
    void operator()(std::string const& s) {
#if 0
        fprintf(stderr, "I got this string: ");
        for(auto&& c : s) {
            fprintf(stderr, "%.2X", c);
        }
        fprintf(stderr, "\n");
#endif
        fprintf(stdout, "test:\t%s\n", s.c_str());
    }
};

struct Writer {
    std::string m_text;
    size_t m_state;
    Writer(std::string const& s) : m_text(s), m_state(0) {}
    int operator()() {
        //fprintf(stderr, "%zd %zd\n", m_state, m_text.size());
        if(m_state >= m_text.size()) return EOF;
        //fprintf(stderr, "Giving: %c\n", m_text[m_state]);
        return m_text[m_state++];
    }
};

int call(int argc, char** argv)
{
    std::function<int()> writer;
    std::function<void(std::string const&)> reader;

    switch(*argv[1]) {
    case '0':
        fprintf(stderr, "==========================\n");
        fprintf(stderr, "testing just running process\n");
        fprintf(stderr, "tty: 1> Hey, hi, hello\n"
                        "     0< (if interactive, it reads)\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "pipe:1> Hey, hi, hello (test.tmp)\n"
                        "     0< (test.tmp)\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "--------------------------\n");
        {
            Process::SpawnAndWait(
                    "test.tmp",
#if 0
                    "systeminfo",
                    "type testshell.cpp",
#else
                    "testshell.exe t",
#endif
                    writer,
                    reader);
        }
        fprintf(stderr, "==========================\n");
        break;
    case '1':
        fprintf(stderr, "==========================\n");
        fprintf(stderr, "testing writing to process\n");
        fprintf(stderr, "tty: 1> Hey, hi, hello\n"
                        "     0< Message from the world!\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "pipe:1> Hey, hi, hello (test.tmp)\n"
                        "     0< Message from the world! (test.tmp)\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "--------------------------\n");
        {
            writer = Writer("Message from the world!\n");
            //writer = Writer(std::string(100000, 'A')); #stress test
            Process::SpawnAndWait(
                    "test.tmp",
                    "testshell.exe t",
                    writer,
                    reader);
        }
        fprintf(stderr, "==========================\n");
        break;
    case '2':
        fprintf(stderr, "==========================\n");
        fprintf(stderr, "testing filtering through process\n");
        fprintf(stderr, "tty: test\t1> Hey, hi, hello\n"
                        "     test\t0< Message from the world!\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "pipe:test\t1> Hey, hi, hello (test.tmp)\n"
                        "     test\t0< Message from the world! (test.tmp)\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "--------------------------\n");
        {
            writer = Writer("Message from the world!\n");
            reader = Reader();
            Process::SpawnAndWait(
                    "test.tmp",
                    "testshell.exe t",
                    writer,
                    reader);
        }
        fprintf(stderr, "==========================\n");
        break;
    case '3':
        fprintf(stderr, "==========================\n");
        fprintf(stderr, "testing reading from process which reads stdin normally\n");
        fprintf(stderr, "tty: test:\t1> Hey, hi, hello\n"
                        "     test:\t0< (if interactive, it reads)\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "pipe:test:\t1> Hey, hi, hello (test.tmp)\n"
                        "     test:\t0< (test.tmp)\n"
                        "     2> ferror(stdin): 0\n");
        fprintf(stderr, "--------------------------\n");
        {
            reader = Reader();
            Process::SpawnAndWait(
                    "test.tmp",
                    "testshell.exe t",
                    //"systeminfo", // at the time of writing, this also works correctly
                    writer,
                    reader);
        }
        fprintf(stderr, "==========================\n");
        break;
    case '4':
        fprintf(stderr, "==========================\n");
        fprintf(stderr, "testing conversion of $ to filename\n");
        fprintf(stderr, "tty: test:\tLine 1\n"
                        "     test:\tLine 2\n"
                        "");
        fprintf(stderr, "pipe:test:\tLine 1\n"
                        "     test:\tLine 2\n"
                        "");
        fprintf(stderr, "--------------------------\n");
        {
            reader = Reader();
            Process::SpawnAndWait(
                    R"(..\test\twolines.txt)",
                    "type $",
                    writer,
                    reader);
        }
        fprintf(stderr, "==========================\n");
        break;
    default:
        fprintf(stderr, "not a valid test number\n");
        return 1;
    }

    return 0;
}

VOID CALLBACK killSelf(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    if(IsDebuggerPresent()) return;
    if(TimerOrWaitFired) {
        fprintf(stderr, "Test suite max time hit\n");
        fflush(stderr);
        std::stringstream ss;
        ss << "taskkill /t /f /pid " << GetCurrentProcessId();
        system(ss.str().c_str());
        exit(127);
    }
}

int main(int argc, char* argv[])
{
    HANDLE hTimer;
    int allowedMaxTime = 5000;
    if(!IsDebuggerPresent()) {
        CreateTimerQueueTimer(
                &hTimer,
                NULL, 
                &killSelf, 
                NULL, 
                allowedMaxTime, 
                0, 
                WT_EXECUTEONLYONCE);
    }



    if(argc != 2) {
        fprintf(stderr, "Argv1 must be a test number\n");
        abort();
    }

    if(*argv[1] == 't') {
        puts("1> Hey, hi, hello");
        putchar('0'); putchar('<'); putchar(' ');
        while(!feof(stdin) && !ferror(stdin)) {
            auto c = getchar();
            if(c == EOF) break;
            putchar(c);
        }
        putchar('\n');
        fprintf(stderr, "2> ferror(stdin): %d\n", ferror(stdin));
        exit(0);
    }

    try {
        return call(argc, argv);
    } catch(std::exception& ex) {
        fprintf(stderr, "Caught: %s\n", ex.what());
        exit(1);
    } catch(...) {
        fprintf(stderr, "Caught some kind of exception\n");
        exit(1);
    }
}
