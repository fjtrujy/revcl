#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void func_0804c220(char *p, int n);
RegPool *GetRP(unsigned t);
bool func_0806f610(Program *p);
bool func_0806f6a4(Program *p);
bool setRange(int start, int end, int lsz);
int func_0804bf50(unsigned int);
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
    func_0804c220(regMask_080dde40, 32);
}

bool initRegMask(RegPool *pool)
{
    RegPoolEntry *e = pool->unk_04;
    int *mp = &maxParts_080bce20;
    func_0804c220(regMask_080dde40, 32);
    for (int i = 0; i != pool->unk_08; i++, e++) {
        if (!e->unk_16)
            regMask_080dde40[i] = (1 << e->unk_28) - 1;
        if (e->unk_28 > *mp)
            *mp = e->unk_28;
    }
    return true;
}

int firstEmptyMask(RegPool *pool)
{
    for (int i = 0; i != pool->unk_08; i++)
        if (!regMask_080dde40[i])
            return i;
    return -1;
}

bool assignExtRegisters(Program *p)
{
    if (!func_0806f610(p)) {
        func_0804ad38("failed to allocReset before ext assign\n");
        return false;
    }
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        unsigned int mode = (unsigned char)wt->unk_08;
        if ((char)mode != 1) {
            const char *modeName = allocModeName(mode);
            func_0804ad38("AssignExtRegisters - alloc mode not NONE for %s mode %s!\n",
                          s->func_0806d674(), modeName);
            return false;
        }
        for (unsigned i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && !rp->assignExt(s)) {
                func_0804ad38("flowMon::AssignExt registers failed for pool %s\n", rp->unk_2c);
                return false;
            }
        }
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            for (int j = 0; j != in->unk_138b; j++) {
                Operand *op = &in->unk_38[j];
                RegPoolEntry *reg = op->unk_18;
                if (reg && reg->unk_14) {
                    regUse *ru = wt->unk_14.func_0805b5dc(reg);
                    if (!ru) {
                        func_0804ad38("failed to find reg ref associated with reg\n");
                        return false;
                    }
                    if (ru->isExternal()) {
                        RegPoolEntry *ext = reg->unk_18;
                        if (!ext) {
                            func_0804ad38("no ext mapping on register\n");
                            return false;
                        }
                        if (ext->unk_14) {
                            func_0804ad38("alloc tmp is virtual in ext assign\n");
                            return false;
                        }
                        op->unk_18 = ext;
                    }
                }
            }
        }
        wt->unk_08 = 2;
    }
    if (!func_0806f6a4(p)) {
        func_0804ad38("failed to clean up after global alloc\n");
        return false;
    }
    return true;
}

static inline char bitSet(int m, int i)
{
    return (m >> i) & 1;
}
