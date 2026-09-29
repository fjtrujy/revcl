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

    bool func_0806c9b8(const char *name);

    bool unk_00;        // new syntax
    char unk_01[3];
    mList unk_04;
    char *unk_18;       // global name (dupString)
    char *unk_1c;       // "Nameless"
    mList unk_20;
    char unk_34[9];
    char unk_3d;        // mode: 1 normal, 3 raw (VSM)
    char unk_3e;        // 3 reorder, 4 noreorder
    bool unk_3f;        // inside .rawloop
    short unk_40;       // rawloop count
    char unk_42[2];
    int unk_44;         // mpg target address, -1 = unset
    int unk_48;
    mList unk_4c;
    char unk_60;
    char unk_61;        // saved unk_3e
    char unk_62;        // saved unk_3d
    bool unk_63;
    char unk_64[0x0c];
    int unk_70;
    char unk_74[0x08];
    int unk_7c;
    char unk_80[0x70];
};

#endif
