// Experimental VUC AlignString original static buffer and spacing logic.
#include <cstring>

char *AlignString(const char *text, int alignment)
{
    static char formatted[256];
    strcpy(formatted, text);
    char *space = strchr(formatted, ' ');
    if (space) {
        int offset = space - formatted;
        if (offset < alignment) {
            int padding = alignment - offset;
            for (int i = 0; i < padding; ++i)
                space[i] = ' ';
            space[padding] = 0;
            strcat(space, text + offset);
        }
    }
    return formatted;
}
