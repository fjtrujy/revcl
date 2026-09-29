// Instruction stream (owner of instructions); layout mostly unknown.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

#include "os.h"

class workTable;
struct regRef;

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

    bool func_0805e4bc();         // gather stats
    bool func_0806563c(bool a, bool b);  // build memory dependencies
    bool func_0806d5cc();         // flow reset
    unsigned char func_0806d908(); // code mode
    const char *func_0806d934();  // code mode name

    char unk_00[0x108];
    instStream *unk_108;         // next in Program::unk_a0
    char unk_10c[0x218 - 0x10c];
    workTable *unk_218;  // work table
    char unk_21c[0x234 - 0x21c];
    regRef *unk_234;
    char unk_238[4];
    int unk_23c;                // dst key
    int unk_240;
    int unk_244;
    int unk_248;
    int unk_24c;
    instStream *unk_250;        // next in Program::unk_dc
    char unk_254[4];
    instStream *unk_258;        // next in Program::unk_e0
    instStream *unk_25c;        // next in Program::unk_e4
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
