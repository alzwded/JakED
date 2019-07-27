# JakED

WIP: nonconformant clone of `man 1 ed`

See [TODO](doc/TODO.md) for current status.

## Files

| File              | Description                                       |
|-------------------|---------------------------------------------------|
| `jaked.cpp`       | Main code (parsers, commands, state, UI). Yeah, it should be refactored |
| `swapfile.{h,cpp}` | [Swapfile](doc/UndoAndSwapFile.md) implementation |
| `jaked_test.cpp`  | Internal test bed. This wraps `jaked.cpp` and the test suites, which are in include files to keep them slightly organized `test/*.ixx` |
| `cprintf.h`       | Header containing `cprintf`, which is a conditionally compiled `printf` based on tokens. This file is generated. |
| `cprintf_generate.c` | Generates `cprintf.h` based on tokens passed to the command line. Uses `cprintf_template.h` as a template. |
| `chrono.c`        | Quick re-implementation of UNIX `time`            |
| `test/*.ed`, `test/*.ref` | External test-files. The `.ed` scripts are the input and the `.ref` files are the reference file. |
| `makeutf8bom.c`   | generate a UTF8 file with a BOM to test that `jaked` *"correctly"* reads it as ASCII

## Tests

Currently, there are internal tests and external tests.

### Internal tests

`jaked_test.cpp` is the test bed for internal tests. It includes `jaked.cpp` and the test suites in `test/*.ixx`. I've opted to use the *include-a-c-file* technique to allow me to properly influence the behaviour of things.

For example, `application_exit` is handled as an exception when under test, because calling `exit(0)` is not nice.

Test suites and cases are defined with the provided macros.

Each suite is executed in its own process, and this process kills itself after a certain `Timeout`.

Cases within a suite are executed in order, so you could pass state on if you're into that kind of thing.

The test bad has its own sanity tests, because who tests the testers?

### External tests

External tests are currently of the [execute commands, write, compare file with reference](test/testWriteCommands.cmd). The inputs are in `test/*.ed` files and the refs in `test/*.ref` files.

### Static code analysis

The last stage of `nmake test` is MSVC's built-in static code analysis. This is mostly to check that `cprintf` and `printf` are invoked with correct parameters.

## Building

The only command you need is `nmake jaked.exe`. It's a stand-alone executable.

## License?

[Yes](LICENSE).
