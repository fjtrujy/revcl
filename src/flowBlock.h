#ifndef FLOWBLOCK_H
#define FLOWBLOCK_H

#include "mList.h"

class Parser;
class Instr;
class instStream;
struct Program;

// Decoded source line entry built from the parse (sizeof 0x28). Name is a guess.
struct flowBlock {
    char *unk_00;
    char unk_04;            // referenced by a branch
    char unk_05;            // entry kind
    char unk_06;
    short unk_08;
    short unk_0a;
    Instr *unk_0c[2];
    mList unk_14;           // label names

    void func_08084220();
    void func_08084268(int index);
    void func_080842f0();
    bool func_080843d0();
};

void func_080842b4(flowBlock *blocks, int count);
flowBlock *func_0808435c(flowBlock *blocks, int count, const char *name);
int func_080843a0(flowBlock *blocks, int count);
flowBlock *func_08083768(Program *p, Parser *parser, int a, bool newSyntax, int *count);
bool splitParserStream(Program *p, flowBlock *blocks, int count);
bool buildPassThru(Program *prog, instStream *s, flowBlock *blocks, int count);
bool buildRaw(Program *prog, instStream *s, flowBlock *blocks, int count, int loopMode);

#endif
