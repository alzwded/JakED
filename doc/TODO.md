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

+ [ ] Ni
+ [ ] Na
+ [ ] .,.c

*Line transfer*:

+ [ ] j
+ [ ] .,.m.
+ [ ] .,.t.

*Cut buffer*:

+ [ ] .,.d
+ [ ] .,.y
+ [ ] .x

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
