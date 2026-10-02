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

bool assignLocalRegisters(Program *p, workTable *wt)
{
    int len = wt->getPhysicalLen();
    if (len < 0) {
        func_0804ad38("nAssignRegisters: istream had table, but was bad!\n");
        func_0804adf0(-1);
    }
    if (wt->unk_94.unk_08) {
        func_0804ad38("nAssign - still things to do .. %d\n", wt->unk_94.unk_08);
        for (Instr *in = wt->unk_94.unk_00; in; in = in->unk_24.unk_00)
            in->func_08054008(false);
        func_0804adf0(-1);
    }
    if (wt->unk_08 != 2) {
        func_0804ad38("AssignLocalRegisters - external assign hadn't happened!\n");
        func_0804adf0(-1);
    }
    unsigned int mode = wt->unk_09;
    if ((unsigned char)mode != 3) {
        func_0804ad38("AssignLocalRegisters - can't do local assign on non-vsm table (%d)\n", mode);
        func_0804adf0(-1);
    }
    if (!wt->unk_0b) {
        func_0804ad38("locks aren't valid for local register assign\n");
        func_0804adf0(-1);
    }
    if (len) {
        setRange(wt->func_0807411c(), wt->func_080741c0(), wt->func_080741b4());
        for (unsigned i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && !rp->func_08088e54(wt))
                return false;
        }
        if (!func_0806f6a4(p)) {
            func_0804ad38("failed to clean up after local alloc\n");
            func_0804aeb0();
            return false;
        }
    }
    wt->unk_08 = 3;
    return true;
}

void clearATab()
{
    func_0804c220(aTab_080bce40, 0x20000);
}

void clearTTab()
{
    func_0804c220(tTab_080dce40, 0x1000);
}

void genRegPool::func_08089a4c()
{
    func_0804c220(unk_30, 0x20);
}

bool genRegPool::build_tTab(RegPoolEntry *reg, unsigned char mask)
{
    func_0804c220(tTab_080dce40, 0x1000);
    if (!mask)
        return true;
    char set = 0;
    if (!reg) {
        func_0804ad38("build_tTab - passed a null pointer\n");
        return false;
    }
    int lsz = g_080b336c;
    for (int i = 0; i != reg->unk_28; i++) {
        bool on = (mask >> i) & 1;
        if (!on)
            continue;
        cls0805c244 *ll = (cls0805c244 *)reg->func_0805a908(i);
        if (!ll) {
            func_0804ad38("failed to get a lock list for %s\n", reg->func_0805a8e0());
            return false;
        }
        cls0805bcc4 *lk = ll->unk_1c;
        if (!lk)
            continue;
        bool shared = ll->unk_2b != 0;
        set |= 1 << i;
        if (lsz && ll->unk_2a)
            shared = true;
        for (; lk; lk = lk->unk_08) {
            int s = 0, e = 0;
            if (!lockLiveRange(ll, lk, &s, &e)) {
                func_0804ad38("failed to get live range for an rLock\n");
                func_0804adf0(-1);
            }
            if (s >= 0) {
                if (lk->unk_2d == 2) {
                    func_0804ad38("extern in .. rl_s>0\n");
                    return false;
                }
            } else {
                if (lk->unk_2d != 2) {
                    func_0804ad38("not extern in .. -ve s\n");
                    return false;
                }
                if (lsz) {
                    if (e == 30000)
                        s = 0;
                    else
                        s = (e / lsz) * lsz;
                } else
                    s = 0;
            }
            if (e == 30000) {
                if (lsz)
                    e = s + lsz - s % lsz;
                else
                    e = tEnd_080bce2c;
            }
            int r = lk->unk_30;
            if (r >= 0)
                tTab_080dce40[r] |= 0x80;
            for (int c = s; c < e; c++) {
                int idx = c;
                if (lsz)
                    idx = c % lsz;
                unsigned char v = tTab_080dce40[idx];
                if (((v >> i) & 1) && !shared) {
                    func_0804ad38("build_tTab - already set.. so some overlap %d?\n", ll->unk_2b != 0);
                    ll->checkCounts();
                    dumpTTab();
                    return false;
                }
                tTab_080dce40[idx] = v | (1 << i);
            }
        }
    }
    if (!set) {
        func_0804ad38("build_tTab - set no bits! for %s\n", reg->func_0805a8e0());
        func_0804adf0(-1);
    }
    return true;
}

bool genRegPool::initAlloc(int start, int end)
{
    func_0804c220(aTab_080bce40, 0x20000);
    func_0804c220(unk_30, 0x20);
    if (start) {
        func_0804ad38("for now - can't base an alloc table other than at zero\n");
        return false;
    }
    if (end < start) {
        func_0804ad38("regpool - assign registers -done table was bad\n");
        return false;
    }
    if (g_080b336c && end != g_080b336c)
        func_0804ad38("loop mismatch\n");
    tStart_080bce28 = start;
    tEnd_080bce2c = end;
    if (end > 0xfff) {
        func_0804ad38("table too big. coder can't be bothered to realloc\n");
        return false;
    }
    RegPoolEntry *e = unk_04;
    if (!e) {
        func_0804ad38("RegPool:Dump - no table?\n");
        return false;
    }
    if (unk_08 > 0x20) {
        func_0804ad38("alloc - fatal - should catch earlier - m_nReg>=MAX_PER_DEV\n");
        return false;
    }
    nRegs_080bce24 = unk_08;
    maxParts_080bce20 = 0;
    for (int i = 0; i != unk_08; i++, e++) {
        if (e->unk_16)
            unk_30[i] = (1 << e->unk_28) - 1;
        if (e->unk_28 > maxParts_080bce20)
            maxParts_080bce20 = e->unk_28;
    }
    return true;
}

