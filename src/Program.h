#ifndef PROGRAM_H
#define PROGRAM_H

#include "mList.h"

struct CpuDef;
class Parser;

// Flow object ("flowMon") living on main's stack. Name is a guess.
struct Program {
    Program(bool symbolic);
    ~Program();
    bool func_0806ea5c(CpuDef *cpu, const char *uniqueName, bool uniqueAll);
    bool func_0806efc4(Parser *parser, bool a, bool newSyntax);

    char unk_00[4];
    mList unk_04;
    char *unk_18;       // global name (dupString)
    char *unk_1c;       // "Nameless"
    mList unk_20;
    char unk_34[0x10];
    int unk_44;         // mpg target address, -1 = unset
    int unk_48;
    mList unk_4c;
    char unk_60[3];
    bool unk_63;
    char unk_64[0x0c];
    int unk_70;
    char unk_74[0x08];
    int unk_7c;
    char unk_80[0x70];
};

#endif
