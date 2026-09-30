#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool func_0807cc6c(Instr *i, cls08058718 *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int func_0807e130(workTable *wt, Instr *in, cls08058718 *pos, bool upwards);
int func_0804bf50(unsigned int x);
RegPoolEntry *getZeroReg(unsigned int type);

static inline bool isPipeMode(workTable *wt)
{
    return wt->unk_09 == 3 && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

inline bool atEarliest(workTable *wt, Instr *in)
{
    bool found = false;
    if (!in)
        return false;
    int e = earliestInsert(wt, in, &found);
    return found && e == in->unk_13e8.unk_04;
}

bool isAtEarliest(workTable *wt, Instr *in)
{
    bool found = false;
    if (!in)
        return false;
    int e = earliestInsert(wt, in, &found);
    return found && e == in->unk_13e8.unk_04;
}

bool sredInstr(workTable *wt, Instr *in, bool up)
{
    cls08058718 pos;
    int iline = in->unk_13e8.unk_04;
    bool found = false;
    int step;
    bool upwards = false;
    int tline;

    if (up) {
        tline = earliestInsert(wt, in, &found);
        if (tline > iline) {
            short mfs = in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2;
            func_0804ad38("int_Sred.. tline>iline..earlieset insert is after instruction.. mode is upwards %d>%d mfs=%d\n",
                          tline, iline, mfs);
            in->func_08054008(false);
            func_0804ad38("iline=%d tline=%d\n", iline, tline);
            return false;
        }
        step = 1;
        upwards = true;
    } else {
        tline = latestInsert(wt, in, &found);
        if (tline < iline) {
            func_0804ad38("latest possible insert is before instruction..%d<%d!\n", tline, iline);
            in->func_08054008(false);
            return false;
        }
        step = -1;
    }

    if (tline != iline) {
        if (found) {
            pos.func_0805874c(tline);
            if (!func_0807e130(wt, in, &pos, upwards))
                goto move;
            tline += step;
        }
        for (; tline != iline; tline += step) {
            pos.func_0805874c(tline);
            if (wt->func_080724f0(tline)->func_08072304() == 1
                && !func_0807e130(wt, in, &pos, upwards))
                goto move;
        }
    }
    return true;

move:
    if (wt->takeOut(in) && func_0807cc6c(in, &pos) && wt->putIn(in))
        return true;
    func_0804ad38("failed to move inst to tline1\n");
    return false;
}

bool sredTable(workTable *wt)
{
    if (!isPipeMode(wt))
        return true;
    if (wt->unk_30 == wt->unk_34)
        return true;
    if (wt->unk_38) {
        func_0804ad38("can't do modular reduction\n");
        return false;
    }
    setRange(wt->unk_30, wt->unk_34, 0);

    int changed;
    do {
        changed = false;
        for (int i = wt->unk_30; i != wt->unk_34; i++) {
            Instr *in = wt->func_080724f0(i)->onlyEntry();
            if (in) {
                if (in->unk_13e8.unk_04 != i) {
                    func_0804ad38("badly placed instruction detected\n");
                    return false;
                }
                if (!sredInstr(wt, in, true)) {
                    func_0804ad38("failed to sred a line\n");
                    return false;
                }
            }
        }
    } while (changed);

    do {
        changed = false;
        for (int i = wt->unk_34 - 1; i >= wt->unk_30; i--) {
            Instr *in = wt->func_080724f0(i)->onlyEntry();
            if (in && !atEarliest(wt, in)) {
                if (in->unk_13e8.unk_04 != i) {
                    func_0804ad38("badly placed instruction detected\n");
                    return false;
                }
                if (!sredInstr(wt, in, false)) {
                    func_0804ad38("failed to sred a line\n");
                    return false;
                }
            }
        }
    } while (changed);
    return true;
}

int g_080de734;
iInfo *g_080de738;
workTable *g_080de73c;
Instr *g_080de740;
RegPoolEntry *g_080de744;
int g_080de748;

int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask)
{
    Operand *op = &in->unk_38[opIdx];
    g_080de73c = wt;
    g_080de744 = 0;
    *capReg = 0;
    int cap = -1;
    g_080de734 = 0;
    g_080de748 = 0;
    *capMask = 0;
    g_080de738 = 0;
    g_080de740 = 0;

    RegPoolEntry *reg = op->getReg2();
    switch (reg->unk_30) {
    case 52:
        cap = 0x73;
        g_080de744 = func_0806c224(51);
        g_080de748 = 1;
        break;
    case 50:
        g_080de744 = reg;
        g_080de748 = 1;
        cap = 0x74;
        break;
    case 55: case 57: case 59: {
        if (in->unk_13e8.getPipe())
            break;
        RegPoolEntry *r0 = in->unk_38[0].outReg();
        if (!r0) {
            func_0804ad38("couldn't get r0 for %s\n",
                          in->unk_34 ? in->unk_34->unk_04 : "NO_BASE_INST");
            return -1;
        }
        if (r0->unk_24 != 9)
            break;
        RegPool *pool = r0->unk_04;
        if (r0 == pool->unk_10) {
            unsigned char mask[32];
            int i;
            for (i = 0; i != 32; i++) {
                RegPoolEntry *e = pool->func_080593e0(i);
                mask[i] = (e->unk_16 && e->unk_15) ? in->unk_38[0].unk_07 : 0;
            }
            mask[0] = 0;
            for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
                RegPoolEntry *reg = ru->unk_08;
                if (reg->unk_24 != 9)
                    continue;
                if (reg->unk_14 == 1) {
                    func_0804ad38("register isn't base!\n");
                    return -1;
                }
                unsigned char m = ru->unk_0e | ru->unk_0f | ru->unk_10;
                int idx = pool->regIndex(reg->unk_30);
                if (idx < 0) {
                    func_0804ad38("failed to get local id\n");
                    return -1;
                }
                if (m & mask[idx])
                    mask[idx] = 0;
            }
            int best = 0;
            for (i = 1; i != 32; i++) {
                int m = in->unk_38[0].unk_07 & mask[i];
                int n = func_0804bf50(m);
                if (n > best) {
                    g_080de744 = pool->func_080593e0(i);
                    g_080de748 = m;
                    best = n;
                }
            }
        } else {
            g_080de748 = in->unk_38[0].unk_07;
            g_080de744 = r0;
        }
        if (g_080de744)
            cap = 0x21;
        break;
    }
    }

    if (cap < 0)
        return 0;
    iInfo *ii = func_0806bf74(cap);
    g_080de738 = ii;
    if (!ii) {
        func_0804ad38("failed to get II for cap \n");
        return -1;
    }
    *capReg = g_080de744;
    g_080de740 = in;
    *capMask = g_080de748;
    g_080de734 = ii->unk_0e_w;
    return g_080de734;
}

