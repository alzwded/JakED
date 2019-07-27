Undo and the Swap File
======================

Swap file format:

```
Swapfile:
    i64     head
    u16     padding, always 0
    i64     cut
    i64     undo
    LE{*}   data

LE:
    i64     next
    u16     sz
    u8[sz]  text
```

In C format:

```C
    struct Line
    {
        struct Line* next;
        uint16_t sz;
        char[sz] text;
    };

    struct Header
    {
        struct Line head[1] = { &data[?], 0 };
        struct Line* cut;
        struct Line* undo;
        struct Line[?] data;
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
+ gc: this is called after the file is written. The goal is to reorder and compact the swap file. It effectively garbage collects disconnected lines via mark-and-sweep – so, obviously, try not to create loops.

The Line object supports the following operations:

+ `next`: returns the successor element, or nothing if it's the end
+ `link`: sets the successor element
+ `text`: retrieves the associated `text` field
+ `length`: retrieves the associated `sz` field

Lines are immutable because they are well packed in memory; memory management responsabilities are sort-of placed on the caller (even though you only have `new` and `gc`).

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
