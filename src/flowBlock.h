#ifndef FLOWBLOCK_H
#define FLOWBLOCK_H

#include "mList.h"

class Parser;
struct Program;

// Decoded source line entry built from the parse (sizeof 0x28). Name is a guess.
struct flowBlock {
    char unk_00[0x14];
    mList unk_14;
    void func_080842f0();
};

flowBlock *func_08083768(Program *p, Parser *parser, int a, bool newSyntax, int *count);
bool func_08085aa8(Program *p, flowBlock *blocks, int count, bool newSyntax);

#endif
