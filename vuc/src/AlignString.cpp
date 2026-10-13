// Byte-exact VUC 2004.1 AlignString reconstruction.
// This standalone source avoids unrelated AsmOutput method-emission state.
#include <cstring>

char *AlignString(const char *text, int alignment)
{
    static char formatted[256];
    strcpy(formatted, text);
    char *space = strchr(formatted, ' ');
    if (space) {
        int offset = space - formatted;
        if (offset < alignment) {
            int i;
            for (i = 0; i < alignment - offset; ++i)
                space[i] = ' ';
            space[i] = 0;
            strcat(space, text + (space - formatted));
        }
    }
    return formatted;
}
