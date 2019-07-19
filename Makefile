jaked.exe: jaked.cpp
	cl /EHa /O2 jaked.cpp

jaked_test.exe: jaked_test.cpp jaked.cpp
	cl /EHa jaked_test.cpp

test: jaked_test.exe
	jaked_test

all: jaked.exe test
