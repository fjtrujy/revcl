// Translation unit of the preprocessor driver (CIE group 1).

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "os.h"

int reportUserMessage(const char *fmt, ...);
bool symbolicOutputRequested = false;
bool cautiousMemoryAccessMode = false;
bool disableLoopProcessing = false;
int optimizationTimeoutSeconds = 4;
bool unk_080b3014 = false;
bool enableCodeSizeReduction = true;
bool disableOutputAlignment = false;
bool disableEndBits = false;
bool enableInstructionCollapse = true;
Tool preprocessorToolTable[3] = {
    { "gcc", "gcc -P -E -C -x assembler-with-cpp" },
    { "gasp", "gasp -p -s -c ';' " },
    { "gasp", "gasp -p -a -s -c ';' " },
};
unsigned char enabledPreprocessorTools = 7;
char toolOptionSwitch = 'I';
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
    if (enabledPreprocessorTools & bits) {
        if (toolOptionSwitch == opt)
            return true;
    }
    return false;
}

bool matchesToolOption(unsigned char mask, char opt)
{
    unsigned int bits = mask;
    if (enabledPreprocessorTools & bits) {
        if (toolOptionSwitch == opt)
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
    for (int preprocessorIndex = 0; preprocessorIndex != 3; preprocessorIndex++) {
        if ((flags >> preprocessorIndex) & 1) {
            buf = runSinglePreprocessor(preprocessorIndex, buf, len, argFirst, argEnd, argv, keepTemps);
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
    bool preprocessingSucceeded = false;
    Tool *preprocessorTool = &preprocessorToolTable[tool];

    reportInfo("..processing with %s\n", preprocessorTool->name);
    inPath[0] = 0;
    outPath[0] = 0;

    if (!buf) {
        reportError("null data passed to doGasp\n");
    } else {
        int inputLength = strlen(buf);
        if (inputLength == 0) {
            reportError("in buffer is null length\n");
        } else {
            sprintf(inName, "vcli_%s", preprocessorTool->name);
            fdIn = createTemporaryFile(inName, inPath);
            if (fdIn < 0) {
                reportError("failed to make temp in file\n");
            } else if (write(fdIn, buf, inputLength) != inputLength) {
                reportError("failed to write data to temp file\n");
            } else {
                close(fdIn);
                *len = 0;
                fdIn = -1;
                delete[] buf;
                buf = 0;
                sprintf(outName, "vclo_%s", preprocessorTool->name);
                fdOut = createTemporaryFile(outName, outPath);
                if (fdOut < 0) {
                    reportError("failed to make tmp outfile\n");
                } else {
                    close(fdOut);
                    fdOut = -1;
                    char *commandCursor = cmd + sprintf(cmd, "%s ", preprocessorTool->cmd);
                    for (int argumentIndex = argFirst; argumentIndex < argEnd; argumentIndex++)
                        if (toolOptMatches(1 << tool, argv[argumentIndex][1]))
                            commandCursor += sprintf(commandCursor, "%s ", argv[argumentIndex]);
                    sprintf(commandCursor, "%s > %s\n", inPath, outPath);
                    int processStatus = system(cmd);
                    if (processStatus) {
                        reportError("system returned %d\n", processStatus);
                    } else {
                        result = readFileIntoBuffer(outPath, len, true);
                        if (!result)
                            reportError("failed to read tmp out file\n");
                        preprocessingSucceeded = true;
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
    if (!preprocessingSucceeded) {
        if (result)
            delete[] result;
        result = 0;
        if (buf)
            delete[] buf;
        *len = 0;
    }
    return result;
}
