The global command
==================

```yacc
g_command: g regex command-list
         | G regex
         ;

g: "[gv]"
 ;

G: "[GV]"
 ;

regex: /* see doc/Regexp.md */
     ;

command-list: command
            | command-list '\\' '\n' command
            ;

command: range c tail
       ;

range: /* see doc/Ranges.md */
     ;

tail: ".*"
    ;

command: /* any supported command except another g, G, v, V; see doc/Commands.md */
       ;
```

Behaviour
---------

**g/re/command-list**

Does a full sweep of the whole file and marks all matching lines.

Then, iterates over each marked line that still exist, sets `.` to point to that line, and applies `command-list`. If any command fails, the `command-list` is interrupted, and `g` will continue with the next marked line.

At the end, the current line is set by the last executed command.

Errors are hidden.

**G/re/**

Does a full sweep of the whole file and marks all matching lines.

Then, iterates over each marked line that still exists, echoes the current line, and pauses for user input. The user input is:

+ an empty line, which executes no command; or
+ a single `&`, which executes the previous `command-list`; or
+ a `command-list`.

Errors are hidden. If any command fails, the `command-list` is interrupted and `G` continues with the next marked line. CTRL-C stops the command.

**v/re/command-list**

Same as `g/re/`, but executed on lines that **do not** match `/re/`. Re**v**erse.

**V/re/**

Same as `G/re/`, but executed on lines that **do not** match `/re/`.
