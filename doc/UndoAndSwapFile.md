Undo and the Swap File
======================

Swap file format:

```
Swapfile:
    u64     head
    u16     padding, always 0
    u64     cut
    u64     undo
    LE{*}   data

LE:
    u64     next
    u16     sz
    u8[sz]  text
```

In C format:

```C
    struct Line
    {
        struct Line *next;
        uint16_t sz;
        char text[sz];
    };

    struct Header
    {
        struct Line head[1]; // assert(head[0].sz == 0);
        struct Line *cut;
        struct Line *undo;
        struct Line data[?];
    };
```

The Swapfile is made up primarily of `Line` structs, which make up
at least one and at most three linked lists. Those lists are the "TEXT",
"CUT" and "UNDO" lists.

The base of the memory space contains three special registers:

+ `head`: is the head of the "TEXT" list. It is represented as a `List` struct with a size of 0 (thus rendering its `text` field non-existant) to simplify the C++ code that manipulates lines (something needs to be appenable even when there is nothing there yet; so may as well make that something look like a `Line`)
+ `cut`: points to the "CUT" list containing the cut buffer; this list used to be part of "TEXT"
+ `undo`: points to a special line containing an undo command. The undo command optionally points to a list of lines that were at one point part of "TEXT". The special command line may point to the "CUT" list, thus they can overlap.

In C++ land, the Swapfile can perform a very small number of operations:

+ read the `head`, `cut` and `undo` registers, which return the head of their respective lists (if any in the case of the latter two)
+ set the `cut` and `undo` list heads
+ add a new, unlinked `Line` to the text
+ ~~gc: this is called after the file is written. The goal is to reorder and compact the swap file. It effectively garbage collects disconnected lines via mark-and-sweep – so, obviously, try not to create loops.~~

The Line object supports the following operations:

+ `next`: returns the successor element, or nothing if it's the end
+ `link`: sets the successor element
+ `text`: retrieves the associated `text` field
+ `length`: retrieves the associated `sz` field

Lines are immutable because they are well packed in memory; memory management responsabilities are sort-of placed on the caller (even though you only have `new` ~~and `gc`~~).

All text manipulation commands use the Swapfile as storage, thus all manipulation commands use the above operations to achieve their goals.

See an example in [`swapfile.h`](../swapfile.h).

Undo behaviour per command
--------------------------

Notations:

| Notation      | Notes                                           |
|---------------|-------------------------------------------------|
| `N`           | Range=N                                         |
| `N,M`         | Range=N,M                                       |
| `N,MmP`       | Range=N,M, destination=P                        |
| `TEXT`        | Inserted text (e.g. for `i`, `c`...)            |
| `LTXT`        | Length of `TEXT`                                |
| `OTXT`        | Deleted text (e.g. for `d`, `c`...)             |

| Command       | Undo instructions                               |
|---------------|-------------------------------------------------|
| `Ni`          | `N-1,N-1+LTXTd`                                 |
| `Na`          | `N,N+LTXTd`                                     |
| `N,McTEXT`    | `N,N+LTXTcOTXT`                                 |
| `N,Mj`        | `NcOTXT`                                        |
| `N,Md`        | `NiOTXT`                                        |
| `Nx`          | `N,N+LTXTd`                                     |
| `Nr`          | `N+1,N+1LTXTd`                                  |
| `e`, `E`      | `1,$cOTXT`                                      |
| `N,MmP`       | <span>`IF P>M P-(M-N+1)+1,PmN-1`<br>`IF P<N N-P,N-P+(M-N+1)mM`</span> |
| `N,MtP`       | `P+1,P+1+(M-N)d`                                |
| `N,Mg,G,v,V`  | `N,McOTXT`                                      |
| `N,Ms...`     | `N,McOTXT`                                      |
| `u`           | *Opposite of whatever command is to be executed* |


