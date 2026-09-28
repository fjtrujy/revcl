#ifndef PROGRAM_H
#define PROGRAM_H

struct CpuDef;
class Parser;

// Flow object ("flowMon") living on main's stack. Name is a guess.
struct Program {
    Program(bool symbolic);
    ~Program();
    bool func_0806ea5c(CpuDef *cpu, const char *uniqueName, bool uniqueAll);
    bool func_0806efc4(Parser *parser, bool a, bool newSyntax);

    char unk_00[0x44];
    int unk_44;
    char unk_48[0x28];
    int unk_70;
    char unk_74[0x08];
    int unk_7c;
    char unk_80[0x70];
};

#endif
