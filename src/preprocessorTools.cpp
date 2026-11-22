// Translation unit of the preprocessor driver (CIE group 1).

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "os.h"

int reportUserMessage(const char *fmt, ...);
bool symbolic_080b300c = false;
bool flag_080b300d = false;
bool noLoops_080b300e = false;
int timeout_080b3010 = 4;
bool unk_080b3014 = false;
bool sizeReduce_080b3015 = true;
bool noAlign_080b3016 = false;
bool noEbit_080b3017 = false;
bool flag_080b3018 = true;
Tool tools_080b301c[3] = {
    { "gcc", "gcc -P -E -C -x assembler-with-cpp" },
    { "gasp", "gasp -p -s -c ';' " },
    { "gasp", "gasp -p -a -s -c ';' " },
};
unsigned char toolMask_080b3034 = 7;
char toolOpt_080b3035 = 'I';
char *runSinglePreprocessor(int tool, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

bool usesGaspPreprocessor(unsigned char flags)
{
    return (flags & 6) != 0;
}

bool usesGccPreprocessor(unsigned char flags)
{
    return flags & 1;
}

static inline bool toolOptMatches(unsigned char mask, char opt)
{
    unsigned int bits = mask;
    if (toolMask_080b3034 & bits) {
        if (toolOpt_080b3035 == opt)
            return true;
    }
    return false;
}

bool matchesToolOption(unsigned char mask, char opt)
{
    unsigned int bits = mask;
    if (toolMask_080b3034 & bits) {
        if (toolOpt_080b3035 == opt)
            return true;
    }
    return false;
}

void enableGccPreprocessing(unsigned char *flags)
{
    *flags |= 1;
}

void enableGaspPreprocessing(unsigned char *flags)
{
    *flags |= 2;
}

void enableAlternateGaspPreprocessing(unsigned char *flags)
{
    *flags |= 4;
}

char *runSelectedPreprocessors(unsigned char flags, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps)
{
    for (int i = 0; i != 3; i++) {
        if ((flags >> i) & 1) {
            buf = runSinglePreprocessor(i, buf, len, argFirst, argEnd, argv, keepTemps);
            if (!buf)
                return 0;
        }
    }
    return buf;
}

char *runSinglePreprocessor(int tool, char *buf, int *len, int argFirst, int argEnd,
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

    reportInfo("..processing with %s\n", t->name);
    inPath[0] = 0;
    outPath[0] = 0;

    if (!buf) {
        reportError("null data passed to doGasp\n");
    } else {
        int n = strlen(buf);
        if (n == 0) {
            reportError("in buffer is null length\n");
        } else {
            sprintf(inName, "vcli_%s", t->name);
            fdIn = func_0804ab6c(inName, inPath);
            if (fdIn < 0) {
                reportError("failed to make temp in file\n");
            } else if (write(fdIn, buf, n) != n) {
                reportError("failed to write data to temp file\n");
            } else {
                close(fdIn);
                *len = 0;
                fdIn = -1;
                delete[] buf;
                buf = 0;
                sprintf(outName, "vclo_%s", t->name);
                fdOut = func_0804ab6c(outName, outPath);
                if (fdOut < 0) {
                    reportError("failed to make tmp outfile\n");
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
                        reportError("system returned %d\n", rc);
                    } else {
                        result = func_0804aef0(outPath, len, true);
                        if (!result)
                            reportError("failed to read tmp out file\n");
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
            reportWarning("failed to unlink temporary gasp input file %s\n", inPath);
        if (outPath[0] && unlink(outPath))
            reportWarning("failed to unlink temporary gasp output file %s\n", outPath);
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
