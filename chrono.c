#include <stdio.h>
#include <time.h>

char buf[4096];

int main(int argc, char* argv[])
{
    time_t t1, t2;
    char buf[4096], *p;
    int i;
    size_t n = 0;
    struct tm *tm;
    int hr;
    t1 = time(NULL);
    //memset(buf, 0, 4096);

    p = buf;
    for(i = 1; i < argc; ++i) {
        n = strlen(argv[i]);
        //fprintf(stderr, "n is now %zd\n", n);
        //fprintf(stderr, "p is now %zd\n", p - buf);
        strcpy(p, argv[i]);
        //fprintf(stderr, "copying %s\n", argv[i]);
        p[n] = ' ';
        p += n + 1;
        //fprintf(stderr, "p - buf = %zd\n", p - buf);
        //fprintf(stderr, "%s\n", buf);
    }
    p[n] = '\0';

    //fprintf(stderr, "Will execute %s\n", buf);
    hr = system(buf);

    t2 = time(NULL) - t1;

    strftime(buf, sizeof(buf), "Executed in %H:%M:%S\n", gmtime(&t2));
    fprintf(stderr, buf);

    return hr;
}
