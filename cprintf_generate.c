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
        if(strcmp(buf, ">>>GUARDS<<<\n") == 0) {
            p = lhead;
            while(p != ltail) {
                fprintf(g,
#if 0
                        "#ifndef CPRINTF_%s\n"
                        "# define __CPRINTF_%s 0\n"
                        "#else\n"
                        "# define __CPRINTF_%s 1\n"
                        "#endif\n",
                        *p, *p, *p);
#else
                        "#define __CPRINTF_%s 0\n",
                        *p);
#endif
                ++p;
            }
            continue;
        }
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
                        "    __CPRINTF_%s,\n",
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
