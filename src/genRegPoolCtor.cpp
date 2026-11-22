#include "os.h"
#include "genRegPool_shared.h"

#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void clearRawBytes(char *p, int n);
RegPool *GetRP(unsigned t);
bool resetProgramAllocationsForFirstPass(Program *p);
bool resetProgramAllocationsForSecondPass(Program *p);
bool setRange(int start, int end, int lsz);
int countSetBits(unsigned int);
int setRangeFrom(workTable *t);


genRegPool::genRegPool()
{
    poolKind = 1;
}

genRegPool::~genRegPool()
{
}

bool genRegPool::assignRegistersForWorkTable(workTable *table)
{
    if (!table) {
        reportError("failed to get itable in nassign - which is not really likely\n");
        return false;
    }
    if (!resetPoolLocalAllocationState()) {
        reportError("failed to reset local allocation scheme for pool\n");
        return false;
    }
    setRangeFrom(table);
    if (!initAlloc(table->getRangeStart(), table->getRangeEnd())) {
        reportError("failed to set table range\n");
        return false;
    }
    regUse *registerUse = table->registerUses.firstReal();
    regUseList *list = &table->registerUses;
    for (; registerUse; registerUse = registerUse->nextReal()) {
        if (!set_in_aTab(registerUse)) {
            reportError("failed to set forced register in aTab\n");
            table->dump();
            list->dumpRegisterUses();
            exitAfterCleanup(-1);
            return false;
        }
    }
    RegPoolEntry *todo = 0;
    Instr *instruction;
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        Operand *operand;
        for (int operandIndex = 0; (operand = instruction->getOp(operandIndex)) != 0; operandIndex++) {
            RegPoolEntry *reg = operand->assignedRegister;
            if (ownsRegisterClass(reg) && reg->isVirtual && reg->assignmentCostTableSize < 0) {
                if (!build_tTab(reg, (1 << reg->partCount) - 1)) {
                    reportError("failed to build ttab while building todolist\n");
                    table->dump();
                    exitAfterCleanup(-1);
                }
                reg->assignmentCostTableSize = countTTab();
                reg->nextPendingAssignment = todo;
                todo = reg;
                if (reg->assignmentCostTableSize < 0) {
                    reportError("-ve ttabsize\n");
                    exitAfterCleanup(-1);
                }
            }
        }
    }
    while (todo) {
        int max = -1;
        RegPoolEntry *best = 0;
        RegPoolEntry *bestPrev = 0;
        RegPoolEntry *prev = 0;
        for (RegPoolEntry *registerEntry = todo; registerEntry; prev = registerEntry, registerEntry = registerEntry->nextPendingAssignment) {
            if (registerEntry->assignmentCostTableSize > max) {
                best = registerEntry;
                bestPrev = prev;
                max = registerEntry->assignmentCostTableSize;
            }
        }
        if (!best) {
            reportError("failed to find lowest register!\n");
            exitAfterCleanup(-1);
        }
        if (!build_tTab(best, (1 << best->partCount) - 1)) {
            reportError("failed to build tta externalb\n");
            exitAfterCleanup(-1);
        }
        int registerIndex = bestReg();
        if (registerIndex < 0) {
            if (!countMarkedPhysicalRegisters()) {
                reportError("vcl failed to allocate a register of type %s, but there's no registers of this type available for it to use.. maybe adding a .init_.. directive would help\n", poolName);
                suppressFurtherErrors();
                return false;
            } else
                list->dumpVirtualRegisterUses();
            return false;
        }
        best->assignedPhysicalRegister = &registerEntries[registerIndex];
        if (!set_t_in_aTab(registerIndex)) {
            registerUse->validateAndDumpParts();
            list->dumpRegisterUses();
            reportError("failed to set t\n");
            exitAfterCleanup(-1);
        }
        dumpATab();
        dumpTTab();
        if (bestPrev)
            bestPrev->nextPendingAssignment = best->nextPendingAssignment;
        else
            todo = best->nextPendingAssignment;
    }
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        Operand *operand;
        for (int virtualOperandIndex = 0; (operand = instruction->getOp(virtualOperandIndex)) != 0; virtualOperandIndex++) {
            RegPoolEntry *reg = operand->assignedRegister;
            if (ownsRegisterClass(reg) && reg->isVirtual) {
                if (!reg->assignedPhysicalRegister) {
                    reportError("notmapped!, should have been mapped by now..%s!\n",
                                  reg->registerName());
                    exitAfterCleanup(-1);
                }
                operand->assignedRegister = reg->assignedPhysicalRegister;
            }
        }
    }
    dumpATab();
    return true;
}

