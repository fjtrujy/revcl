#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int func_0804bf50(unsigned int x);
RegPoolEntry *getZeroReg(unsigned int type);

static inline bool isRawTable(workTable *wt)
{
    return wt->unk_09 == 3;
}

static inline bool isPipeMode(workTable *wt)
{
    return isRawTable(wt) && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

inline bool atEarliest(workTable *wt, Instr *in)
{
    bool found = false;
    if (!in)
        return false;
    int e = earliestInsert(wt, in, &found);
    return found && e == in->position.unk_04;
}

bool isAtEarliest(workTable *wt, Instr *in)
{
    bool found = false;
    if (!in)
        return false;
    int e = earliestInsert(wt, in, &found);
    return found && e == in->position.unk_04;
}

static inline void dumpSredInstruction(Instr *in)
{
    in->func_08054008(false);
}

extern const char sredEarliestAfterError[];
extern const char sredMoveInstError[];
extern const char sredLatestBeforeError[];
extern const char sredBadlyPlacedError[];

bool sredInstr(workTable *wt, Instr *in, bool up)
{
    InstructionPosition pos;
    int iline = in->position.unk_04;
    bool found = false;
    int step;
    bool upwards = false;
    int tline;

    if (up) {
        tline = earliestInsert(wt, in, &found);
        if (tline > iline) {
            short mfs = in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2;
            reportError(sredEarliestAfterError,
                          tline, iline, mfs);
            in->func_08054008(false);
            reportError("iline=%d tline=%d\n", iline, tline);
            return false;
        }
        step = 1;
        upwards = true;
    } else {
        tline = latestInsert(wt, in, &found);
        if (tline < iline) {
            reportError(sredLatestBeforeError, tline, iline);
            dumpSredInstruction(in);
            return false;
        }
        step = -1;
    }

    if (tline == iline)
        return true;
    if (found) {
        pos.setFromLinearRow(tline);
        if (!func_0807e130(wt, in, &pos, upwards))
            goto move;
        tline += step;
    }
    for (; tline != iline; tline += step) {
        pos.setFromLinearRow(tline);
        wtRow *row = wt->getRowByIndex(tline);
        if (row->countEntries() == 1
            && !func_0807e130(wt, in, &pos, upwards))
            goto move;
    }
    return true;

move:
    if (!wt->takeOut(in) || !moveLocks(in, &pos) || !wt->putIn(in)) {
        reportError(sredMoveInstError);
        return false;
    }
    return true;
}

bool sredTable(workTable *wt)
{
    if (!isPipeMode(wt))
        return true;
    int i = wt->unk_30;
    int n = wt->unk_34;
    if (i != n) {
        i = n = wt->unk_38;
        if (i) {
            reportError("can't do modular reduction\n");
            return false;
        }
        setRange(wt->unk_30, wt->unk_34, 0);

        int changed;
        do {
            changed = false;
            for (i = wt->unk_30; i != wt->unk_34; i++) {
                Instr *in = wt->getRowByIndex(i)->onlyEntry();
                if (in) {
                    if (in->position.unk_04 != i) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(wt, in, true)) {
                        reportError("failed to sred a line\n");
                        return false;
                    }
                }
            }
        } while (changed);

        do {
            changed = false;
            for (i = wt->unk_34 - 1; i >= wt->unk_30; i--) {
                Instr *in = wt->getRowByIndex(i)->onlyEntry();
                if (in && !atEarliest(wt, in)) {
                    if (in->position.unk_04 != i) {
                        reportError(sredBadlyPlacedError);
                        return false;
                    }
                    if (!sredInstr(wt, in, false)) {
                        reportError("failed to sred a line\n");
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
