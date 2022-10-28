Shell commands
==============

*Disclaimer: I distinctly remember writing this file 3 years ago, but I guess I never added it to git. Let's try again:*

Forms:

| Form             | Effect                                    |
|------------------|-------------------------------------------|
| `! command args` | executes `cmd /s /c "command args"`. The character `$` is replaced by the file name (because `%THESE_ARE_VARIABLES%`). I've noticed the growing popularity of powershell recently, so this may have been an odd choice. Eh well. |
| `N,M! command args` | Passes the lines from `N` to `M` to `cmd /s /c "command args"` and replaces the range with the output of the command |
| `N,Mw! command args` | Passes the lines from `N` to `M` to `cmd /s /c "command args"`, but does not change anything in the buffer |
| `Nr! command args` | executes `cmd /s /c "command args"` and appends the output after line `N` |
| `!!`             | refers to the last command; think of it as "if `command` is `!`, then use the previous command line"; applies to all cases IIRC? |

Note, there is some slight weirdness in how shell input is handled. If you are piping one command into another, JakED both reads and writes UTF-8. 
When it comes to the [Console](https://learn.microsoft.com/en-us/windows/console/console-reference), we have to be UCS-2/UTF-16. I believe that in some
strange cases, copy/pasting from the actual console window *will* result in UCS-2/UTF-16 encoded text, but you'll have to trust, that both internally and
via pipes, we only ever use UTF-8. And when I say JakED uses UTF-8, it uses it about as well as any legacy ASCII application would, but handling text on a line-by-line basis hides this pretty well.

Another consequence of the console api is that we need to be upfront about how much
input we can accept "per line". It may seem like lines are limited to 4096 (or whatever I hardcoded), but that's how much text you can accept via `ReadConsoleW`.
Command line programs piping into JakED (or out of it) can use whatever line lengths they want, and interactively you can make lines longer with the `j` command.

Piping to shell escapes requires a whole bunch of threads to be set up, so don't let those frighten you.
