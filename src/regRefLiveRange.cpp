#include "genRegPool_shared.h"

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


static inline bool validPipe(int p)
{
    return p == 0 || p == 1;
}

bool regRefLiveRange(regUse *ru, int part, int *s, int *e, bool *always)
{
    *s = -4;
    *e = -3;
    *always = false;
    regUseSlot *sl = &ru->unk_20[part];
    if ((char)bitSet(ru->unk_0e, part)) {
        if (sl->unk_3c[0] < 0 || !validPipe(sl->unk_3c[1]) ||
            sl->unk_3c[3] < 0 || !validPipe(sl->unk_3c[4])) {
            func_0804ad38("firstInput<0.. but iBit set\n");
            func_0804adf0(-1);
        }
    }
    if ((char)bitSet(ru->unk_0f, part)) {
        if (sl->unk_3c[5] < 0 || !validPipe(sl->unk_3c[6]) ||
            sl->unk_3c[8] < 0 || !validPipe(sl->unk_3c[9])) {
            func_0804ad38("firstInput<0.. but iBit set\n");
            func_0804adf0(-1);
        }
    }
    if ((char)bitSet(ru->unk_10 & ~(ru->unk_0f | ru->unk_0e), part)) {
        *s = -1;
        *e = 30000;
        *always = true;
        return true;
    }
    if ((char)bitSet(ru->unk_0c, part)) {
        *s = -1;
    } else {
        if ((char)bitSet(ru->unk_0e, part) && sl->unk_3c[0] <= sl->unk_3c[5]) {
            func_0804ad38("first input before first output.. but not xi %d,%d!\n",
                          sl->unk_3c[0], sl->unk_3c[5]);
            func_0804adf0(-1);
        }
        if ((char)bitSet(ru->unk_0f, part))
            *s = sl->unk_3c[5];
    }
    if ((char)bitSet(ru->unk_10 | ru->unk_0d, part)) {
        *e = 30000;
    } else {
        if ((char)bitSet(ru->unk_0f, part))
            *e = sl->unk_3c[8] + 1;
        if ((char)bitSet(ru->unk_0e, part) && sl->unk_3c[3] > *e)
            *e = sl->unk_3c[3];
    }
    int row = *s;
    if (*e <= row) {
        row = *e;
        func_0804ad38("liverange for regref has e<=s .. %d,%d?\n", *s, row);
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

bool lockLiveRange(cls0805c244 *ll, cls0805bcc4 *out, int *s, int *e)
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
        int line = rl->getRow();
        int want = out->unk_32;
        if ((short)line != (short)want) {
            func_0804ad38("rLine mismatch\n");
            return false;
        }
        if (!validPipe(rl->getPipe())) {
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
        int line = rl->getRow();
        int want = c->unk_32;
        if ((short)line != (short)want) {
            func_0804ad38("rline mismatch for input locks\n");
            return false;
        }
        if (!validPipe(rl->getPipe())) {
            func_0804ad38("bad pipe for instruction\n");
            return false;
        }
        if ((short)line <= *s) {
            func_0804ad38("el<=s, but can't age\n");
            return false;
        }
        if ((short)line > *e)
            *e = (short)line;
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
                if (isRegisterOperand(op, 2)) {
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

