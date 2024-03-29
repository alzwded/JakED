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

Regex
-----

+ [x] /RE/
+ [x] ?RE?
+ [x] //
+ [x] ??
+ [x] ranges based off of regexes
+ [x] g//
  * [x] mutiline "command-list"
  * [x] a, i and c support
  * [x] g/old/s//new/ applies s/old/new/ on all matching lines with no error reported
  * [ ] std::regex is apparently superslow... try passing the compile flag, or find a snappier regex library (like the one on FreeBSD, that one's BSD licensed)
+ [x] G//
+ [x] v//
+ [x] V//
+ [ ] g, v, G, V no-regex shortcuts (can't cope with lack of tail)
+ [x] s///
+ [x] s///g
+ [x] s///N (replace only Nth match)
+ [x] s///p
+ [x] s///gp
+ [x] s///Np
+ [x] s
+ [x] s/re/fmt (= s/re/fmt/p)
+ [ ] s/re/ (special case, where previous fmt is kept and p is implied)

Display
-------

+ [x] .,.p
+ [x] .,.l
  + [ ] print specials chars in inverted mode
+ [x] .,.pn
+ [x] .,.n
+ [x] zN
  * [x] zNn
  * [x] zn (use stored N)
+ [x] =
+ [x] <CR>
+ [x] ~~Interactive write string prints UTF16 so that conhost shows unicode properly~~ print out UTF8 correctly
+ [x] **read** utf8 correctly on console

Misc
----

+ [x] u, see [design document](UndoAndSwapFile.md)
  * [x] validate commands
    - [x] g//s//
    - [x] m
    - [x] d
    - [x] i
    - [x] t
    - [x] r
    - [x] a
    - [x] c
    - [x] j
    - [x] s//
    - [x] x
    - [x] e
  * [x] multilevel undo. Requires a simple-ish change, i.e. the undo reg points to a dummy line which points to the previous undo subhead, size is 8, and the text is a pointer to the undo command:
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
+ [x] ~~% means~~ $ means default filename in shell commands (since the chars for env vars are swapped, might as well swap them back to avoid too much aprsing; $ doesn't mean _too much_ on windows; I'll maybe add an escape char in the future to support reading silly device names or alternate data streams)

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
+ [x] .,.m.
+ [x] move preserves registers
+ [x] .,.t.

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
  * [x] on clean exit, delete file
  * [x] Use MapViewOfFile and CreateFileMapping instead of CRT file I/O because I/O is through the roof. CreateFileMapping can be used to grow the swap file as needed (say, `4k -> 8k -> ... 2**27 -> 2 * 2**27 -> 3 * 2**27 ...`; `2**27` should be 100MB or so). MapViewOfFile should be done chunked, because UnmapViewOfFile will schedule changed pages for commit (async) which would keep it pretty fresh. In this implementation, the "no I/O can be performed" scenario can cause it to work with swapfile backed memory (i.e. normal memory) while keeping the same API. Might work on this once I have more test coverage, since the slow CRTIO implementation at least works for now. See [remapper](../experiments/remapper.c) – it's much, much faster now
+ [x] rewrite swap file to be a giant linked list + a bunch of registers
+ [x] large file support (i.e. use getfpos and setfpos and update header to be 64bit) – it's sort-of there, but uses a whole lotta disk space
+ [ ] NullImpl
+ [ ] In-memory impl based on MappedImpl (but without an explicit file)
+ [ ] command line flags:
  * [ ] `--slow-swap[=dir]` to use `FileImpl`
  * [ ] `--no-swap` to use `MemoryImpl`
  * [ ] `--swap[=dir]` to use `MappedImpl`
  * [ ] `JAKED_DEFAULT_SWAP_DIRECTORY`
  * [ ] `--recover=file`, but I probably need to align the two formats first

I/O
---

+ [x] fix the gremlines that happen when I invoke echo and read the output, it really seems like my end is reading "correctly", but I still haven't figured out where the garbage buffer overflow bleed comes from... (mb2wcs was not null terminated)

+ [x] r [F]
  * [ ] be able to read test\OctrlChars.txt; right now it fails at ^Z (windows EOT? can that even be parsed?)
    this might be a non-issue, although it would be nice to be able to read everything. Need to check if real ed can read ^D or something like that.
    I think it's because I'm using strings and fgetc or something like that, I think after switching to use pascal strings, and drastically change the way input handling is done (i.e. fgetc -> ReadFile) we'll see improvements, but that's a big task
+ [x] e [F]
+ [x] E [F]
+ [x] f FILE
+ [x] .,.w
+ [x] .,.W
+ [x] q
+ [x] Q
+ [X] ^V literal input – It's done, but it works poorly. Might revisit this one in the future
+ [x] handle `CTRL_C` and `CTRL_BRK` elegantly
+ [x] ~~dump swapfile to a readable file~~ have a recovery file if console's closed; ~~or call `_exit` which theoretically won't purge the tmp file? or was that `__exit`? or `_quick_exit`? meh~~ calls `quick_exit(127)`

*Shell versions*:

+ [x] r !sh
+ ~~f !sh~~ f is used in the shell command line, and it's not in the spec either
+ [x] e !sh
  + [x] e! and E! set dirty flag
+ [x] w !sh
+ [x] N,M!sh, N,M!!   (extension to filter text through external command)
+ [x] ! (execute some command and output a '!')
+ [x] !! (repeat last shell command)
  * [ ] refactor common code (processing !!, the sink and the emitter)

Internals, Externals and Other Tasks
------------------------------------

+ [x] refactor string processing – done, implemented swapfile.
  * [x] refactor it again to use a byte array (required for multi-level undo support without reimagining the swapfile format) –we got pointers now, woot!
+ [x] scripting behaviour (cancel on error if !isatty)
+ [x] test that e&q cowardly refuse to exit
+ [x] utf8 with BOM
+ [x] command line argument for P
+ [x] arg: `-s` (suppress diagnostics)
+ [x] start with empty file
+ [x] jaked ~~--help~~ `-h`
+ [x] jaked ~~--version~~ `-V`
+ [ ] some nice html documentation, like `ed`'s man page
+ [ ] read windows-native UTF16 files (`rutf16 somefile?`)
+ [x] don't crash if file can't be read
+ [ ] the debug build (used to run external tests) should read a timeout from an env var to kill itself if the test takes too long
+ [ ] add a command line flag for recovery

Summary
=======

Big topics:

+ [x] Shell escapes
+ [x] Command line arguments
+ [x] Undo
+ [x] Multilevel undo -- the back-end support is there, I just have to go over all commands and use an extra indirect line reference to keep track of th undo stack

Other small topics remain, but `ed` is usable even if those topics are open.
