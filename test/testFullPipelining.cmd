@SET jaked=jaked_debug

@SET ERRLVL=0
ECHO :help | cscript /NOLOGO test\cat.vbs | %jaked% | cscript /NOLOGO test\cat.vbs
IF ERRORLEVEL 1 EXIT /B 1
@ECHO repeat interactively:
@ECHO ECHO :help ^| cscript /NOLOGO test\cat.vbs ^| %jaked% ^| cscript /NOLOGO test\cat.vbs
@ECHO cscript /NOLOGO test\cat.vbs ^| %jaked% ^| cscript /NOLOGO test\cat.vbs

@EXIT /B 0
