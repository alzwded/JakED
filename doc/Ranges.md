Ranges
======

| Format        | Effect                                                |
|---------------|-------------------------------------------------------|
| `.`           | The current line. The current line is updated by many commands |
| *void*        | Default, specific to each command                     |
| `N`           | Line number N                                         |
| `N,M`         | From line N to line M                                 |
| `$`           | The last line                                         |
| `,`           | as-if `1,$`                                           |
| `%`           | Implies `1,$`                                         |
| `N,`          | Equivalent to `N,$`                                   |
| `;`           | Equivalent to `.,`                                    |
| `L+N`         | Nth line after location L                             |
| `L-N`         | Nth line before location L                            |
| `L1-N,L2+M`   | From the Nth line before L1 through the Mth line after L2 |
| `+`           | Next line; may be repeated                            |
| `-`           | Previous line; may be repeated                        |
| `+N`          | Implies `.+N`                                         |
| `/re/`        | Next line that match regular expression. May loop around. See [Regexp](Regexp.md) |
| `'r`          | Location of a previously marked line (with `k`)       |

Yacc form (which I'll probably get wrong):

```yacc
%type pair<number, number>
range: /* void */           /* specific to each command; usually . */
     | srange             { $$.first = $1.first ; $$.second = $1.second }
     | ';'                { $$.first = . ; $$.second = $ }
     | ';' srange         { $$.first = . ; $$.second = $2.second }
     | ','                { $$.first = 1 ; $$.second = $ }
     | ',' srange         { $$.first = 1 ; $$second = $2.second }
     | "%"                { $$.first = 1 ; $$.second = $ }
     ;

srange: location           { $$.first = $$.second = $1 }
      | location ',' srange { $$.first = $1 ; $$.second = $3.second } /* yes, right recursion */
     ; 

location: /*void*/              
        | something-with-offset
        ;

something-with-offset: something
                     | something sign number
                     | something sign-repeated
                     ;

sign: '+'
    | '-'
    | '^'       /* means '-' */
    ;

sign-repeated: pluses
             | minuses
             | carrets
             ;

pluses: '+'
      | pluses '+'
      ;

minuses: '-'
       | minuses '-'
       ;

carrets: '^'
       | carrets '^'
       ;

something: /* void */           /* implies dot; BUT, the higher-level */
                                /* command parses matches a void range to */
                                /* the command's default, which may vary; */
                                /* if you string together many ranges, */
                                /* the void is implied to be '.' */
         | number
         | '.'
         | '$'
         | regexp               /* see Regexp.md */
         | register
         ;

number: "[0-9]\+"
      ;

register: "'[a-z]"
        ;
```
