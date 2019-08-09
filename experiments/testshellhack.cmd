
for /l %%i in (0, 1, 3) do (
	echo.%%i
	time /T | testshell %%i
    IF ERRORLEVEL 1 EXIT /B 1
	time /T | testshell %%i > test.tmp && echo.test.tmp+++++ && type test.tmp && echo.test.tmp=====
    IF ERRORLEVEL 1 EXIT /B 1
)
echo.Don\'t forget to also test it interactively: testshell [0123]
