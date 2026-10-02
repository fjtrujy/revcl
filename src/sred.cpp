#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, cls08058718 *pos);
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
            wtRow *row = wt->func_080724f0(tline);
            if (row->func_08072304() == 1
                && !func_0807e130(wt, in, &pos, upwards))
                goto move;
        }
    }
    return true;

move:
    if (wt->takeOut(in) && moveLocks(in, &pos) && wt->putIn(in))
        return true;
    func_0804ad38("failed to move inst to tline1\n");
    return false;
}

bool sredTable(workTable *wt)
{
    if (!isPipeMode(wt))
        return true;
    if (wt->unk_30 != wt->unk_34) {
    if (wt->unk_38) {
        func_0804ad38("can't do modular reduction\n");
        return false;
    }
    setRange(wt->unk_30, wt->unk_34, 0);

    int changed;
    int i;
    do {
        changed = false;
        for (i = wt->unk_30; i != wt->unk_34; i++) {
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
        for (i = wt->unk_34 - 1; i >= wt->unk_30; i--) {
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
    }
    return true;
}

int g_080de734;
iInfo *g_080de738;
workTable *g_080de73c;
Instr *g_080de740;
RegPoolEntry *g_080de744;
int g_080de748;
