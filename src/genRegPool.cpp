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
    RegPoolEntry *e = pool->registerEntries;
    int *mp = &maximumAllocationParts;
    clearRawBytes(allocationRegisterMask, 32);
    for (int i = 0; i != pool->entryCount; i++, e++) {
        if (!e->enabledPhysicalRegister)
            allocationRegisterMask[i] = (1 << e->partCount) - 1;
        if (e->partCount > *mp)
            *mp = e->partCount;
    }
    return true;
}

int firstEmptyMask(RegPool *pool)
{
    for (int i = 0; i != pool->entryCount; i++)
        if (!allocationRegisterMask[i])
            return i;
    return -1;
}

bool assignExtRegisters(Program *p)
{
    if (!resetProgramAllocationsForFirstPass(p)) {
        reportError("failed to allocReset before ext assign\n");
        return false;
    }
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        unsigned int mode = (unsigned char)wt->allocationMode;
        if ((char)mode != 1) {
            const char *modeName = allocModeName(mode);
            reportError("AssignExtRegisters - alloc mode not NONE for %s mode %s!\n",
                          s->streamName(), modeName);
            return false;
        }
        for (unsigned i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && !rp->assignExt(s)) {
                reportError("flowMon::AssignExt registers failed for pool %s\n", rp->poolName);
                return false;
            }
        }
        for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
            for (int j = 0; j != in->operandCount; j++) {
                Operand *op = &in->operands[j];
                RegPoolEntry *reg = op->assignedRegister;
                if (reg && reg->isVirtual) {
                    regUse *ru = wt->registerUses.findByRegister(reg);
                    if (!ru) {
                        reportError("failed to find reg ref associated with reg\n");
                        return false;
                    }
                    if (ru->isExternal()) {
                        RegPoolEntry *ext = reg->assignedPhysicalRegister;
                        if (!ext) {
                            reportError("no ext mapping on register\n");
                            return false;
                        }
                        if (ext->isVirtual) {
                            reportError("alloc tmp is virtual in ext assign\n");
                            return false;
                        }
                        op->assignedRegister = ext;
                    }
                }
            }
        }
        wt->allocationMode = 2;
    }
    if (!resetProgramAllocationsForSecondPass(p)) {
        reportError("failed to clean up after global alloc\n");
        return false;
    }
    return true;
}

static inline char bitSet(int m, int i)
{
    return (m >> i) & 1;
}