bool genRegPool::assignExt(instStream *stream)
{
    if (!stream) {
        reportError("AssignExternalRegisters - passed a null  pointer!\n");
        return false;
    }
    Program *owner = stream->owningProgram;
    if (!owner) {
        reportError("AssignExternalRegisters - stream had no owner\n");
        return false;
    }
    workTable *table = stream->getTable();
    for (regUse *registerUse = table->registerUses.firstVirtual(); registerUse; registerUse = registerUse->nextVirtual()) {
        RegPoolEntry *reg = registerUse->registerEntry;
        if (!reg) {
            reportError("flowMon::AssignExt: register not set in rf\n");
            return false;
        }
        if (ownsRegisterClass(reg) && registerUse->isExternal() && !reg->assignedPhysicalRegister) {
            initRegMask(this);
            for (instStream *currentStream = owner->streamHead; currentStream; currentStream = currentStream->nextStream) {
                if (!discardDisjointExternalRegister(currentStream, reg)) {
                    reportError("generic external allocator had internal error.. ret<0 for DiscardDisjoint\n");
                    return false;
                }
            }
            int registerIndex = firstEmptyMask(this);
            if (registerIndex < 0) {
                if (!countMarkedPhysicalRegisters()) {
                    reportError("vcl failed to assign a register of type %s, but there's no registers of that type available for it to use, maybe adding a .init.. directive would help\n");
                    suppressFurtherErrors();
                    return false;
                }
                reportInfo("currently at stream: %s trying to allocate external register %s\n",
                              stream->streamName(), reg->registerName());
                table->registerUses.dumpVirtualRegisterUses();
                reportError("failed to assign an external %s\n", reg->registerName());
                suppressFurtherErrors();
                return false;
            }
            reg->assignedPhysicalRegister = &registerEntries[registerIndex];
        }
    }
    return true;
}

bool genRegPool::discardDisjointExternalRegister(instStream *stream, RegPoolEntry *reg)
{
    if (!reg || !ownsRegisterClass(reg)) {
        reportError("discard disjoint called, but this pool doesn't own this register!\n");
        return false;
    }
    if (reg->assignedPhysicalRegister) {
        reportError("rg_a is mapped.. at start of DiscardDisjoint registers %s\n",
                      reg->registerName());
        return false;
    }
    workTable *table = stream->getTable();
    regUseList *registerUseList = &table->registerUses;
    regUse *registerUse;
    for (registerUse = registerUseList->firstVirtual(); registerUse; registerUse = registerUse->nextVirtual()) {
        if (registerUse->registerEntry == reg) {
            if (!registerUse->isExternal()) {
                reportError("discard disjoint - found a match - but it's not external\n");
                return false;
            }
            break;
        }
    }
    if (!registerUse)
        return true;
    if (!(registerUse->externalInputMask | registerUse->externalOutputMask | registerUse->throughputMask)) {
        reportError("genRegPool: discard disjoint - an xnf register had was not xixot\n");
        return false;
    }
    if (registerUse->externalInputMask & ~registerUse->inputMask) {
        reportError("xiBits is outside of iBits \n");
        return false;
    }
    if (registerUse->externalOutputMask & ~registerUse->outputMask) {
        reportError("xiBits is outside of iBits \n");
        return false;
    }
    for (regUse *otherRegisterUse = registerUseList->head; otherRegisterUse; otherRegisterUse = otherRegisterUse->next) {
        RegPoolEntry *registerEntry = otherRegisterUse->registerEntry;
        if (!registerEntry) {
            reportError("flowMon::discard disjoint forced regref with null reg\n");
            return false;
        }
        if (!ownsRegisterClass(registerEntry) || (!registerEntry->assignedPhysicalRegister && registerEntry->isVirtual))
            continue;
        unsigned char overlapMask = 0;
        switch (table->codeModeValue) {
        case 1:
        case 4: {
            unsigned char firstUsageMask = registerUse->inputMask | registerUse->outputMask;
            if (registerUse->outputMask)
                firstUsageMask |= 0x80;
            unsigned char secondUsageMask = otherRegisterUse->inputMask | otherRegisterUse->outputMask;
            if (otherRegisterUse->outputMask)
                secondUsageMask |= 0x80;
            overlapMask = secondUsageMask & firstUsageMask;
            overlapMask |= (firstUsageMask & otherRegisterUse->throughputMask) | (secondUsageMask & registerUse->throughputMask);
            break;
        }
        case 3:
            for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                int firstRangeStart = 0, firstRangeEnd = 0;
                bool firstAlwaysLive = false;
                int secondRangeStart = 0, secondRangeEnd = 0;
                bool secondAlwaysLive = false;
                if (!regRefLiveRange(registerUse, partIndex, &firstRangeStart, &firstRangeEnd, &firstAlwaysLive)) {
                    reportError("couldn't determine live range_a\n");
                    return false;
                }
                if (!regRefLiveRange(otherRegisterUse, partIndex, &secondRangeStart, &secondRangeEnd, &secondAlwaysLive)) {
                    reportError("couldn't determine live range_b.%d\n", partIndex);
                    otherRegisterUse->validateAndDumpParts();
                    table->dump();
                    return false;
                }
                if (!(firstAlwaysLive && secondAlwaysLive) && (secondRangeEnd >= 0 || firstRangeEnd >= 0) && firstRangeStart < secondRangeEnd && firstRangeEnd > secondRangeStart)
                    overlapMask |= 1 << partIndex;
            }
            if (registerUse->outputMask && otherRegisterUse->outputMask && hack_slo(table, reg, registerEntry))
                overlapMask |= 0x80;
            break;
        default:
            return false;
        }
        RegPoolEntry *tmpr;
        if (registerEntry->isVirtual != 1)
            tmpr = registerEntry;
        else
            tmpr = registerEntry->assignedPhysicalRegister;
        if (!tmpr) {
            reportError("tmpr is zero\n");
            return false;
        }
        if (tmpr->isVirtual) {
            reportError("tmpr is virtual\n");
            return false;
        }
        allocationRegisterMask[tmpr->regIndex()] |= overlapMask;
    }
    return true;
}
