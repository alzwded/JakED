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
+ [ ] .,.pn
+ [ ] .,.n
+ [ ] zN
+ [x] =
+ [x] <CR>

Misc
----

+ [ ] u
+ [ ] P
+ [ ] command line argument for P
+ [ ] H
+ [x] h
+ [x] .,.#

Commands
--------

+ [x] k
+ [ ] Ni
+ [ ] Na
+ [ ] .,.c
+ [ ] .,.d
+ [ ] j
+ [ ] .,.m.
+ [ ] .,.t.
+ [ ] .,.y
+ [ ] .x

I/O
---

+ [x] r [F]
+ [ ] r !sh
+ [x] e [F]
+ [x] E [F]
+ [x] f FILE
+ [ ] .,.w
+ [ ] .,.W
+ [ ] f !sh
+ [ ] e !sh
+ [ ] w !sh
+ [x] q
+ [x] Q
+ [ ] ^V literal input
+ [ ] handle `CTRL_C` and `CTRL_BRK` elegantly

Internals
---------

+ [ ] refactor string processing
+ [x] scripting behaviour (cancel on error if !isatty)
+ [ ] test that e&q cowardly refuse to exit
