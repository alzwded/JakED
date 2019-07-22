Undo and the Swap File
======================

Swap file format:

```
i32     cut_buffer_start
u32     cut_buffer_len
i32     undo_command_start
u32     undo_command_len
*       [uncommitted inserts]
...
[cut_buffer_start..+cut_buffer_len] text
...
[undo_command_start..+undo_command_len] script
```

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
