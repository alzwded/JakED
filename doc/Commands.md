Commands
========

The general format of commands is:

```
    [Range]C[command specific]
```

All commands are a single character.

At the end of each command, the current line (or dot, `.`) is moved to the last affected line.

If `Range` is not specified, then the command's default range is used.

See [Ranges](Ranges.md).

| Command | Default range | Arguments & Notes                             |
|---------|---------------|-----------------------------------------------|
| `P`     | *N/A*         | Toggles printing a prompt when expecting user input |
| `p`     | `.,.`         | Prints the lines in the specified range. If `p` is followed by `n`, then line numbers are printed as well. |
| `n`     | `.,.`         | Prints the lines in the specified range. Each line is prepended by a number, then a tab, then the text. |
| `e`     | *N/A*         | Expects a file name. If the first character of the file name is an `!`, see [Shell](Shell.md). Edits the file or the output of the shell command. This command complains if the buffer is dirty. Destroys the buffer if it isn't dirty. If no file name is specified, it edits the file previously marked with the `f` command.
| `E`     | *N/A*         | Same as `e`, but doesn't complain. Caution, destroys the buffer without prompt. |
| `r`     | `$`           | Expects a file name. If the first character of the file name if an `!`, see [Shell](Shell.md). Reads the file or the output of the shell command and appends it at the end of the specified range. |
| `q`     | *N/A*         | Exits the program if the buffer is not dirty, otherwise it complains. |
| `Q`     | *N/A*         | Exits the program without prompt. |
| `h`     | *N/A*         | Prints out the last diagnostic message. Diagnostic messages are available when a command encounters an error, and prints out a `?`. `h` prints more information, basically. |
| `H`     | *N/A*         | Acts as an implicit `h` invoked after every `?` |
| `#`     | *ignored*     | This command does nothing. You can use it for comments |
| `k`     | `.`           | Expects to be followed by a small case latin letter (`[a-z]`). Marks the line at the end of the specified range with that letter. The line can later be retrieved with the `'` command.
| `=`     | `$`           | Prints out the line number at the end of the specified range.
| `f`     | *N/A*         | If it is followed by a file name, it sets the default file name used by the `e`, `E`, `w`, `W` and [shell escapes](Shell.md). If it is not followed by anything, it prints out the default file name.
| `w`     | `1,$`         | Writes the specified range. If it is followed by a file name, it writes to that file. If the first character of the file name is an `!`, it writes the range to the standard input of the specified [shell](Shell.md) command. If it is not followed by anything, it assumes the file name set by `f`
| `W`     | `1,$`         | Like `w`, but appends instead of overwriting.
| `z`     | `.+1`         | Scrolls the screen starting from the next line. If it is followed by a number, it scrolls by that many lines. If it is not followed by anything, it uses N from a previous invocation. N is initially 1.
| `i`     | `.`           | Inserts text before the end of the specified range. To end input mode, enter a single `.` on a line.
| `a`     | `.`           | Inserts text at the end of the specified range. To end input mode, enter a single `.` on a line. The range may be line 0.
| `c`     | `.,.`         | Change the specified range. The range is deleted with `d`, and then `i` is invoked to allow you to enter new text.
| `d`     | `.,.`         | Deletes the specified range. The lines are placed in the cut buffer.
| `j`     | `.,.+1`       | Joins the specified range, concatenating the lines. It uses the string immediately following it to separate the lines.
| `s`     | `.,.`         | Text substitution. See [Regexp](Regexp.md)
| `x`     | `.`           | Inserts text from the cut buffer after the specified range
| `y`     | `.,.`         | Yanks the specified range into the cut buffer.
| `l`     | `.,.`         | Lists the specified range unambiguously, by spelling out non-printable characters and appending a `$` at the end of the line.
| `m`     | `.,.`         | Expects to be followed by another range specifying the destination. Moves the specified range to after the destination range. The destinatino range may be line 0.
| `t`     | `.,.`         | Transfers text. Expects to be followed by another range specifying the destination. Copies the text from the specified range to after the destination range. The destination range may be line 0.
| `g`     | `1,$`         | See [Global](Global.md)
| `G`     | `1,$`         | See [Global](Global.md)
| `v`     | `1,$`         | See [Global](Global.md)
| `V`     | `1,$`         | See [Global](Global.md)
| *NUL*   | `.+1`         | Prints out the last line of the specified range.
| `u`     | *N/A*         | Undoes the previous action.
| `!`     | *it's complicated* | Executes a shell command with the text immediately following it. If it is immediately followed by another `!`, it repeats the last non-repeat shell command. After the invoked command exits, it prints out a single `!`. If a range is specified, it filters the lines in the specified range using the shell command. See [Shell](Shell.md)
| `:`     | *N/A*         | Extended commands. See builtin `:help`. Initially added to support `:crlf` and `:lf` to support different line endings
