0.9.6
=====

* always flush stdout after a line; there were issues doing `cat | jaked | cat`,
  on the output side
* parse extended commands

0.9.5
=====

* improved online help

0.9.4
=====

* fix issue with file lengths going negative when deleting lines in an empty
  file (#7)
* support CRLF vs LF line endings when writing files (#8)
* added extended commands via `:`, e.g. `:crlf`, `:help`
* `=` now prints 0 for an empty buffer instead of erroring out with invalid 
  range

0.9.2 -> 0.9.3
==============

* implement infinite levels of undo

0.9.1 -> 0.9.2
==============

* allow `\/` in `s//`, like `s/aa/a\/a/` and it does what you think it does

0.9.0 -> 0.9.1
==============

* implement undo for filtering (`N,M!sh`)
* fix filtering only part of the file and the shell command not outputing anything
* fix `nmake clean` deleting the dist build
* added CHANGELOG
* launched jaked with a shell command doesn't persist the command as the default filename
