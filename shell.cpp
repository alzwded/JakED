/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "shell.h"
#include "common.h"
#include "cprintf.h"

#include <exception>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>

struct MethodAndHandle {
    HANDLE h;
    void* fn;
};

CHAR sysdir[32767] = {0};

DWORD WINAPI ReadWorker(LPVOID lpParameter) 
{
    HANDLE h = ((MethodAndHandle*)lpParameter)->h;
    std::function<int()> readCharFn = *(
            (std::function<int()>*)(
                ((MethodAndHandle*)lpParameter)->fn
            ));
    //fprintf(stderr, "never write from a thread: %c\n", readCharFn());
    if(!h || h == INVALID_HANDLE_VALUE) return 0;
    int cc;
    while((cc = readCharFn()) != EOF) {
        DWORD numWritten; // D/C
        char buf[2] = { cc & 0xFF, '\0' };
        SetLastError(0);
        auto hr = WriteFile(
                h,
                buf,
                1,
                &numWritten,
                NULL);
        //fprintf(stderr, "never write from a thread: %d %d\n", hr, numWritten);
        if(GetLastError() == ERROR_OPERATION_ABORTED) {
            break;
        }
    }
    CloseHandle(h);
    return 0;
}

DWORD WINAPI WriteWorker(LPVOID lpParameter) 
{
    HANDLE h = ((MethodAndHandle*)lpParameter)->h;
    std::function<void(std::string const&)> writeStringFn = *(
            (std::function<void(std::string const&)>*)(
                ((MethodAndHandle*)lpParameter)->fn
            ));
    if(!h || h == INVALID_HANDLE_VALUE) return 0;
        std::stringstream buffer;
    do {
        // Clear GetLastError() because as we know nobody
        // ever does.
        SetLastError(0);
#if 0
        // TODO same as r, detect UCS-2/UTF16; assuming UTF8 right now
        WCHAR buf[256];
        CHAR buf2[256 * 5];
#else
        CHAR buf[256];
#endif
        DWORD rv;
        //fprintf(stderr, "the horrors: reading\n");
        auto hr = ReadFile(
                h,
                &buf,
                sizeof(buf)/sizeof(buf[0]) - 1,
                &rv,
                NULL);
        buf[sizeof(buf) / sizeof(buf[0]) - 1] = 
#if 0
            L'\0'
#else
            '\0'
#endif
            ;
        //fprintf(stderr, "the horrors: %d %d\n", hr, GetLastError());

        if(GetLastError() == ERROR_OPERATION_ABORTED) {
            break;
        }

        if(GetLastError() == 0 && rv > 0) {
#if 0
            // TODO same as r, detect UCS-2/UTF16; assuming UTF8 right now
            // FIXME NULL obviously terminates the string somewhere
            auto cchbuf = WideCharToMultiByte(CP_UTF8, 0, buf, rv, buf2, sizeof(buf2) - 1, NULL, NULL);
            buf2[cchbuf] = '\0';
            for(size_t i = 0; i < cchbuf; ++i) {
                buffer.push_back(buf2[i]);
            }
#else
            for(size_t i = 0; i < rv; ++i) {
                if(buf[i] == '\n') {
                    buffer << buf[i];
                    writeStringFn(buffer.str());
                    buffer.str("");
                    continue;
                }
                buffer << buf[i];
            }
#endif          
        } else if(!hr) {
            break;
        } else if(rv == 0) {
            break;
        } else {
            /* BBQ? */
        }
    } while(1);

    //fprintf(stderr, "the horrors: exiting\n");

    if(!buffer.str().empty()) {
        if(buffer.str()[buffer.str().size() - 1] != '\n')
            buffer << '\n';
        writeStringFn(buffer.str());
    }

    CloseHandle(h);
    return 127;
}

