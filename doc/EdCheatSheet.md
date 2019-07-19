Navigation
==========

    N         go to line N
    ,         = 1,$
    $         last line
    .         current line
    +N        nth next line
    -N        nth previous line
    +         = +1; can be repeated
    -         = -1; can be repeated
    ;         = .,
    /RE/      next line matching RE
    ?RE?      previous line matching RE
    //  ??    repeat last search
    'r        refer to line marked as r;
              = vim's 'r

**N.b.**: addresses may be followed by one or more address offsets

Regexp
======

    g/RE/cmd  execute cmd for all lines matching RE;
              default is p
    G/RE/cmd  interagtive g/RE/
    v//, V//  reverse of g//, G//
    s/RE/RE/  replace; / can be anything;
              suffixed: g, COUNT, l, n, p
    s/RE/RE/N replace, but only Nth match
    regexps:  [:alnum:], [a-z], ^, $, 
              \(.*\), \{N,M\}, \<, \>, etc.
    extended: \` (begining), \' (ending), 
              \?, \+, \b, \B, \w, \W

**N.b.**: extended pattern tokens are only in GNU ed

Display
=======

    [N,N]pn   print range w/ line numbers 
    [N,N]l    list (like vim's set list)
    [N+1]zN   scroll by N lines 
              (quick view)
    [$]=      prints line number of 
              addressed line w/o jumping
    <CR>      scroll by one line; = +1p

Misc
====

    u         undo (like old vi); 
              g,G,v,V are one command
    P         turn prompt on
    H         auto-display diagnostics
    h         print diagnostic 
              for last failed command
    [N,N]#    comment

Commands
========

    [ADDRESS[,ADDRESS]]COMMAND[PARAMETERS]     general format

`ADDRESS` can be (almost) anything from #Navigation

    kr        mark line as register r;
              = vim's mr
    [N]i      insert [before N].
              End command with .
    [N]a      append [after N].
              End command with .
    [N,N]c    change range; end with .
    [N,N]d    delete range
    N,N+1j    join lines
    [N,N]m[N] move range after line;
              0 is a valid destination
    [N,N]s    repeat last substitution
    [N,N]t[N] transfer range after dest’n
    [N,N]m[N] move range after destination
    [N,N]y    yank range to cut buffer;
              c,d,j,s,y put into cut buffer
    [N]x      paste from cut buffer


I/O
===

    [r ]!sh   read output of shell command
    [$]r [F]  read F into buffer.
              0 is a valid destination.
    e FILE    edit file
    E FILE    edit unconditionally
    f FILE    set default file name
              for commands taking FILE
    [N,N]w F  write range to FILE;
              default is ,
    [N,N]W F  append range to FILE
    q         quit ed
    Q         quit ed really hard
