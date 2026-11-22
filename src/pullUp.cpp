#include "linearReduce_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(workTable *t);
bool moveLocks(Instr *i, InstructionPosition *pos);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
void dumpProgramStreamsAndRegisters(Program *p, int a);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int checkInstructionPlacementConstraints(workTable *wt, Instr *i, InstructionPosition *pos);
bool setRange(int start, int end, int lsz);
int pullUp(workTable *wt);
int dragDown(workTable *wt);

static inline bool setPullUpRange(workTable *scheduleTable, int lsz)
{
    return setRange(scheduleTable->rangeStart, scheduleTable->rangeEnd, lsz);
}

int pullUp(workTable *scheduleTable)
{
    if (scheduleTable->rangeEnd == scheduleTable->rangeStart)
        return 0;
    if (scheduleTable->loopSize) {
        reportError("raw_PullUp it has loop\n");
        return -1;
    }
    setRangeFrom(scheduleTable);

    int moved;
    int mrl;
    Instr *lbdli = 0;
    Instr *br = scheduleTable->getBranch();
    if (br) {
        lbdli = scheduleTable->rows[scheduleTable->rangeEnd - 1].entries[1].instruction;
        if (lbdli)
            lbdli = writesInputRegisterOf(lbdli, br) ? lbdli : 0;
    }

    do {
        moved = 0;
        mrl = scheduleTable->rangeStart;
        for (int l = scheduleTable->rangeStart; l != scheduleTable->rangeEnd; l++) {
            wtRow *row = scheduleTable->getRowByIndex(l);
            for (int p = 0; p != 2; p++) {
                Instr *in = row->getEntry(p);
                if (!in || in == br || in == lbdli)
                    continue;
                int rl = in->position.getRow();
                if (rl != l) {
                    reportError("rl not l!\n");
                    exitAfterCleanup(-1);
                }
                int e = earliestInsert(scheduleTable, in, 0);
                if (e < rl) {
                    if (!doLocalMove(scheduleTable, in, e, rl, 1)) {
                        reportError("failed to do local move for raw up\n");
                        return -1;
                    }
                    if (in->position.getRow() != rl)
                        moved++;
                }
                short mte = in->externalOutputConstraint < in->externalOutputPairedConstraint ? in->externalOutputPairedConstraint : in->externalOutputConstraint;
                int end = in->position.getRow() + mte;
                if (end > mrl)
                    mrl = end;
                if (mrl >= scheduleTable->rangeEnd) {
                    reportError("mRline would be pushed after m_iEnd..%d, %d\n", mrl, scheduleTable->rangeEnd);
                    scheduleTable->dump();
                    int diagnosticMte = in->externalOutputConstraint < in->externalOutputPairedConstraint ? in->externalOutputPairedConstraint : in->externalOutputConstraint;
                    reportError("faulting instruction is (mte=%d):\n", diagnosticMte);
                    in->leDump();
                    return -1;
                }
            }
        }
    } while (moved);

    if (mrl - scheduleTable->rangeStart + 1 < scheduleTable->externalMinimumLength)
        mrl = scheduleTable->rangeStart + scheduleTable->externalMinimumLength - 1;

    int branchPresent = br != 0;
    if (branchPresent) {
        int l = mrl - 1;
        bool done = false;
        if (l < scheduleTable->rangeStart)
            l = scheduleTable->rangeStart;
        while (!done) {
            if (l >= scheduleTable->rangeEnd) {
                reportError("branch line overrun\n");
                return -1;
            }
            InstructionPosition pos;
            pos.setInstructionPipe(br->position.pipelineIndex);
            pos.setFromLinearRow(l);
            if (checkInstructionPlacementConstraints(scheduleTable, br, &pos)) {
                l++;
            } else {
                if (!scheduleTable->takeOut(br)) {
                    reportError("failed to take out instruction\n");
                    exitAfterCleanup(-1);
                }
                if (!moveLocks(br, &pos)) {
                    reportError("failed to move !\n");
                    exitAfterCleanup(-1);
                }
                if (!scheduleTable->putIn(br)) {
                    reportError("failed to put in back in\n");
                    exitAfterCleanup(-1);
                }
                l++;
                if (l > mrl)
                    mrl = l;
                if (lbdli) {
                    pos.setFromLinearRow(l);
                    pos.setInstructionPipe(lbdli->position.pipelineIndex);
                    if (l >= scheduleTable->rangeEnd) {
                        reportError("over run for bdli\n");
                        return -1;
                    }
                    if (checkInstructionPlacementConstraints(scheduleTable, lbdli, &pos))
                        continue;
                    if (!scheduleTable->takeOut(lbdli)) {
                        reportError("failed to take out lbdli instruction\n");
                        exitAfterCleanup(-1);
                    }
                    if (!moveLocks(lbdli, &pos)) {
                        reportError("failed to move lbdli !\n");
                        exitAfterCleanup(-1);
                    }
                    if (!scheduleTable->putIn(lbdli)) {
                        reportError("failed to put in back in lbdli\n");
                        exitAfterCleanup(-1);
                    }
                    if (l > mrl)
                        mrl = l;
                }
                done = true;
            }
        }
    }

    int ret = scheduleTable->rangeEnd - mrl - 1;
    int end = mrl + 1;
    if (end > scheduleTable->rangeEnd) {
        reportError("table would grow ! mrl=%d..m_iE=%d\n", mrl, scheduleTable->rangeEnd);
        scheduleTable->dump();
        return -1;
    }
    scheduleTable->rangeEnd = end;
    end = scheduleTable->loopSize;
    setPullUpRange(scheduleTable, end);
    return ret;
}
