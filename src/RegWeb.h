// Register webs. The class is not under #pragma interface (its inline ctor/dtor are
// not emitted out of line); deferred inlines live in RegWeb_inline.h.
#ifndef REGWEB_H
#define REGWEB_H

#include "os.h"
#include "Instr.h"
#include "workTable.h"

class RegWeb {
public:
    RegisterEntry *assignedRegister;
    RegisterEntry *virtualRegister;
    RegisterEntry *forcedRegister;
    RegWeb *parentWeb;
    unsigned short memberCount;
    short webIndex;
    unsigned char componentMask;
    char reservedRegisterWebTailBytes[3];

    RegWeb() {}
    ~RegWeb() {}

    static bool collectInstructionRegisterWebs(Instr *instruction);
    static RegWeb *createRegisterWeb(RegisterEntry *registerEntry);
    static bool merge(RegWeb *firstWeb, RegWeb *secondWeb);
    static bool setForcedRegisterMapping(RegWeb *registerWeb, RegisterEntry *forcedRegister);
    static bool collectTableRegisterWebs(ScheduleTable *scheduleTable);
    static bool applyRegisterWebToOperand(Operand *operand);
    static bool applyRegisterWebsToInstruction(Instr *instruction);
    static bool applyRegisterWebsToTable(ScheduleTable *scheduleTable);
    static void checkRegisterWebInvariants();
    static bool clearTableRegisterWebs(ScheduleTable *scheduleTable);
};

extern RegWeb regWebs[0x4000];
extern int regWebUsed;
extern int regWebCount;

bool resetRegisterWebs();
bool assignRegisterWebs();

#endif
