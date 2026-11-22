// Translation unit of the preprocessor driver (CIE group 1).

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "os.h"

int reportUserMessage(const char *fmt, ...);
char *runSinglePreprocessor(int tool, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

const char *matchedDirective;

static inline char lowerCase(char character)
{
    if (character >= 'A' && character <= 'Z')
        character += 'a' - 'A';
    return character;
}

static inline bool startsWith(const char *text, const char *prefix)
{
    const char *prefixCursor = prefix;
    while (*prefixCursor && lowerCase(*prefixCursor) == lowerCase(*text)) {
        text++;
        prefixCursor++;
    }
    if (*prefixCursor == 0)
        matchedDirective = prefix;
    return *prefixCursor == 0;
}

bool warnAboutMissingPreprocessing(const char *buf)
{
    if (!buf) {
        reportUserMessage("Input was a null pointer?\n");
        return false;
    }
    const char *inputCursor = buf;
    bool inComment = false;
    for (; *inputCursor; inputCursor++) {
        if (*inputCursor == ';')
            inComment = true;
        if (*inputCursor == '\n')
            inComment = false;
        if (*inputCursor == '\r')
            inComment = false;
        if (!inComment) {
            if (startsWith(inputCursor, ".mac") || startsWith(inputCursor, ".inc")) {
                reportUserMessage("The input seems to include directives that should be processed by gasp, not vcl ('%s...')\n", matchedDirective);
                reportUserMessage("Try adding '-g' to the command line\n");
                return false;
            }
            if (startsWith(inputCursor, "#def") || startsWith(inputCursor, "#inc")) {
                reportUserMessage("The input seems to include directives that should be processed by gcc, not vcl ('%s..')\n", matchedDirective);
                reportUserMessage("Try adding '-G' to the command line\n");
                return false;
            }
        }
    }
    return true;
}