bool doCap(int line, int pipe, int mask)
{
    if (!g_080de738) {
        func_0804ad38("cap_ii not set\n");
        return false;
    }
    if (!mask) {
        func_0804ad38("zero mod bits for doCap\n");
        return false;
    }
    Instr *ci = new Instr(g_080de738);
    if (!ci) {
        func_0804ad38("failed to make cap instruction\n");
        return false;
    }
    ci->unk_138f = mask;
    for (int i = 0; i != ci->unk_138b; i++)
        ci->unk_38[i].unk_07 = mask;

    if (g_080de744->unk_24 == 9) {
        RegPoolEntry *z = getZeroReg(9);
        ci->unk_38[0].unk_18 = z;
        ci->unk_38[0].unk_14 = z;
        ci->unk_38[0].unk_10 = 0;
        ci->unk_38[1].unk_18 = g_080de744;
        ci->unk_38[1].unk_14 = g_080de744;
        ci->unk_38[1].unk_10 = 0;
        g_080de740->unk_38[0].unk_14 = g_080de744;
        g_080de740->unk_38[0].unk_18 = g_080de744;
        g_080de740->unk_38[0].unk_10 = 0;
        regUse *ru = g_080de73c->unk_14.func_0805b5dc(g_080de744);
        if (!ru) {
            ru = g_080de73c->unk_14.AddNewRegRef(g_080de744);
            if (!ru) {
                func_0804ad38("failed to add new regref\n");
                return true;
            }
        }
        ru->unk_0e |= mask;
        ru->unk_0f |= mask;
    } else {
        ci->unk_38[0].unk_18 = g_080de744;
        ci->unk_38[0].unk_14 = g_080de744;
    }
    ci->unk_13e8.func_0805874c(line);
    ci->unk_13e8.func_0805878c(pipe);
    if (!g_080de73c->putIn(ci)) {
        func_0804ad38("failed to put instruction into itable\n");
        return false;
    }
    g_080de738 = 0;
    return true;
}
