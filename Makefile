JAKED_TEST_SANITY_CHECK=0

jaked.exe: jaked.cpp
	cl /EHa /O2 /Ob1 /Ox /Ot /MT jaked.cpp

jaked_debug.exe: jaked.cpp
	cl /EHa /Zi /DJAKED_DEBUG jaked.cpp /Fejaked_debug.exe

jaked_test.exe: jaked_test.cpp jaked.cpp test\*.ixx
	cl /EHa /Zi /DJAKED_TEST /DJAKED_TEST_SANITY_CHECK=$(JAKED_TEST_SANITY_CHECK) jaked_test.cpp

test: jaked_test.exe jaked_debug.exe
	jaked_test
	cmd /c test\testWriteCommands.cmd

all: jaked.exe test

clean:
	del /q /s *.o *.obj *.exe *.pdb *.ilk
