#include "Program.h"
#include "instStream.h"
#include "genRegPool.h"

void func_0804c220(char *p, int n);
RegPool *GetRP(unsigned t);
bool func_0806f610(Program *p);
bool func_0806f6a4(Program *p);

int maxParts_080bce20;
char regMask_080dde40[32];

void clearRegMask()
{
    func_0804c220(regMask_080dde40, 32);
}

bool initRegMask(RegPool *pool)
{
    RegPoolEntry *e = pool->unk_04;
    func_0804c220(regMask_080dde40, 32);
    for (int i = 0; i != pool->unk_08; i++, e++) {
        if (!e->unk_16)
            regMask_080dde40[i] = (1 << e->unk_28) - 1;
        if (e->unk_28 > maxParts_080bce20)
            maxParts_080bce20 = e->unk_28;
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
        if (mode != 1) {
            func_0804ad38("AssignExtRegisters - alloc mode not NONE for %s mode %s!\n",
                          s->func_0806d674(), allocModeName(mode));
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

bool regRefLiveRange(regUse *ru, int part, int *s, int *e, bool *always)
{
    regUseSlot *sl = &ru->unk_20[part];
    *s = -4;
    *e = -3;
    *always = false;
    if (bitSet(ru->unk_0e, part)) {
        if (sl->unk_3c[0] < 0 || (unsigned)sl->unk_3c[1] > 1 ||
            sl->unk_3c[3] < 0 || (unsigned)sl->unk_3c[4] > 1) {
            func_0804ad38("firstInput<0.. but iBit set\n");
            func_0804adf0(-1);
        }
    }
    if (bitSet(ru->unk_0f, part)) {
        if (sl->unk_3c[5] < 0 || (unsigned)sl->unk_3c[6] > 1 ||
            sl->unk_3c[8] < 0 || (unsigned)sl->unk_3c[9] > 1) {
            func_0804ad38("firstInput<0.. but iBit set\n");
            func_0804adf0(-1);
        }
    }
    if (bitSet(~(ru->unk_0f | ru->unk_0e) & ru->unk_10, part)) {
        *s = -1;
        *always = true;
        *e = 30000;
        return true;
    }
    if (bitSet(ru->unk_0c, part)) {
        *s = -1;
    } else {
        if (bitSet(ru->unk_0e, part) && sl->unk_3c[0] <= sl->unk_3c[5]) {
            func_0804ad38("first input before first output.. but not xi %d,%d!\n",
                          sl->unk_3c[0], sl->unk_3c[5]);
            func_0804adf0(-1);
        }
        if (bitSet(ru->unk_0f, part))
            *s = sl->unk_3c[5];
    }
    if (bitSet(ru->unk_10 | ru->unk_0d, part)) {
        *e = 30000;
    } else {
        if (bitSet(ru->unk_0f, part))
            *e = sl->unk_3c[8] + 1;
        if (bitSet(ru->unk_0e, part) && sl->unk_3c[3] > *e)
            *e = sl->unk_3c[3];
    }
    if (*e <= *s) {
        func_0804ad38("liverange for regref has e<=s .. %d,%d?\n", *s, *e);
        return false;
    }
    return true;
}

static inline bool isInternalLock(cls0805bcc4 *lk)
{
    return lk->unk_2d == 2;
}

static inline bool isInternalIn(cls0805bcc4 *lk)
{
    return lk->unk_19 == 2;
}

bool lockLiveRange(cls0805bcc4 *in, cls0805bcc4 *out, int *s, int *e)
{
    *s = -1;
    *e = -1;
    if (!isInternalLock(out)) {
        Instr *ins = out->ownerInstr();
        if (!ins) {
            func_0804ad38("failed to get instruction for non -internal input\n");
            return false;
        }
        cls08058718 *rl = &ins->unk_13e8;
        short line = rl->unk_00;
        if (line != out->unk_32) {
            func_0804ad38("rLine mismatch\n");
            return false;
        }
        if ((unsigned)rl->unk_02 > 1) {
            func_0804ad38("bad pipe for instruction\n");
            return false;
        }
        *s = line;
    } else {
        *s = -1;
    }
    if (out->unk_16) {
        if (g_080b336c && *s >= 0)
            *e = *s + g_080b336c - *s % g_080b336c;
        else
            *e = 30000;
    } else {
        *e = -1;
    }
    for (cls0805bcc4 *c = out->unk_0c; c; c = c->unk_08) {
        if (isInternalIn(c))
            continue;
        Instr *ci = c->ownerInstr();
        if (!ci) {
            func_0804ad38("no inst for rl\n");
            return false;
        }
        cls08058718 *rl = &ci->unk_13e8;
        short line = rl->unk_00;
        if (line != c->unk_32) {
            func_0804ad38("rline mismatch for input locks\n");
            return false;
        }
        if ((unsigned)rl->unk_02 > 1) {
            func_0804ad38("bad pipe for instruction\n");
            return false;
        }
        if (line <= *s) {
            func_0804ad38("el<=s, but can't age\n");
            return false;
        }
        if (line > *e)
            *e = line;
    }
    if (*e < 0)
        *e = *s + 1;
    return true;
}

bool hack_slo(workTable *wt, RegPoolEntry *a, RegPoolEntry *b)
{
    if (wt->unk_09 != 3) {
        func_0804ad38("hack_slo - called - but table not raw\n");
        return false;
    }
    if (!wt->unk_2c) {
        func_0804ad38("hack_slo - called - but table ptr is null\n");
        return false;
    }
    for (int r = wt->unk_30; r < wt->unk_34; r++) {
        wtRow *row = &wt->unk_2c[r];
        bool hasA = false;
        bool hasB = false;
        for (int j = 0; j != 2; j++) {
            Instr *in = row->unk_00[j].unk_00;
            if (!in)
                continue;
            for (int i = 0; i != in->unk_138b; i++) {
                Operand *op = &in->unk_38[i];
                if (op->isReg2()) {
                    if (op->unk_18 == a)
                        hasA = true;
                    if (op->unk_18 == b)
                        hasB = true;
                }
            }
        }
        if (hasA && hasB)
            return true;
    }
    return false;
}

genRegPool::genRegPool()
{
    unk_0e = 1;
}

genRegPool::~genRegPool()
{
}
