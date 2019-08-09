testshell: testshell.exe
	testshellhack

testshell.exe: testshell.cpp ../shell.cpp ../shell.h ../common.h ../cprintf.h
	cl /std:c++17 /Zi /EHa testshell.cpp ../shell.cpp

remapper.exe: remapper.c
	cl /Zi remapper.c
