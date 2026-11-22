// Register webs. The class is not under #pragma interface (its inline ctor/dtor are
// not emitted out of line); deferred inlines live in RegWeb_inline.h.
#ifndef REGWEB_H
#define REGWEB_H

#include "os.h"
#include "Instr.h"
#include "workTable.h"

class RegWeb {
public:
    RegPoolEntry *assignedRegister;
    RegPoolEntry *virtualRegister;
    RegPoolEntry *forcedRegister;
    RegWeb *parentWeb;
    unsigned short memberCount;
    short webIndex;
    unsigned char componentMask;
    char unk_15[3];

    RegWeb() {}
    ~RegWeb() {}

    static bool collectInstructionRegisterWebs(Instr *instruction);
    static RegWeb *createRegisterWeb(RegPoolEntry *registerEntry);
    static bool merge(RegWeb *firstWeb, RegWeb *secondWeb);
    static bool setForcedRegisterMapping(RegWeb *registerWeb, RegPoolEntry *forcedRegister);
    static bool collectTableRegisterWebs(workTable *scheduleTable);
    static bool applyRegisterWebToOperand(Operand *operand);
    static bool applyRegisterWebsToInstruction(Instr *instruction);
    static bool applyRegisterWebsToTable(workTable *scheduleTable);
    static void checkRegisterWebInvariants();
    static bool clearTableRegisterWebs(workTable *scheduleTable);
};

extern RegWeb regWebs[0x4000];
extern int regWebUsed;
extern int regWebCount;

bool resetRegisterWebs();
bool assignRegisterWebs();

#endif
