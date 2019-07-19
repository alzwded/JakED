jaked.exe: jaked.cpp
	cl /EHa /O2 jaked.cpp

jaked_debug.exe: jaked.cpp
	cl /EHa /Zi /DJAKED_DEBUG jaked.cpp /Fejaked_debug.exe

jaked_test.exe: jaked_test.cpp jaked.cpp
	cl /EHa jaked_test.cpp

test: jaked_test.exe
	jaked_test

all: jaked.exe test

clean:
	del /q /s *.o *.obj *.exe *.pdb *.ilk
