// File reading and terminal error handling (separate original translation unit).

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "os.h"

extern char *currentOutputFileName;

void abortAfterError()
{
    if (currentOutputFileName)
        printf("ERROR: something went wrong, making '%s' vcl's outta here.\n", currentOutputFileName);
    else
        puts("ERROR: something went wrong, vcl's outta here.");
    OS_End();
    exit(-1);
}

char *readFileIntoBuffer(const char *path, int *len, bool nulTerm)
{
    bool readSucceeded = false;
    char *buf = 0;
    int size = -1;
    int fileDescriptor = open(path, O_RDONLY);

    if (fileDescriptor == -1) {
        printf("cgReadFile: failed to open file '%s'\n", path);
    } else {
        size = lseek(fileDescriptor, 0, SEEK_END);
        lseek(fileDescriptor, 0, SEEK_SET);
        int allocSize = size;
        if (nulTerm)
            allocSize = size + 1;
        buf = (char *)malloc(allocSize);
        if (buf) {
            if (read(fileDescriptor, buf, size) == size)
                readSucceeded = true;
            else
                printf("cgReadFile: failed to read all of file %s\n", path);
        }
        if (nulTerm)
            buf[size] = 0;
    }
    if (fileDescriptor != -1)
        close(fileDescriptor);
    if (!readSucceeded) {
        size = -1;
        if (buf)
            free(buf);
        buf = 0;
    }
    if (len)
        *len = size;
    return buf;
}

char *readDescriptorIntoBuffer(int fileDescriptor, int *len, bool nulTerm)
{
    int readSucceeded = 0;
    int used = 0;
    char *buf = (char *)malloc(0x10000);

    if (!buf) {
        reportError("failed to alloc initial read buffer\n");
    } else {
        int size = 0x10000;
        for (;;) {
            if (size - used < 0x10000) {
                size += 0x10000;
                char *nbuf = (char *)realloc(buf, size);
                if (!nbuf) {
                    reportError("nReadFile - failed to extend buffer\n");
                    break;
                }
                buf = nbuf;
            }
            int bytesRead = read(fileDescriptor, buf + used, 0xffff);
            if (bytesRead < 0) {
                reportError("read returned <0\n");
                break;
            }
            if (bytesRead == 0) {
                readSucceeded = 1;
                break;
            }
            used += bytesRead;
        }
        if (nulTerm && buf)
            buf[used] = 0;
    }
    if (!readSucceeded && buf) {
        free(buf);
        buf = 0;
        used = 0;
    }
    if (fileDescriptor > 0)
        close(fileDescriptor);
    if (len)
        *len = used;
    return buf;
}
