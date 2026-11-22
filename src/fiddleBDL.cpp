#include "workTable.h"
#include "Instr.h"
#include "os.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int func_0807dc44(workTable *wt, Instr *i, InstructionPosition *pos);

static inline bool isRawMode(workTable *wt)
{
    if (wt->codeMode() == 3)
        return true;
    return false;
}

static inline bool noReduce(workTable *wt)
{
    return isRawMode(wt) && (wt->rawSchedulingMode == 2 || wt->rawSchedulingMode == 4);
}

extern const char fiddleBranchPosError[];
extern const char fiddleFailedPrevError[];
extern const char fiddleMoveBranchError[];
extern const char fiddleGetPrevIeError[];
extern const char fiddleGetBranchIeError[];
extern const char fiddleGetLastIeError[];
extern const char fiddleSmallerError[];

bool fiddleBDL(workTable *wt)
{
    setRange(wt->rangeStart, wt->rangeEnd, wt->loopSize);
    if (noReduce(wt)) {
        return true;
    } else {
        int len = wt->rangeEnd - wt->rangeStart;
        if (len < wt->externalMinimumLength) {
            reportError(fiddleSmallerError);
            return false;
        }
        if (len == wt->externalMinimumLength)
            return true;
        if (wt->loopSize)
            goto done;

        // reduceExt
        Instr *br = wt->getBranch();
        int extlen = wt->rangeEnd - wt->rangeStart;
        if (extlen <= 2 || !br)
            goto done;
        short m = br->unk_13b2 < br->unk_13b0 ? br->unk_13b0 : br->unk_13b2;
        if (m > extlen - 3)
            goto done;
        wtRow *last = wt->getRowByIndex(wt->rangeEnd - 1);
        if (!last) {
            reportError(fiddleGetLastIeError);
            return false;
        }
        if (last->countEntries())
            goto done;
        wtRow *brow = wt->getRowByIndex(wt->rangeEnd - 2);
        if (!brow) {
            reportError(fiddleGetBranchIeError);
            return false;
        }
        wtRow *prow = wt->getRowByIndex(wt->rangeEnd - 3);
        if (!prow) {
            reportError(fiddleGetPrevIeError);
            return false;
        }

        InstructionPosition posP;
        {
            InstructionPosition posB;
            short row = wt->rangeEnd;
            row -= 3;
            posP.setRow(row);
            row = wt->rangeEnd;
            row -= 2;
            posB.setRow(row);
            bool ok = true;
            if (br != brow->entries[1].unk_00) {
                reportError(fiddleBranchPosError);
                return false;
            }
            Instr *prev = prow->entries[1].unk_00;
            if (writesInputRegisterOf(prev, br))
                return true;

            if (prev && !wt->takeOut(prev)) {
                reportError("failed to takeout in_p\n");
                return false;
            }
            if (func_0807dc44(wt, br, &posP))
                ok = false;
            if (prev) {
                if (!moveLocks(prev, &posP)) {
                    reportError("failed to undo takeout\n");
                    return false;
                }
                if (!wt->putIn(prev)) {
                    reportError("failed to takeout in_p\n");
                    return false;
                }
            }
            if (!ok)
                goto success;

            if (prev) {
                if (!wt->takeOut(br)) {
                    reportError("failed to takeout in_b\n");
                    return false;
                }
                if (func_0807dc44(wt, prev, &posB))
                    ok = false;
                int m2 = prev->unk_13b6 < prev->unk_13b4 ? prev->unk_13b4 : prev->unk_13b6;
                if ((short)m2)
                    ok = false;
                if (!moveLocks(br, &posB)) {
                    reportError("failed to undo branch\n");
                    return false;
                }
                if (!wt->putIn(br)) {
                    reportError("failed to put branch back in\n");
                    return false;
                }
            }
            if (!ok)
                goto success;

            if (!wt->takeOut(br)) {
                reportError("failed to takeout in_b\n");
                return false;
            }
            if (prev && !wt->takeOut(prev)) {
                reportError("failed to takeout in_p\n");
                return false;
            }
            if (!moveLocks(br, &posP)) {
                reportError(fiddleMoveBranchError);
                return false;
            }
            if (prev) {
                if (!moveLocks(prev, &posB)) {
                    reportError(fiddleFailedPrevError);
                    return false;
                }
                if (!wt->putIn(prev)) {
                    reportError("failed to put prev back in\n");
                    return false;
                }
            }
            if (!wt->putIn(br)) {
                reportError("failed to put branch back in\n");
                return false;
            }
            wt->rangeEnd--;
success:
            ;
        }
    }
done:
    return true;
}
