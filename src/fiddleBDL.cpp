#include "workTable.h"
#include "Instr.h"
#include "os.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int checkInstructionPlacementConstraints(workTable *wt, Instr *i, InstructionPosition *pos);

static inline bool isRawMode(workTable *scheduleTable)
{
    if (scheduleTable->codeMode() == 3)
        return true;
    return false;
}

static inline bool noReduce(workTable *scheduleTable)
{
    return isRawMode(scheduleTable) && (scheduleTable->rawSchedulingMode == 2 || scheduleTable->rawSchedulingMode == 4);
}

extern const char fiddleBranchPosError[];
extern const char fiddleFailedPrevError[];
extern const char fiddleMoveBranchError[];
extern const char fiddleGetPrevIeError[];
extern const char fiddleGetBranchIeError[];
extern const char fiddleGetLastIeError[];
extern const char fiddleSmallerError[];

bool fiddleBDL(workTable *scheduleTable)
{
    setRange(scheduleTable->rangeStart, scheduleTable->rangeEnd, scheduleTable->loopSize);
    if (noReduce(scheduleTable)) {
        return true;
    } else {
        int len = scheduleTable->rangeEnd - scheduleTable->rangeStart;
        if (len < scheduleTable->externalMinimumLength) {
            reportError(fiddleSmallerError);
            return false;
        }
        if (len == scheduleTable->externalMinimumLength)
            return true;
        if (scheduleTable->loopSize)
            goto done;

        // reduceExt
        Instr *br = scheduleTable->getBranch();
        int extlen = scheduleTable->rangeEnd - scheduleTable->rangeStart;
        if (extlen <= 2 || !br)
            goto done;
        short m = br->externalInputConstraint < br->externalInputPairedConstraint ? br->externalInputPairedConstraint : br->externalInputConstraint;
        if (m > extlen - 3)
            goto done;
        wtRow *last = scheduleTable->getRowByIndex(scheduleTable->rangeEnd - 1);
        if (!last) {
            reportError(fiddleGetLastIeError);
            return false;
        }
        if (last->countEntries())
            goto done;
        wtRow *brow = scheduleTable->getRowByIndex(scheduleTable->rangeEnd - 2);
        if (!brow) {
            reportError(fiddleGetBranchIeError);
            return false;
        }
        wtRow *prow = scheduleTable->getRowByIndex(scheduleTable->rangeEnd - 3);
        if (!prow) {
            reportError(fiddleGetPrevIeError);
            return false;
        }

        InstructionPosition posP;
        {
            InstructionPosition posB;
            short row = scheduleTable->rangeEnd;
            row -= 3;
            posP.setRow(row);
            row = scheduleTable->rangeEnd;
            row -= 2;
            posB.setRow(row);
            bool ok = true;
            if (br != brow->entries[1].instruction) {
                reportError(fiddleBranchPosError);
                return false;
            }
            Instr *prev = prow->entries[1].instruction;
            if (writesInputRegisterOf(prev, br))
                return true;

            if (prev && !scheduleTable->takeOut(prev)) {
                reportError("failed to takeout in_p\n");
                return false;
            }
            if (checkInstructionPlacementConstraints(scheduleTable, br, &posP))
                ok = false;
            if (prev) {
                if (!moveLocks(prev, &posP)) {
                    reportError("failed to undo takeout\n");
                    return false;
                }
                if (!scheduleTable->putIn(prev)) {
                    reportError("failed to takeout in_p\n");
                    return false;
                }
            }
            if (!ok)
                goto success;

            if (prev) {
                if (!scheduleTable->takeOut(br)) {
                    reportError("failed to takeout in_b\n");
                    return false;
                }
                if (checkInstructionPlacementConstraints(scheduleTable, prev, &posB))
                    ok = false;
                int m2 = prev->externalOutputConstraint < prev->externalOutputPairedConstraint ? prev->externalOutputPairedConstraint : prev->externalOutputConstraint;
                if ((short)m2)
                    ok = false;
                if (!moveLocks(br, &posB)) {
                    reportError("failed to undo branch\n");
                    return false;
                }
                if (!scheduleTable->putIn(br)) {
                    reportError("failed to put branch back in\n");
                    return false;
                }
            }
            if (!ok)
                goto success;

            if (!scheduleTable->takeOut(br)) {
                reportError("failed to takeout in_b\n");
                return false;
            }
            if (prev && !scheduleTable->takeOut(prev)) {
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
                if (!scheduleTable->putIn(prev)) {
                    reportError("failed to put prev back in\n");
                    return false;
                }
            }
            if (!scheduleTable->putIn(br)) {
                reportError("failed to put branch back in\n");
                return false;
            }
            scheduleTable->rangeEnd--;
success:
            ;
        }
    }
done:
    return true;
}