void genRegPool::dumpTTab()
{
    for (int i = tStart_080bce28; i < tEnd_080bce2c; i++) {
        char c = tTab_080dce40[i];
        if (c)
            ;
    }
}

int genRegPool::countTTab()
{
    int n = 0;
    for (int i = tStart_080bce28; i < tEnd_080bce2c; i++)
        if (tTab_080dce40[i])
            n++;
    return n;
}

int genRegPool::fitCost(int reg)
{
    int total = 0;
    for (int i = tStart_080bce28; i < tEnd_080bce2c; i++) {
        unsigned char t = tTab_080dce40[i];
        int m = t & 0x7f;
        if (((unsigned char)unk_30[reg] & m) != m)
            return -1;
        char *row = &aTab_080bce40[i * 32];
        unsigned char a = row[reg];
        if (t & a)
            return -1;
        int n = func_0804bf50((t | a) & 0x7f);
        if (n > maxParts_080bce20) {
            func_0804ad38("nbits overrun \n");
            func_0804adf0(-1);
        }
        total += maxParts_080bce20 - n;
    }
    if (total < 0)
        total = 0;
    return total;
}

bool genRegPool::set_t_in_aTab(int reg)
{
    int end = tEnd_080bce2c;
    for (int i = tStart_080bce28; i < end; i++) {
        unsigned char c = tTab_080dce40[i];
        int t = c;
        if (((unsigned char)unk_30[reg] & t & 0x7f) != (c & 0x7f)) {
            func_0804ad38("couldnt fit at user has no access..id=%d\n", reg);
            return false;
        }
        char *row = &aTab_080bce40[i * 32];
        unsigned char a = row[reg];
        if (a & t) {
            func_0804ad38("set_t_in_aTab - overlap detected at line %d, register %d our mask is %x vs %x!\n",
                          i, reg, a, t);
            return false;
        }
        row[reg] = a | c;
    }
    return true;
}

void genRegPool::dumpATab()
{
    for (int i = tStart_080bce28; i != tEnd_080bce2c; i++)
        for (int r = 0; r != nRegs_080bce24; r++) {
            int m = (unsigned char)unk_30[r];
            for (int p = 0; p != maxParts_080bce20; p++) {
                if (m & (1 << p))
                    ;
            }
        }
}

int genRegPool::bestReg()
{
    int best = 100000000;
    int which = -1;
    for (int r = 0; r != nRegs_080bce24; r++) {
        int c = fitCost(r);
        if (c < 0)
            continue;
        if (c == 0)
            return r;
        if (c < best) {
            best = c;
            which = r;
        }
    }
    return which;
}

bool genRegPool::set_in_aTab(regUse *ru)
{
    RegPoolEntry *reg = ru->unk_08;
    if (!reg) {
        func_0804ad38("regref had no register\n");
        return false;
    }
    if (func_08059e20(reg)) {
        if (reg->unk_14) {
            func_0804ad38("RegPool::set_in_aTab,regref: alloc..had forced virtual register %s\n",
                          reg->func_0805a8e0());
            func_0804adf0(-1);
        }
        int id = reg->regIndex();
        if (id < 0) {
            func_0804ad38("failed to get local_id from a mapped register\n");
            func_0804adf0(-1);
        }
        if (reg->unk_16) {
            unsigned char mask = ru->unk_0e | ru->unk_0f | ru->unk_10;
            if (!mask) {
                func_0804ad38("error here\n");
                func_0804adf0(-1);
            }
            int set = 0;
            for (int i = 0; i != reg->unk_28; i++) {
                bool on = (mask >> i) & 1;
                if (on && !reg->func_0805a908(i)) {
                    func_0804ad38("failed to get rll\n");
                    return false;
                }
                cls0805c244 *ll = (cls0805c244 *)reg->func_0805a908(i);
                if (ll) {
                    if (!ll->unk_03)
                        set |= 1 << i;
                    ll->unk_03 = 1;
                }
            }
            if (set) {
                if (!build_tTab(reg, set)) {
                    func_0804ad38("failed to build ttab Forced build for mapped reg %s from %s\n",
                                  reg->func_0805a8e0(), reg->func_0805a8e0());
                    return false;
                }
                if (!set_t_in_aTab(id)) {
                    func_0804ad38("failed to check_set t_in_aTab for '%s' at local ID=%d map\n",
                                  reg->func_0805a8e0(), id);
                    dumpTTab();
                    dumpATab();
                    return false;
                }
            }
        }
    }
    return true;
}