```

    [Loop]     [CommandProcessor]        [Swapfile]
       |             |                        |
()---->|--N,McPT--->||                        |
       |            ||----\                   |
       |            ||    | exec(c)           |
       |            ||<---/                   |
       |            ||----a(T)-------------->||           // if applicable
       |            ||<......................||
       |            ||                        |
       |            ||----cut(L)------------>||           // if applicable
       |            ||<......................||
       |            ||                        |
       |            ||----u(N',M'c'P'L)----->||
       |            ||<......................||
       |<...........||                        |
       |             |                        |
       |             |                        |
       |             |                        |
()---->|---u------->||                        |
       |            ||-------getUndo-------->||
       |            |||<-N',M'c'P'L----------||
       |            |||----\                 ||
       |            |||    | exec(c')        ||
       |            |||<---/                 ||
       |            |||----a(T)------------>|||           // if applicable
       |            |||<....................|||
       |            |||                      ||
       |            |||----cut(L')--------->|||           // if applicable
       |            |||<....................|||
       |            |||                      ||
       |            |||--u(N",M"c"P"T"L")-->|||
       |            |||<....................|||
       |            |||                      ||
       |            |||.....................>||
       |            ||<......................||
       |<...........||                        |
       |             |                        |
```

Legend:

- N,McPT: execute command `c` on range `[N,M]` with `tail=P` and inserted text `T`
- `N',M'c'P'`: command parameters derived from the table above, based on the input `N,McPT`
- `L`: lines of text removed from buffer as a result of the command (e.g. `d`)

When `u` is invoked, the `N',M'c'P'L` command gets executed recursively(1).

As a result, this command will generate a more derived set of instructions,
`N",M"c"P"L"` based on the table above, and store it in the swap file.

After a number of recursions (2) as a result of invoking the 'u' command
repeatedly, we end up in one of the following oscillating state:

- `c -> c -> c...`
- `d -> i -> d -> i...`
- `m -> m -> m...`

The down side is that `TEXT` gets rewritten each time until a save purges all
of them, but at least it's a functioning system.

The above diagram simplifies things a bit (or complicates them?) since it's
likely the Swapfile itself will not _act_, rather it will merely retrieve
the undo command for the command processor to go into a loop. But it was
easier to draw, and conceptually it kind-of works that way. Oh, and at the
time of writing, the CommandProcessor is basically the code inside the
Loop() function, so that needs to be refactored.

Stuff like the cut buffer should be tested with the in-memory implementation,
more to check that the command invoke the correct calls than anything else.

Some other rafactoring that needs to be done:

- don't create a giant string of `TEXT` or `OTXT` or the whole UNDO stack, rather process them as lines or in blocks. This applies to both a, cut, undo and getUndo
- the cut buffer may be re-used by the undo command (i.e. the cut buffer points *into* the undo buffer).

Testability
-----------

This thing should be tested externally with the real swap file. I.e. in the [last stage](../test/testWriteCommands.cmd)

Reasons:

+ it records & plays back existing commands (which should be well tested by the lower levels)
+ it interacts with the Swapfile, which is a deployment thing
+ it is something "you see", i.e. we'd be testing mostly glue code

It would be best if there were a separate test suite for `Swapfile/FileImpl`

s/// algorithm
--------------

This command has a bit of a more complicated algorithm because it tries to stay stable in case of interrupts or regex errors. So, at each step, it tries to perform the replacement, and if it succeds, swaps out the old line for the new line and links predecessors and successors accordingly. The old line is then pushed to an undo list that has a temporary head because at this point we have no idea how many lines we will end up touching, so we cannot initialize the real undo command line.

