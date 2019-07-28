Regular Expression Support
==========================

JakED supports POSIX BRE with the `\+` and `\?` extensions as short-cuts for `\{0,1\}` and `\{1,\}` respectively. It may support other extensions accidently as a result of the regex library implementation.

The following are known to work:

| Expression            | Matches                                       |
|-----------------------|-----------------------------------------------|
| `c`                   | The character matches itself                  |
| `\c`                  | The character matches itself unless otherwise noted |
| `.`                   | Any single character                          |
| `[class]`             | Basic character range support, e.g. [a-z], [ ] etc. |
| `[^class]`            | Any single character not in the class         |
| `^`                   | If it is the first character, matches Begining of line |
| `$`                   | If it is the first character, matches End of line |
| `\<`, `\>`            | Match word boundaries                         |
| `\(re\)`              | A subexpression or capture group              |
| `*`                   | Previous character or group may be repeated 0 or more times |
| `\{n,m\}`             | Previous character or group may be between n and m times |
| `\{n,\}`, `\{n\}`     | = n or more                                   |
| `\?`                  | = `\{0,1\}`                                   |
| `\+`                  | = `\{1,\}`                                    |

Regular expressions may be used as base ranges (offsets are supported), in the g, G, v, V commands and in the first part of the s command.

Replacements in the second part of `s///` are done with sed rules:

| Token                 | Produces                                      |
|-----------------------|-----------------------------------------------|
| `c`                   | The character matches itself                  |
| `\c`                  | The character matches itself unless otherwise noted |
| `&`                   | The regex match                               |
| `\N`                  | Where N is a digit. Replaces with the Nth capture group (`\(re\)`) |

The `s///` command may be followed by:

| Flag                  | Meaning                                       |
|-----------------------|-----------------------------------------------|
| `N`                   | Where N is a number. Executes the replacement only on the Nth match |
| `g`                   | Executes the replacement on all matches in a line. Without `g`, it only executes on the first match of a line |
| `p`                   | Print the pattern space (i.e. `&` to stdout when a replacement is made) |
