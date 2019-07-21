#include <stdio.h>

int main()
{
    FILE* f;
    f = fopen("test\\utf8bom.txt", "w");
    fprintf(f, "%s\n", "\xEF" "\xBB" "\xBF" "Line 1");
    fprintf(f, "%s\n", "Line 2");
    fclose(f);
}
