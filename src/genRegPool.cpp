#include "os.h"
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

extern int maximumAllocationParts;
extern int allocationRegisterCount;
extern int allocationTimelineStart;
extern int allocationTimelineEnd;
extern char allocationRegisterTimeline[0x20000];
extern char candidateRegisterTimeline[0x1000];
extern char allocationRegisterMask[32];

void clearRegMask()
{
    clearRawBytes(allocationRegisterMask, 32);
}

bool initRegMask(RegPool *pool)
{
    RegPoolEntry *registerEntry = pool->registerEntries;
    int *mp = &maximumAllocationParts;
    clearRawBytes(allocationRegisterMask, 32);
    for (int entryIndex = 0; entryIndex != pool->entryCount; entryIndex++, registerEntry++) {
        if (!registerEntry->enabledPhysicalRegister)
            allocationRegisterMask[entryIndex] = (1 << registerEntry->partCount) - 1;
        if (registerEntry->partCount > *mp)
            *mp = registerEntry->partCount;
    }
    return true;
}

int firstEmptyMask(RegPool *pool)
{
    for (int entryIndex = 0; entryIndex != pool->entryCount; entryIndex++)
        if (!allocationRegisterMask[entryIndex])
            return entryIndex;
    return -1;
}

bool assignExtRegisters(Program *program)
{
    if (!resetProgramAllocationsForFirstPass(program)) {
        reportError("failed to allocReset before ext assign\n");
        return false;
    }
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *scheduleTable = stream->getTable();
        unsigned int mode = (unsigned char)scheduleTable->allocationMode;
        if ((char)mode != 1) {
            const char *modeName = allocModeName(mode);
            reportError("AssignExtRegisters - alloc mode not NONE for %s mode %s!\n",
                          stream->streamName(), modeName);
            return false;
        }
        for (unsigned i = 0; i != 12; i++) {
            RegPool *registerPool = GetRP(i);
            if (registerPool && !registerPool->assignExt(stream)) {
                reportError("flowMon::AssignExt registers failed for pool %s\n", registerPool->poolName);
                return false;
            }
        }
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                RegPoolEntry *reg = operand->assignedRegister;
                if (reg && reg->isVirtual) {
                    regUse *registerUse = scheduleTable->registerUses.findByRegister(reg);
                    if (!registerUse) {
                        reportError("failed to find reg ref associated with reg\n");
                        return false;
                    }
                    if (registerUse->isExternal()) {
                        RegPoolEntry *ext = reg->assignedPhysicalRegister;
                        if (!ext) {
                            reportError("no ext mapping on register\n");
                            return false;
                        }
                        if (ext->isVirtual) {
                            reportError("alloc tmp is virtual in ext assign\n");
                            return false;
                        }
                        operand->assignedRegister = ext;
                    }
                }
            }
        }
        scheduleTable->allocationMode = 2;
    }
    if (!resetProgramAllocationsForSecondPass(program)) {
        reportError("failed to clean up after global alloc\n");
        return false;
    }
    return true;
}

static inline char bitSet(int m, int i)
{
    return (m >> i) & 1;
}
