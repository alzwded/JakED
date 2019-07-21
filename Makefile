JAKED_TEST_SANITY_CHECK=0

HEADERS = swapfile.h
SOURCES = jaked.cpp  swapfile.cpp

jaked.exe: $(SOURCES)
	cl /EHa /O2 /Ob1 /Ox /Ot /MT $** /Fe:$@

jaked_debug.exe: $(SOURCES)
	cl /EHa /Zi /DJAKED_DEBUG $** /Fe$@

jaked_test.exe: jaked_test.cpp $(SOURCES) test\*.ixx
	cl /EHa /Zi /DJAKED_TEST /DJAKED_TEST_SANITY_CHECK=$(JAKED_TEST_SANITY_CHECK) jaked_test.cpp swapfile.cpp

test: jaked_test.exe jaked_debug.exe
	jaked_test
	cmd /c test\testWriteCommands.cmd

all: jaked.exe test

clean:
	del /q /s *.o *.obj *.exe *.pdb *.ilk
