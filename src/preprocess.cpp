// Translation unit of the preprocessor driver (CIE group 1).

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "os.h"

int func_0804ad84(const char *fmt, ...);
char *func_0804a6e0(int tool, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

const char *matchedDirective;

static inline char lowerCase(char c)
{
    if (c >= 'A' && c <= 'Z')
        c += 'a' - 'A';
    return c;
}

static inline bool startsWith(const char *s, const char *prefix)
{
    const char *p = prefix;
    while (*p && lowerCase(*p) == lowerCase(*s)) {
        s++;
        p++;
    }
    if (*p == 0)
        matchedDirective = prefix;
    return *p == 0;
}

bool func_0804a410(const char *buf)
{
    if (!buf) {
        func_0804ad84("Input was a null pointer?\n");
        return false;
    }
    const char *p = buf;
    bool inComment = false;
    for (; *p; p++) {
        if (*p == ';')
            inComment = true;
        if (*p == '\n')
            inComment = false;
        if (*p == '\r')
            inComment = false;
        if (!inComment) {
            if (startsWith(p, ".mac") || startsWith(p, ".inc")) {
                func_0804ad84("The input seems to include directives that should be processed by gasp, not vcl ('%s...')\n", matchedDirective);
                func_0804ad84("Try adding '-g' to the command line\n");
                return false;
            }
            if (startsWith(p, "#def") || startsWith(p, "#inc")) {
                func_0804ad84("The input seems to include directives that should be processed by gcc, not vcl ('%s..')\n", matchedDirective);
                func_0804ad84("Try adding '-G' to the command line\n");
                return false;
            }
        }
    }
    return true;
}
