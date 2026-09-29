// Instruction stream (owner of instructions); layout mostly unknown.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

#include "os.h"

class workTable;

class instStream {
public:
    const char *func_0806d674();  // stream name or default
    workTable *getWorkTable()
    {
        if (unk_218)
            return unk_218;
        func_0804ad38("work table not set in %s\n", func_0806d674());
        func_0804adf0(-1);
        return unk_218;
    }

    char unk_00[0x218];
    workTable *unk_218;  // work table
    char unk_21c[0x260 - 0x21c];
    char unk_260;      // set when stream takes part in a stall
    char unk_261[0x5f];
    short unk_2c0;     // stream id
    char unk_2c2[0x2e0 - 0x2c2];
    int unk_2e0;       // dependency length
    int unk_2e4;

    int func_0805f284();
    int func_0805f29c();
    int getOrGuessDepLength();
    int func_0805f354();
    int getOrGuessLogicalLength();
};

#endif
