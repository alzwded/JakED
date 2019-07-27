Commands to implement
=====================

Navigation
----------

+ [x] N
+ [x] ,
+ [ ] % means ,
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

+ [ ] /RE/
+ [ ] ?RE?
+ [ ] //
+ [ ] ??
+ [ ] g//
+ [ ] g/old/s//new/ applies s/old/new/ on all matching lines with no error reported
+ [ ] G//
+ [ ] v//
+ [ ] V//
+ [ ] s//
+ [ ] s//g
+ [ ] s//N (replace only Nth match)
+ [ ] s

Display
-------

+ [x] .,.p
+ [ ] .,.l
+ [x] .,.pn
+ [x] .,.n
+ [x] zN
+ [x] =
+ [x] <CR>

Misc
----

+ [ ] u, see [design document](UndoAndSwapFile.md)
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
+ [x] swap file: see [design document](UndoAndSwapFile.md)
  * [x] On w/W, buffer is cleared and re-initialized with current cut buffer
  * [ ] if no I/O can be performed at all, keep everything in a giant stringstream
  * [x] on clean exit, delete file

+ [x] .,.c
+ [x] .,.d
+ [x] test registers get clobbered by d
+ [ ] .,.y
+ [ ] .x
+ [ ] test x preserves registers like i

I/O
---

+ [x] r [F]
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
+ [x] large file support (i.e. use getfpos and setfpos and update header to be 64bit) – it's sort-of there, but uses a whole lotta disk space
  * [ ] command line flag for no swap file and env var to set swap file default directory
+ [ ] rewrite swap file to be a giant linked list + a bunch of registers
+ [ ] don't crash if file can't be read
+ [ ] the debug build (used to run external tests) should read a timeout from an env var to kill itself if the test takes too long
