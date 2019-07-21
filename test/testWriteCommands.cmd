@SET jaked=jaked_debug

@FOR %%i in (test\*.ed) DO @(
    @CALL :COMPARISON %%i
    @IF ERRORLEVEL 1 EXIT /B 1
)

@ECHO OK.
@EXIT /B 0

:COMPARISON
    TYPE %1 | %jaked% test\tenlines.txt
    @IF ERRORLEVEL 1 EXIT /B 1
    @COMP test.tmp %~dpn1.ref /A /M
    @IF ERRORLEVEL 1 EXIT /B 1
    @DEL /Q test.tmp
    @EXIT /B 0
