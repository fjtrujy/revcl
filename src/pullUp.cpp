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
    Instr *branchInstruction = scheduleTable->getBranch();
    if (branchInstruction) {
        lbdli = scheduleTable->rows[scheduleTable->rangeEnd - 1].entries[1].instruction;
        if (lbdli)
            lbdli = writesInputRegisterOf(lbdli, branchInstruction) ? lbdli : 0;
    }

    do {
        moved = 0;
        mrl = scheduleTable->rangeStart;
        for (int rowIndex = scheduleTable->rangeStart; rowIndex != scheduleTable->rangeEnd; rowIndex++) {
            wtRow *row = scheduleTable->getRowByIndex(rowIndex);
            for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                Instr *currentInstruction = row->getEntry(pipeIndex);
                if (!currentInstruction || currentInstruction == branchInstruction || currentInstruction == lbdli)
                    continue;
                int instructionRow = currentInstruction->position.getRow();
                if (instructionRow != rowIndex) {
                    reportError("rl not l!\n");
                    exitAfterCleanup(-1);
                }
                int earliestAllowedRow = earliestInsert(scheduleTable, currentInstruction, 0);
                if (earliestAllowedRow < instructionRow) {
                    if (!doLocalMove(scheduleTable, currentInstruction, earliestAllowedRow, instructionRow, 1)) {
                        reportError("failed to do local move for raw up\n");
                        return -1;
                    }
                    if (currentInstruction->position.getRow() != instructionRow)
                        moved++;
                }
                short mte = currentInstruction->externalOutputConstraint < currentInstruction->externalOutputPairedConstraint ? currentInstruction->externalOutputPairedConstraint : currentInstruction->externalOutputConstraint;
                int end = currentInstruction->position.getRow() + mte;
                if (end > mrl)
                    mrl = end;
                if (mrl >= scheduleTable->rangeEnd) {
                    reportError("mRline would be pushed after m_iEnd..%d, %d\n", mrl, scheduleTable->rangeEnd);
                    scheduleTable->dump();
                    int diagnosticMte = currentInstruction->externalOutputConstraint < currentInstruction->externalOutputPairedConstraint ? currentInstruction->externalOutputPairedConstraint : currentInstruction->externalOutputConstraint;
                    reportError("faulting instruction is (mte=%d):\n", diagnosticMte);
                    currentInstruction->leDump();
                    return -1;
                }
            }
        }
    } while (moved);

    if (mrl - scheduleTable->rangeStart + 1 < scheduleTable->externalMinimumLength)
        mrl = scheduleTable->rangeStart + scheduleTable->externalMinimumLength - 1;

    int branchPresent = branchInstruction != 0;
    if (branchPresent) {
        int startingRowIndex = mrl - 1;
        bool done = false;
        if (startingRowIndex < scheduleTable->rangeStart)
            startingRowIndex = scheduleTable->rangeStart;
        while (!done) {
            if (startingRowIndex >= scheduleTable->rangeEnd) {
                reportError("branch line overrun\n");
                return -1;
            }
            InstructionPosition pos;
            pos.setInstructionPipe(branchInstruction->position.pipelineIndex);
            pos.setFromLinearRow(startingRowIndex);
            if (checkInstructionPlacementConstraints(scheduleTable, branchInstruction, &pos)) {
                startingRowIndex++;
            } else {
                if (!scheduleTable->takeOut(branchInstruction)) {
                    reportError("failed to take out instruction\n");
                    exitAfterCleanup(-1);
                }
                if (!moveLocks(branchInstruction, &pos)) {
                    reportError("failed to move !\n");
                    exitAfterCleanup(-1);
                }
                if (!scheduleTable->putIn(branchInstruction)) {
                    reportError("failed to put in back in\n");
                    exitAfterCleanup(-1);
                }
                startingRowIndex++;
                if (startingRowIndex > mrl)
                    mrl = startingRowIndex;
                if (lbdli) {
                    pos.setFromLinearRow(startingRowIndex);
                    pos.setInstructionPipe(lbdli->position.pipelineIndex);
                    if (startingRowIndex >= scheduleTable->rangeEnd) {
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
                    if (startingRowIndex > mrl)
                        mrl = startingRowIndex;
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
