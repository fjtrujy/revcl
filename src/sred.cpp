#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int countSetBits(unsigned int x);
RegPoolEntry *getZeroReg(unsigned int type);

static inline bool isRawTable(workTable *wt)
{
    return wt->codeModeValue == 3;
}

static inline bool isPipeMode(workTable *wt)
{
    return isRawTable(wt) && (wt->rawSchedulingMode == 1 || wt->rawSchedulingMode == 3);
}

inline bool atEarliest(workTable *wt, Instr *in)
{
    bool found = false;
    if (!in)
        return false;
    int e = earliestInsert(wt, in, &found);
    return found && e == in->position.loopRow;
}

bool isAtEarliest(workTable *wt, Instr *in)
{
    bool found = false;
    if (!in)
        return false;
    int e = earliestInsert(wt, in, &found);
    return found && e == in->position.loopRow;
}

static inline void dumpSredInstruction(Instr *in)
{
    in->dumpOperandDiagnostics(false);
}

extern const char sredEarliestAfterError[];
extern const char sredMoveInstError[];
extern const char sredLatestBeforeError[];
extern const char sredBadlyPlacedError[];

bool sredInstr(workTable *wt, Instr *in, bool up)
{
    InstructionPosition pos;
    int iline = in->position.loopRow;
    bool found = false;
    int step;
    bool upwards = false;
    int tline;

    if (up) {
        tline = earliestInsert(wt, in, &found);
        if (tline > iline) {
            short mfs = in->externalInputConstraint < in->externalInputPairedConstraint ? in->externalInputPairedConstraint : in->externalInputConstraint;
            reportError(sredEarliestAfterError,
                          tline, iline, mfs);
            in->dumpOperandDiagnostics(false);
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
        if (!checkPlacementInAllowedPipelines(wt, in, &pos, upwards))
            goto move;
        tline += step;
    }
    for (; tline != iline; tline += step) {
        pos.setFromLinearRow(tline);
        wtRow *row = wt->getRowByIndex(tline);
        if (row->countEntries() == 1
            && !checkPlacementInAllowedPipelines(wt, in, &pos, upwards))
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
    int i = wt->rangeStart;
    int n = wt->rangeEnd;
    if (i != n) {
        i = n = wt->loopSize;
        if (i) {
            reportError("can't do modular reduction\n");
            return false;
        }
        setRange(wt->rangeStart, wt->rangeEnd, 0);

        int changed;
        do {
            changed = false;
            for (i = wt->rangeStart; i != wt->rangeEnd; i++) {
                Instr *in = wt->getRowByIndex(i)->onlyEntry();
                if (in) {
                    if (in->position.loopRow != i) {
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
            for (i = wt->rangeEnd - 1; i >= wt->rangeStart; i--) {
                Instr *in = wt->getRowByIndex(i)->onlyEntry();
                if (in && !atEarliest(wt, in)) {
                    if (in->position.loopRow != i) {
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
RegPoolEntry *selectedCapRegister;
int selectedCapMask;
