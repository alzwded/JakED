JAKED_TEST_SANITY_CHECK=0

HEADERS = swapfile.h cprintf.h
SOURCES = jaked.cpp  swapfile.cpp

CPRINTF_KEYS=\
W \
r \
CTRLC \
CTRLC2 \
Q \
a \
d \
regs \
j \
parser \
error \
swap \
regex \
MandT \

# leave above line blank

jaked.exe: $(SOURCES) license.cpp
	cl /std:c++17 /EHa /O2 /Ob1 /Ox /Ot /MT $** /Fe:$@

jaked_debug.exe: $(SOURCES)
	cl /std:c++17 /EHa /Zi /DJAKED_DEBUG $** /Fe$@

jaked_test.exe: jaked_test.cpp $(SOURCES) test\*.ixx 
	cl /std:c++17 /EHa /Zi /DJAKED_TEST /DJAKED_TEST_SANITY_CHECK=$(JAKED_TEST_SANITY_CHECK) /bigobj jaked_test.cpp swapfile.cpp

run_all_tests: jaked_test.exe jaked_debug.exe jaked.exe
	jaked_test
	cmd /c test\testWriteCommands.cmd
	cmd /c test\testStdoutCommands.cmd
	nmake /NOLOGO sal

sal:
	cl /std:c++17 /analyze:only /analyze:autolog- /WX /Wall $(SOURCES)

test: chrono.exe
	chrono nmake /NOLOGO run_all_tests

all: jaked.exe test

cprintf_generate.exe: cprintf_generate.c
	cl.exe /O2 /Ot cprintf_generate.c

cprintf.h: cprintf_template.h cprintf_generate.exe Makefile
	cprintf_generate $(CPRINTF_KEYS)

chrono.exe: chrono.c
	cl /O2 /Ot chrono.c

$(SOURCES): $(HEADERS)

license.cpp: LICENSE Makefile
	echo.extern ^"C^" const char LICENSE[] = R^"(>license.cpp
    type LICENSE>>license.cpp
    echo.)^";>>license.cpp
	echo.#pragma comment(linker, ^"^/include:LICENSE^")>>license.cpp

clean:
	del /q /s *.o *.obj *.exe *.pdb *.ilk cprintf.h license.cpp
