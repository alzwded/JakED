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
+ [x] .,.pn
+ [x] .,.n
+ [x] zN
+ [x] =
+ [x] <CR>

Misc
----

+ [ ] u, see [UndoAndSwapFile.md]
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

+ [ ] j
+ [ ] jSEPARATOR join lines using SEPARATOR (extension)
+ [ ] .,.m.
+ [ ] move preserves registers
+ [ ] .,.t.

*Cut buffer, register update*:

+ [ ] cut buffer
+ [ ] swap file: see [UndoAndSwapFile.md]
  * [ ] On w/W, buffer is cleared and re-initialized with current cut buffer
  * [ ] if no I/O can be performed at all, keep everything in a giant stringstream
  * [ ] on clean exit, delete file

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

*Shell versions*:

+ [ ] r !sh
+ [ ] f !sh
+ [ ] e !sh
+ [ ] w !sh

Internals, Externals and Other Tasks
------------------------------------

+ [ ] refactor string processing
+ [x] scripting behaviour (cancel on error if !isatty)
+ [ ] test that e&q cowardly refuse to exit
+ [x] utf8 with BOM
+ [ ] command line argument for P
+ [ ] arg: `-s` (suppress diagnostics)
+ [ ] start with empty file
+ [ ] jaked --help
+ [ ] jaked --version
+ [ ] some nice html documentation, like `ed`'s man page
+ [ ] read windows-native UTF16 files (`rutf16 somefile?`)
