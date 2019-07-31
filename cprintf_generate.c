/*
Copyright 2019 Vlad Meșco

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int die(const char message[])
{
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
    abort();
}

int main(int argc, char* argv[])
{
    char buf[256];
    FILE *f = NULL, *g = NULL;
    char** lhead = NULL;
    char** ltail = NULL;
    char** p = NULL;
    int read;

    lhead = argv + 1;
    ltail = argv + argc;

    f = fopen("cprintf_template.h", "r");
    if(!f) die("Cannot open cprintf_template.h");

    g = fopen("cprintf.h", "w");
    if(!g) die("Cannot open cprintf.h for writing");

    // skip error line
    fgets(buf, 256, f);
    // write out warning
    fprintf(g, "/***********************************************************\n"
               " * WARNING: this file is generated, but you can edit the   *\n"
               " *       default values of the __CPRINTF_KEY defails.      *\n"
               " ***********************************************************/\n");
    while(!feof(f)) {
        read = !!fgets(buf, 256, f);
        if(!read) continue;
        if(strcmp(buf, ">>>KEYS<<<\n") == 0) {
            p = lhead;
            while(p != ltail) {
                fprintf(g,
                        "    %s,\n",
                        *p);
                ++p;
            }
            continue;
        }
        if(strcmp(buf, ">>>MACROS<<<\n") == 0) {
            p = lhead;
            while(p != ltail) {
                fprintf(g,
                        "    0 /* %s */,\n",
                        *p);
                ++p;
            }
            continue;
        }
        if(strcmp(buf, ">>>TRANSLATIONS<<<\n") == 0) {
            p = lhead;
            while(p != ltail) {
                fprintf(g,
                        "    \"%s\",\n",
                        *p);
                ++p;
            }
            continue;
        }
        fprintf(g, "%s", buf);
    }

    fclose(g);
    fclose(f);

    return 0;
}
