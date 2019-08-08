/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "shell.h"

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
                ((MethodAndHandle*)lpParameter)->fn;
            ));
    if(!h || h == INVALID_HANDLE_VALUE) return 0;
    // TODO
    CloseHandle(h);
    return 127;
}

DWORD WINAPI WriteWorker(LPVOID lpParameter) 
{
    HANDLE h = ((MethodAndHandle*)lpParameter)->h;
    std::function<void(std::string const&)> readCharFn = *(
            (std::function<void(std::string const&)>*)(
                ((MethodAndHandle*)lpParameter)->fn;
            ));
    if(!h || h == INVALID_HANDLE_VALUE) return 0;
    // TODO
    CloseHandle(h);
    return 127;
}

Process::SpawnAndWait(
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
    HANDLE myOut, myIn;
    BOOL hr;
    SetLastError(0);
    hr = CreatePipe(&procIn, &myOut, NULL, 0);
    if(!hr) {
        fprintf(stderr, "Failed to create pipe: %d\n", GetLastError());
        throw std::runtime_error("Failed to create pipe");
    }
    hr = CreatePipe(&myIn, &procOut, NULL, 0);
    if(!hr) {
        fprintf(stderr, "Failed to create pipe: %d\n", GetLastError());
        throw std::runtime_error("Failed to create pipe");
    }

    int which = ((readCharFn) ? 1 : 0) + ((writeStringFn) ? 2 : 0);
    switch(which) {
    case 0: // just "!..." or "!!"
        CloseHandle(myOut);
        CloseHandle(myIn);
        myIn = NULL;
        myOut = NULL;
        if(ISATTY(_fileno(stdin))) {
            CloseHandle(procIn);
            closeProcIn = false;
            procIn = GetStdHandle(STD_INPUT_HANDLE);
        } else {
            // OK, pass the read end of the closed pipe
        }
        if(ISATTY(_fileno(stdout))) {
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_ERROR_HANDLE);
        }
        break;
    case 1: // we are piping to the process
        CloseHandle(myIn);
        myIn = NULL;
        if(ISATTY(_fileno(stdout))) {
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
            CloseHandle(procOut);
            closeProcOut = false;
            procOut = GetStdHandle(STD_ERROR_HANDLE);
        }
        break;
    case 2: // we are piping from the process
        CloseHandle(myOut);
        myOut = NULL;
        if(ISATTY(_fileno(stdin))) {
            CloseHandle(procIn);
            closeProcIn = false;
            procIn = GetStdHandle(STD_INPUT_HANDLE);
        } else {
            // OK, pass the read end of the closed pipe
        }
        break;
    case 3: // N,N!... or N,N!!, i.e. filtering through process
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
    // TODO inputCommandLine =~ s/(?!\\)$/defaultFileName/g
    commandLine << sysdirmaybebackslash << "cmd.exe /s /c \"" << inputCommandLine << "\"";
    std::string multiByteString = commandLine.str();
    // TODO AtFunctionExit free
    wchar_t* wideString = (wchar_t*)calloc(multiByteString.size(), sizeof(whar_t));
    int cchWideChar = (int)multiByteString.size();
    int len = MultiByteToWideChar(CP_UTF8,
            0 /*because CP_UTF8*/,
            (int)multiByteString.size(),
            multiByteString.data(),
            wideString, 
            cchWideChar);
    if(len == 0) throw std::runtime_error("Failed to convert %s to unicode\n", multiByteString.c_str());
    STARTUPINFO startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = 0
        | STARTF_USESTDHANDLES // implies bInheritHandles in CreateProcessW
        ;
    startupInfo.hStdInput = procIn;
    startupInfo.hStdOutput = procOut;
    startupInfo.hStdError = procErr;
    DWORD dwFlags = 0
        | CREATE_NO_WINDOW // I don't want this to be an ncurses interactive thing, but it can be a windows application
        | CREATE_SUSPENDED // I'll tell it when to run with ResumeThread(hProcess)
        ;
    HANDLE hProcess = CreateProcessW(
            NULL, // application
            wideString, // command line
            NULL, // sec context
            NULL, // sec context
            TRUE, // bInheritHandles, because we're passing pipes
            dwFlags,
            NULL, // use my environment
            NULL, // use my cwd
            &startupInfo);
    if(!hProcess || hProcess == INVALID_HANDLE_VALUE) {
        fprintf("Failed to spawn process: %d\n", GetLastError());
        throw std::runtime_error("Failed to spawn process");
    }

    MethodAndHandle readThreadData = { myIn, &readCharFn };
    HANDLE hRead = CreateThread(
            NULL, // sec atts
            0, // default size; we're not calling much, so we should be fine; well, except the mb2wcs functions, so... IDK. It says it defaults to 1MB
            &ReadWorker,
            &readThreadData,
            0, // flags
            NULL); // thread id
    MethodAndHandle writeThreadData = { myOut, &writeStringFn };
    HANDLE hWrite = CreateThread(
            NULL,
            0,
            &WriteWorker,
            &writeThreadData,
            0,
            NULL);

    // start the actual process
    ResumeThread(hProcess);
    DWORD waitHR = WaitForSingleObject(hProcess, INFINITE);
    // TODO check waitHR

    if(CtrlC()) {
        // TODO
        CancelIOEx(myIn, NULL);
        CancelIOEx(myOut, NULL);
    }

    HANDLE workers[] = { hRead, hWrite };
    waitHR = WaitForMultipleObjects(
            2,
            workers,
            TRUE,
            INFINITE); // TODO wait for 5s and CancelIOEx

    free(wideString);
    CloseHandle(hProcess);
    CloseHandle(hRead);
    CloseHandle(hWrite);
    if(closeProcIn) CloseHandle(procIn);
    if(closeProcOut) CloseHandle(procOut);

    // done
    return;
}
