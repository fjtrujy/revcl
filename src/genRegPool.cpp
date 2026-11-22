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

extern int maxParts_080bce20;
extern int nRegs_080bce24;
extern int tStart_080bce28;
extern int tEnd_080bce2c;
extern char aTab_080bce40[0x20000];
extern char tTab_080dce40[0x1000];
extern char regMask_080dde40[32];

void clearRegMask()
{
    clearRawBytes(regMask_080dde40, 32);
}

bool initRegMask(RegPool *pool)
{
    RegPoolEntry *e = pool->registerEntries;
    int *mp = &maxParts_080bce20;
    clearRawBytes(regMask_080dde40, 32);
    for (int i = 0; i != pool->entryCount; i++, e++) {
        if (!e->unk_16)
            regMask_080dde40[i] = (1 << e->partCount) - 1;
        if (e->partCount > *mp)
            *mp = e->partCount;
    }
    return true;
}

int firstEmptyMask(RegPool *pool)
{
    for (int i = 0; i != pool->entryCount; i++)
        if (!regMask_080dde40[i])
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
