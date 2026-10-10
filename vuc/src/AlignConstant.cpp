// Experimental VUC constant alignment and 0x->H' syntax conversion.
#include <cstring>
extern int constantsTabStop;
extern char *AlignString(const char *, int);

char *AlignConstant(const char *text)
{
    char *aligned = AlignString(text, constantsTabStop);
    char *marker = strstr(aligned, "0x");
    if (marker) {
        marker[0] = 'H';
        marker[1] = '\'';
    }
    return aligned;
}