```
Let FILE have 6 lines.
We will perform a 3,5s/re/fm/ which will affect all lines 3 and 4 and error out on the 5th
1
2
3   -> 3'
4   -> 4'
5   -> !
6

  0 GS <- g_state                           ; alias
 10 ex <- NUL                               ; hold any raised exception
 20 GS.line <- 3                            ; set line to working line
 30 if ParseRegex(tail).line != GS.line     ; initialize regex to avoid recompiling
        throw
 35 fmt <- ParseReplaceFormat(tail)         ; cache format since it's the same
 40 uh <- swapfile.line("")                 ; undo head (stub; 10 byte overhead)
 50 up <- uh->Copy()                        ; undo working pointer
 60 it <- 2                                 ; it points to the second line
 70 loop
 80     ax <- it->next()                    ; set accumulator to current line
 90     cx <- ax->text()                    ; grab text in another accumulator
100     if ParseRegex(//).line != GS.line   ; if line doesn't match regex
110         set ex = "Line doesn't match"   ; simulate exception 
120     cx <- s(cx, GS.regexp)              ; perform replace and keep string
            ! catch(ex)                     ; catch exceptions
130     ax <- swapfile.line(cx)             ; add a new line
140     ax->link(it->next()->next())        ; link new line to the
                                            ; current line's successor
150     it->next()->link()                  ; unlink current line
160     up->link(it->next())                ; link undo pointer to current line
170     it->link(ax)                        ; link previous line to new line
    
180     it <- it->next()                    ; move it to new line, whose successor
                                            ; is the next line in the original text
190     up <- up->next()                    ; move undo pointer to the deleted line
    
200     if ex                               ; if exceptions were set,
            ! break                         ; break

205     GS.line ++                          ; increment work line
    until GS.line > 5                       ; continue until all lines were processed
    
210 ul <- swapfile.line(3 << "," << line << "c") ; initialize undo command
                                            ; to replace all affected lines
220 swapfile.undo(ul)                       ; set undo register
230 ul->link(uh->next())                    ; link undo register to the first
                                            ; real line in our undo list
                                            ; (the head was a 10 byte stub)
    
240 if ex                                   ; if an exception was set,
        ! throw ex                          ; reraise
```

g/// undo support
-----------------

This command has a more involved undo algorithm:

1. it creates a disconnected phantom list; let's call it the GLOB list; its text is `1,$c`;
2. it iterates over the whole current state of the TEXT list; for each line, it creates an indirect handle (`Swapfile::line(LinePtr->ref())`) in order to save the current line order
3. it then goes through [its normal behaviour](Global.md).
4. it sets the UNDO head to the GLOB head

On undo, normal stuff happens. You would expect to end up with gibberish, and you would be right if the present never got to that point in the future where the author implemented indirect handles and undo support.

Actually, let's clarify some more how undo will work in the multilevel undo case: The `c` command is bullshit. It actually goes through the GLOB list and relinks the pointed-to line to be the next pointed-to line. After undo completes, the UNDS (undo stack) list head is popped and UNDO set to the next element. That's going to be interesting to implement.

During `command-list`, `u` is initially set to NUL, but it is available inside one execution for undo marks set by the commands therein. At the end, the undo mark for the whole `g//` command is set. I hope my multi-level undo list will work this way. Maybe indirect handles can help.

What does an undo buffer look like?
-----------------------------------

*Disclaimer: I've implemented this much later and I was too lazy to re-read the whole document*

The global undo header points to a `U` line reference whose `next` pointer points to the undo command line `Uc`; the `sz` is `-1` (magic value for references; in which case the text is actually a 64bit pointer) and `text` points to the previous undo `U1` in the stack. Next, `Uc` has `text[sz]` equal to a command that would undo whatever was done previously (e.g. for a `g//`, it is a `c`, for a `d` it is a `i`, for `i` it is a `d`); `Uc`'s `next` points to `0` if it needs no additional input, else it points to normal text lines which would be needed as input to the command.

So let's say we had issued a couple of change commands:

```
0a
a
.
1c
bsd
qwe
zxc
.
1s/b/a/
```

The swap file would look something like thus: (actual text lines omitted)

```
head.undo -> U2
...
U1' next = 0   sz = 1  text = 'a'
U1c next = U1' sz = 4  text = '1,3c'
U1  next = U1c sz = -1 text = 0x00000000
...
U2' next = 0   sz = 1  text = 'bsd'
U2c next = U2' sz = 4  text = '1,1c'
U2  next = U2c sz = -1 text = <addrof U1>
```

*2nd disclaimer: I don't remember why I implemented line references, but it's good that I did*
