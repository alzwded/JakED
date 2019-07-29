Commands to implement
=====================

Navigation
----------

+ [x] N
+ [x] ,
+ [x] % means ,
+ [x] $
+ [x] .
+ [x] offsets
+ [x] +
+ [x] -
+ [ ] ^ means -
+ [x] ;
+ [x] 'r
+ [ ] ranges which are not explicitly numeric print the line number instead/as well

Regex
-----

+ [x] /RE/
+ [x] ?RE?
+ [x] //
+ [x] ??
+ [x] ranges based off of regexes
+ [ ] g//
  * [ ] mutiline "command-list"
+ [ ] g/old/s//new/ applies s/old/new/ on all matching lines with no error reported
+ [ ] G//
+ [ ] v//
+ [ ] V//
+ [x] s//
+ [x] s//g
+ [ ] s//N (replace only Nth match)
+ [ ] s//p
+ [ ] s//gp
+ [ ] s//Np
+ [ ] s

Display
-------

+ [x] .,.p
+ [x] .,.l
+ [x] .,.pn
+ [x] .,.n
+ [x] zN
+ [x] =
+ [x] <CR>
+ [ ] Interactive write string prints UTF16 so that conhost shows unicode properly

Misc
----

+ [ ] u, see [design document](UndoAndSwapFile.md)
  * [ ] multilevel undo. Requires a simple-ish change, i.e. the undo reg points to a dummy line which points to the previous undo subhead, size is 8, and the text is a pointer to the undo command:
    ```
    undo:       pPrevUndo 8 pUndoCmd1
    ...
    pPrevUndo:  0         8 pUndoCmd0       ; e.g. last available undo
    ...
    pUndoCmd1:  pLines    4 "1,3c"
    pLines:     0         2 "hi"
    pUndoCmd0:  pLines0   2 "1d"
    pLines0:    0         2 "ho"
    ...

    ```
    Linking a new undo command links the new subhead to the previous subhead and creates the new undo command line + additional lines. Invoking undo "pops" the subhead and points the global undo register to the previous subhead.
+ [x] P
+ [x] H
+ [x] h
+ [x] .,.#
+ [ ] ! (execute some command and output a '!')
+ [ ] !! (repeat last shell command)
+ [ ] ~~% means~~ $ means default filename in shell commands (since the chars for env vars are swapped, might as well swap them back to avoid too much aprsing; $ doesn't mean _too much_ on windows; I'll maybe add an escape char in the future to support reading silly device names or alternate data streams)

Commands
--------

+ [x] k

*Insert mode*:

+ [x] Ni
+ [x] test inserted text preserves registers correctly
+ [x] Na
+ [x] test registers get clobbered by c

*Line transfer, register update*:

+ [x] j
+ [x] jSEPARATOR join lines using SEPARATOR (extension)
+ [ ] .,.m.
+ [ ] move preserves registers
+ [ ] .,.t.

*Cut buffer, register update*:

+ [x] cut buffer
+ [x] .,.c
+ [x] .,.d
+ [x] test registers get clobbered by d
+ [x] .,.y
+ [x] .x
+ [x] test x preserves registers like i

Swapfile
--------

+ [x] swap file: see [design document](UndoAndSwapFile.md)
  * [x] On w/W, buffer is cleared and re-initialized with current cut buffer
  * [ ] if no I/O can be performed at all, keep everything in a giant stringstream
  * [x] on clean exit, delete file
  * [ ] Use MapViewOfFile and CreateFileMapping instead of CRT file I/O because I/O is through the roof. CreateFileMapping can be used to grow the swap file as needed (say, `4k -> 8k -> ... 2**27 -> 2 * 2**27 -> 3 * 2**27 ...`; `2**27` should be 100MB or so). MapViewOfFile should be done chunked, because UnmapViewOfFile will schedule changed pages for commit (async) which would keep it pretty fresh. In this implementation, the "no I/O can be performed" scenario can cause it to work with swapfile backed memory (i.e. normal memory) while keeping the same API. Might work on this once I have more test coverage, since the slow CRTIO implementation at least works for now. See [remapper](../experiments/remapper.c)
+ [x] large file support (i.e. use getfpos and setfpos and update header to be 64bit) – it's sort-of there, but uses a whole lotta disk space
  * [ ] command line flag for no swap file and env var to set swap file default directory
+ [x] rewrite swap file to be a giant linked list + a bunch of registers

I/O
---

+ [x] r [F]
  * [ ] be able to read test\OctrlChars.txt; right now it fails at ^Z (windows EOT? can that even be parsed?)
    this might be a non-issue, although it would be nice to be able to read everything. Need to check if real ed can read ^D or something like that.
+ [x] e [F]
+ [x] E [F]
+ [x] f FILE
+ [x] .,.w
+ [x] .,.W
+ [x] q
+ [x] Q
+ [ ] ^V literal input
+ [x] handle `CTRL_C` and `CTRL_BRK` elegantly
+ [ ] dump swapfile to a readable file if console's closed; or call `_exit` which theoretically won't purge the tmp file? or was that `__exit`? or `_quick_exit`? meh

*Shell versions*:

+ [ ] r !sh
+ [ ] f !sh
+ [ ] e !sh
+ [ ] w !sh

Internals, Externals and Other Tasks
------------------------------------

+ [x] refactor string processing – done, implemented swapfile.
+ [x] scripting behaviour (cancel on error if !isatty)
+ [/] test that e&q cowardly refuse to exit
+ [x] utf8 with BOM
+ [ ] command line argument for P
+ [ ] arg: `-s` (suppress diagnostics)
+ [ ] start with empty file
+ [ ] jaked --help
+ [ ] jaked --version
+ [ ] some nice html documentation, like `ed`'s man page
+ [ ] read windows-native UTF16 files (`rutf16 somefile?`)
+ [ ] don't crash if file can't be read
+ [ ] the debug build (used to run external tests) should read a timeout from an env var to kill itself if the test takes too long
