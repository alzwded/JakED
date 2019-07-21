Commands to implement
=====================

Navigation
----------

+ [x] N
+ [x] ,
+ [x] $
+ [x] .
+ [x] offsets
+ [x] +
+ [x] -
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
+ [ ] G//
+ [ ] v//
+ [ ] V//
+ [ ] s//
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

+ [ ] u
+ [ ] P
+ [ ] command line argument for P
+ [ ] other command line arguments
+ [ ] start with empty file
+ [x] H
+ [x] h
+ [x] .,.#

Commands
--------

+ [x] k

*Insert mode*:

+ [x] Ni
+ [x] test inserted text preserves registers correctly
+ [x] Na
+ [ ] .,.c
+ [ ] test registers get clobbered by c

*Line transfer, register update*:

+ [ ] j
+ [ ] .,.m.
+ [ ] move preserves registers
+ [ ] .,.t.

*Cut buffer, register update*:

+ [ ] .,.d
+ [ ] test registers get clobbered by d
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

Internals
---------

+ [ ] refactor string processing
+ [x] scripting behaviour (cancel on error if !isatty)
+ [ ] test that e&q cowardly refuse to exit
+ [x] utf8 with BOM
