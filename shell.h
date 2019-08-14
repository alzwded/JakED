/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <functional>

class Process
{
    Process();
public:
    // Spawns a new process using cmdLine and waits for the process to complete.
    // If cmdLine contains one or more unescaped "$"s, it is replaced with
    // defaultFileName.
    // The emitter and sink, if non-null, are run on separate threads, so
    // make sure they don't share data in a way that would cause a race
    // condition.
    // If emitter is empty, and stdin is a tty, the process will use
    // standard input instead. If stdin is not a tty, stdin will be closed.
    // If emitter is defined, all input until the first EOF will be passed
    // to the spawned process.
    // If sink is empty, the process will use standard output.
    // If sink is non-empty, the process' output will be passed to this
    // function in chunks terminated by EOL. If the command's output ended
    // with a line not terminated by EOL, SpawnAndWait will pass the extra
    // EOL, such that the output of a command is defined as a series of
    // strings terminated by EOL
    static void SpawnAndWait(
            std::string const& defaultFileName,
            std::string const& cmdLine,
            std::function<int()> emitter,
            std::function<void(std::string const&)> sink);
};

#endif
