// Translation unit of the preprocessor driver (CIE group 1).

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "os.h"

int func_0804ad84(const char *fmt, ...);
char *func_0804a6e0(int tool, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

extern const char *matchedDirective;
extern unsigned char toolMask_080b3034;
extern char toolOpt_080b3035;

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

bool func_0804a608(unsigned char flags)
{
    return (flags & 6) != 0;
}

bool func_0804a61c(unsigned char flags)
{
    return flags & 1;
}

static inline bool toolOptMatches(unsigned char mask, char opt)
{
    if (mask & toolMask_080b3034) {
        if (toolOpt_080b3035 == opt)
            return true;
    }
    return false;
}

bool func_0804a62c(unsigned char mask, char opt)
{
    if (mask & toolMask_080b3034) {
        if (toolOpt_080b3035 == opt)
            return true;
    }
    return false;
}

void func_0804a658(unsigned char *flags)
{
    *flags |= 1;
}

void func_0804a664(unsigned char *flags)
{
    *flags |= 2;
}

void func_0804a670(unsigned char *flags)
{
    *flags |= 4;
}

char *func_0804a67c(unsigned char flags, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps)
{
    for (int i = 0; i != 3; i++) {
        if ((flags >> i) & 1) {
            buf = func_0804a6e0(i, buf, len, argFirst, argEnd, argv, keepTemps);
            if (!buf)
                return 0;
        }
    }
    return buf;
}

char *func_0804a6e0(int tool, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps)
{
    char cmd[1024];
    char inPath[1024];
    char outPath[1024];
    char inName[256];
    char outName[256];
    char *result = 0;
    int fdIn = -1;
    int fdOut = -1;
    bool ok = false;
    Tool *t = &tools_080b301c[tool];

    func_0804acb0("..processing with %s\n", tools_080b301c[tool].name);
    inPath[0] = 0;
    outPath[0] = 0;

    if (!buf) {
        func_0804ad38("null data passed to doGasp\n");
    } else {
        int n = strlen(buf);
        if (n == 0) {
            func_0804ad38("in buffer is null length\n");
        } else {
            sprintf(inName, "vcli_%s", tools_080b301c[tool].name);
            fdIn = func_0804ab6c(inName, inPath);
            if (fdIn < 0) {
                func_0804ad38("failed to make temp in file\n");
            } else if (write(fdIn, buf, n) != n) {
                func_0804ad38("failed to write data to temp file\n");
            } else {
                close(fdIn);
                *len = 0;
                fdIn = -1;
                delete[] buf;
                buf = 0;
                sprintf(outName, "vclo_%s", tools_080b301c[tool].name);
                fdOut = func_0804ab6c(outName, outPath);
                if (fdOut < 0) {
                    func_0804ad38("failed to make tmp outfile\n");
                } else {
                    close(fdOut);
                    fdOut = -1;
                    char *p = cmd + sprintf(cmd, "%s ", t->cmd);
                    for (int i = argFirst; i < argEnd; i++)
                        if (toolOptMatches(1 << tool, argv[i][1]))
                            p += sprintf(p, "%s ", argv[i]);
                    sprintf(p, "%s > %s\n", inPath, outPath);
                    int rc = system(cmd);
                    if (rc) {
                        func_0804ad38("system returned %d\n", rc);
                    } else {
                        result = func_0804aef0(outPath, len, true);
                        if (!result)
                            func_0804ad38("failed to read tmp out file\n");
                        ok = true;
                    }
                }
            }
        }
    }

    if (fdIn > 0)
        close(fdIn);
    if (fdOut > 0)
        close(fdOut);
    if (!keepTemps) {
        if (inPath[0] && unlink(inPath))
            func_0804ac7c("failed to unlink temporary gasp input file %s\n", inPath);
        if (outPath[0] && unlink(outPath))
            func_0804ac7c("failed to unlink temporary gasp output file %s\n", outPath);
    }
    if (!ok) {
        if (result)
            delete[] result;
        result = 0;
        if (buf)
            delete[] buf;
        *len = 0;
    }
    return result;
}