void Process::SpawnAndWait(
        std::string const& defaultFileName,
        std::string const& inputCommandLine,
        std::function<int()> readCharFn,
        std::function<void(std::string const&)> writeStringFn)
{
    // Setup logic.
    // - if ISATTY
    //     - if readCharFn, create pipe proc_stdin
    //     - else, GetStdHandle(CONSOLE_STD_INPUT)
    //     - if writeLineFn, create pipe proc_stdout
    //     - else, GetStdHandle(CONSOLE_STD_OUTPUT)
    //     - GetStdHandle(CONSOLE_STD_ERROR)
    // - if !ISATTY
    //     - if readCharFn, create pipe proc_stdin
    //     - else, NULL proc_stdin
    //     - if writeLineFn, create pipe proc_stdout
    //     - else, GetStdHandle(CONSOLE_STD_ERROR)
    //     - GetStdHandle(CONSOLE_STD_ERROR)

    // Caller thread:
    //     create required pipes and determine hProcIn, hProcOut, hProcErr
    //     spawn process
    //     spawn a thread for writing to proc stdin from readCharFn
    //     spawn a thread for read from proc stdout to writeStringFn
    // 10  WaitForSingleObject(hProcess)
    //     if(CtrlC()) { 
    //          Send ctrl-c to process
    //          CancelIoEx(proc stdin, nullptr)
    //          CancelIoEx(proc stdout, nullptr)
    //     }
    // 20  WaitForMultipleObjects(T1, T2, bWaitAll=true, timeout 5s)
    //     if wait timed out,
    //          CancelIoEx(*)
    //          GOTO 20
    //     SetLastError(0);

    // Worker thread 1:
    //     while(c from readCharFn() != EOF) {
    //         WriteFile(MultiByteToWideString(c))
    //         if interrupted return
    //     }

    // Worker thread 2:
    //     while(1) {
    //         
    //         ReadFile() => rv
    //         WideStringToMultiByte(rv) => c
    //         ss << c;
    //         WriteFile(MultiByteToWideString(c))
    //         if interrupted return
    //     }

    // TODO AtFunctionExit CloseHandles
    bool closeProcIn = true, closeProcOut = true;
    HANDLE procIn, procOut, procErr = GetStdHandle(STD_ERROR_HANDLE);
    HANDLE myOut = NULL, myIn = NULL;
    BOOL hr;

    SECURITY_ATTRIBUTES secAtts;
    memset(&secAtts, 0, sizeof(SECURITY_ATTRIBUTES));
    // Set up the security attributes struct.
    secAtts.nLength= sizeof(SECURITY_ATTRIBUTES);
    secAtts.lpSecurityDescriptor = NULL;
    secAtts.bInheritHandle = TRUE; // !!! VERY IMPORTANT

    SetLastError(0);
    cprintf<CPK::shell>("Creating pipes\n");
    hr = CreatePipe(&procIn, &myOut, &secAtts, 0);
    if(!hr) {
        fprintf(stderr, "Failed to create pipe: %d\n", GetLastError());
        throw std::runtime_error("Failed to create pipe");
    }
    hr = CreatePipe(&myIn, &procOut, &secAtts, 0);
    if(!hr) {
        fprintf(stderr, "Failed to create pipe: %d\n", GetLastError());
        throw std::runtime_error("Failed to create pipe");
    }

    SetHandleInformation(myIn, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(myOut, HANDLE_FLAG_INHERIT, 0);


    cprintf<CPK::shell>("isatty(0): %d, isatty(1): %d, isatty(2): %d\n",
            ISATTY(_fileno(stdin)),
            ISATTY(_fileno(stdout)),
            ISATTY(_fileno(stderr)));

    int which = ((readCharFn) ? 1 : 0) + ((writeStringFn) ? 2 : 0);
    switch(which) {
    case 0: // just "!..." or "!!"
        cprintf<CPK::shell>("!!: Closing myOut, myIn\n");
        CloseHandle(myOut);
        CloseHandle(myIn);
        myIn = NULL;
        myOut = NULL;
        if(ISATTY(_fileno(stdin))) {
            cprintf<CPK::shell>("Closing procIn\n");
            CloseHandle(procIn);
            closeProcIn = false;
            procIn = GetStdHandle(STD_INPUT_HANDLE);
        } else {
            // OK, pass the read end of the closed pipe
            cprintf<CPK::shell>("Stdin is not a tty\n");
        }
        if(true || ISATTY(_fileno(stdout))) { // XXX this is a hard decision on what behaviour should be...
            cprintf<CPK::shell>("Closing procOut and using STD_OUTPUT_HANDLE\n");
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
#if 1
            cprintf<CPK::shell>("Closing procOut and using STD_ERROR_HANDLE\n");
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_ERROR_HANDLE);
#else
            cprintf<CPK::shell>("Stdout is not a tty\n");
#endif
        }
        break;
    case 1: // we are piping to the process
        cprintf<CPK::shell>("w! Closing myIn\n");
        CloseHandle(myIn);
        myIn = NULL;
        if(true || ISATTY(_fileno(stdout))) { // XXX this is a hard decision on what behaviour should be...
            cprintf<CPK::shell>("Closing procOut and using STD_OUTPUT_HANDLE\n");
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
#if 1
            cprintf<CPK::shell>("Closing procOut and using STD_ERROR_HANDLE\n");
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_ERROR_HANDLE);
#else
            cprintf<CPK::shell>("Stdout is not a tty\n");
#endif
        }
        break;
    case 2: // we are piping from the process
        cprintf<CPK::shell>("r! Closing myOut\n");
        CloseHandle(myOut);
        myOut = NULL;
        if(ISATTY(_fileno(stdin))) {
            cprintf<CPK::shell>("Closing procIn\n");
            CloseHandle(procIn);
            closeProcIn = false;
            procIn = GetStdHandle(STD_INPUT_HANDLE);
        } else {
            // OK, pass the read end of the closed pipe
        }
        break;
    case 3: // N,N!... or N,N!!, i.e. filtering through process
        cprintf<CPK::shell>("N,N!!\n");
        break;
    }

    // spawn the process
    std::stringstream commandLine;
    std::string sysdirmaybebackslash;
    if(!*sysdir) {
        DWORD len = GetEnvironmentVariable(
                "SystemRoot",
                sysdir,
                sizeof(sysdir));
        if(!hr) {
            fprintf(stderr, "Failed to get %%SystemRoot%%\n", GetLastError());
            sysdir[0] = '\0';
        }
        sysdirmaybebackslash.assign(sysdir);
        if(sysdirmaybebackslash[sysdirmaybebackslash.size() - 1] != '\\') {
            sysdirmaybebackslash += '\\';
        }
        sysdirmaybebackslash += "System32\\cmd.exe";

    }
    cprintf<CPK::shell>("cmd.exe path is: %s\n", sysdirmaybebackslash.c_str());
    // TODO inputCommandLine =~ s/(?!\\)$/defaultFileName/g
    commandLine << sysdirmaybebackslash << " /s /c \"" << inputCommandLine << "\"";
    std::string multiByteString = commandLine.str();
    cprintf<CPK::shell>("Command line is %s\n", multiByteString.c_str());
    // TODO AtFunctionExit free
    wchar_t* wideString = (wchar_t*)calloc(multiByteString.size(), sizeof(wchar_t));
    int cchWideChar = (int)multiByteString.size();
    int len = MultiByteToWideChar(CP_UTF8,
            0 /*because CP_UTF8*/,
            multiByteString.data(),
            (int)multiByteString.size(),
            wideString, 
            cchWideChar);
    if(len == 0) {
        std::stringstream ss;
        ss << "Failed to convert " << multiByteString << " to unicode: " << GetLastError() << "\n";
        throw std::runtime_error(ss.str());
    }
    STARTUPINFOW startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = 0
        | STARTF_USESTDHANDLES // implies bInheritHandles in CreateProcessW
        //| CREATE_NEW_CONSOLE
        | CREATE_NO_WINDOW // I don't want this to be an ncurses interactive thing, but it can be a windows application
        ;
    startupInfo.hStdInput = procIn;
    startupInfo.hStdOutput = procOut;
    startupInfo.hStdError = procErr;
    DWORD dwFlags = 0
        | CREATE_SUSPENDED // I'll tell it when to run with ResumeThread(hProcess)
        ;
    PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof(procInfo));
    cprintf<CPK::shell>("Calling CreateProcessW\n");
    SetLastError(0);
    hr = CreateProcessW(
            NULL, // application
            wideString, // command line
            NULL, // sec context
            NULL, // sec context
            TRUE, // bInheritHandles, because we're passing pipes
            dwFlags,
            NULL, // use my environment
            NULL, // use my cwd
            &startupInfo, // startup info
            &procInfo); // output lpProcInfo
    if(!hr || procInfo.hProcess == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to spawn process: %d\n", GetLastError());
        throw std::runtime_error("Failed to spawn process");
    }

    cprintf<CPK::shell>("Spawining reader thread\n");
    MethodAndHandle readThreadData = { myOut, &readCharFn };
    HANDLE hRead = CreateThread(
            NULL, // sec atts
            0, // default size; we're not calling much, so we should be fine; well, except the mb2wcs functions, so... IDK. It says it defaults to 1MB
            &ReadWorker,
            &readThreadData,
            0, // flags
            NULL); // thread id
    cprintf<CPK::shell>("Spawining writer thread\n");
    MethodAndHandle writeThreadData = { myIn, &writeStringFn };
    HANDLE hWrite = CreateThread(
            NULL,
            0,
            &WriteWorker,
            &writeThreadData,
            0,
            NULL);


    // !!! very important:
    //     we need to close our copy of the process' handles,
    //     otherwise our workers will never know to wake up
    //     from their respective read/writes.
    //     This is based on pure observation and theory crafting,
    //     but it honestly hangs otherwise.
    if(closeProcIn) {
        cprintf<CPK::shell>("Closing procIn\n");
        CloseHandle(procIn);
    }
    if(closeProcOut) {
        cprintf<CPK::shell>("Closing procOut\n");
        CloseHandle(procOut);
    }

    cprintf<CPK::shell>("Resuming the thread of our process\n");
    // start the actual process
    ResumeThread(procInfo.hThread);
    DWORD waitHR = WaitForSingleObject(procInfo.hProcess, INFINITE);
    // TODO check waitHR

    if(CtrlC()) {
        cprintf<CPK::shell>("Cancelling all I/O\n");
        // TODO send process a termination signal and replace INFINITE above
        CancelIoEx(myIn, NULL);
        CancelIoEx(myOut, NULL);
    }

    cprintf<CPK::shell>("Waiting for workers\n");
    HANDLE workers[] = { hRead, hWrite };
    waitHR = WaitForMultipleObjects(
            2,
            workers,
            TRUE,
            INFINITE); // TODO wait for 5s and CancelIOEx, but be careful because if it's processing a large buffer (?!), the reader might still be working.

    cprintf<CPK::shell>("Workers joined, closing handles\n");

    free(wideString);
    CloseHandle(procInfo.hThread);
    CloseHandle(procInfo.hProcess);
    CloseHandle(hRead);
    CloseHandle(hWrite);

    // done
    return;
}
