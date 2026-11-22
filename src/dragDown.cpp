#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

static inline void refreshDragDownRange(workTable *scheduleTable, int lsz)
{
    setRange(scheduleTable->rangeStart, scheduleTable->rangeEnd, lsz);
}

static inline void limitDragDownStart(workTable *scheduleTable, int &start)
{
    if (scheduleTable->rangeEnd - start < scheduleTable->externalMinimumLength)
        start = scheduleTable->rangeEnd - scheduleTable->externalMinimumLength;
}

int dragDown(workTable *scheduleTable)
{
    if (scheduleTable->rangeEnd == scheduleTable->rangeStart)
        return 0;
    scheduleTable->getBranch();
    if (scheduleTable->loopSize) {
        reportError("hit this debug assert - loopsize!=0\n");
        abortAfterError();
    }
    setRange(scheduleTable->rangeStart, scheduleTable->rangeEnd, scheduleTable->loopSize);

    int moved;
    int msl;
    int oStart = scheduleTable->rangeStart;
    do {
        moved = 0;
        msl = scheduleTable->rangeEnd;
        for (int rowIndex = scheduleTable->rangeEnd - 1; rowIndex >= scheduleTable->rangeStart; rowIndex--) {
            wtRow *row = scheduleTable->getRowByIndex(rowIndex);
            for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                Instr *instruction = row->getEntry(pipeIndex);
                if (instruction && !instruction->branchFlags) {
                    int instructionRow = instruction->position.getRow();
                    short mte = instruction->externalOutputConstraint < instruction->externalOutputPairedConstraint ? instruction->externalOutputPairedConstraint : instruction->externalOutputConstraint;
                    int minimumAllowedRow = scheduleTable->rangeEnd - mte - 1;
                    if (instructionRow != rowIndex) {
                        reportError("rl not l!\n");
                        exitAfterCleanup(-1);
                    }
                    int operandIndex;
                    Operand *operand;
                    for (operandIndex = 0; (operand = instruction->getOp(operandIndex)) != 0; operandIndex++) {
                        RegPoolEntry *reg = registerOperand(operand, 2);
                        if (reg) {
                            for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                                OpSlot *operandSlot = &operand->parts[partIndex];
                                if ((operand->componentMask >> partIndex) & 1) {
                                    int latestAllowedRow = scheduleTable->rangeEnd - 1;
                                    if (operandSlot->scheduledConsumerCount)
                                        latestAllowedRow = operandSlot->earliestConsumerFreeCycle - operandSlot->issueLatency;
                                    if (latestAllowedRow < minimumAllowedRow)
                                        minimumAllowedRow = latestAllowedRow;
                                }
                            }
                        }
                        if (registerOperand(operand, 1) && operand->reductionHazardMask)
                            minimumAllowedRow = instructionRow;
                    }
                    if (minimumAllowedRow > instructionRow) {
                        if (!doLocalMove(scheduleTable, instruction, minimumAllowedRow, instructionRow, -1)) {
                            reportError("failed to do local move for raw down\n");
                            return -1;
                        }
                        if (instruction->position.getRow() != instructionRow)
                            moved++;
                    }
                }
                if (instruction) {
                    int earliestAllowedRow = instruction->position.getRow()
                          - (instruction->externalInputConstraint < instruction->externalInputPairedConstraint ? instruction->externalInputPairedConstraint : instruction->externalInputConstraint);
                    if (earliestAllowedRow < msl)
                        msl = earliestAllowedRow;
                    if (msl < oStart) {
                        short mfs = instruction->externalInputConstraint < instruction->externalInputPairedConstraint ? instruction->externalInputPairedConstraint : instruction->externalInputConstraint;
                        reportWarning("%s %d-%d < mSline=%d [%d,%d]\n",
                                      instruction->opcodeInfo->mnemonic, instruction->position.getRow(),
                                      mfs,
                                      msl, scheduleTable->rangeStart, scheduleTable->rangeEnd);
                        const char *name = instruction->opcodeInfo ? instruction->opcodeInfo->mnemonic : "NO_BASE_INST";
                        reportWarning("%s: fixing mSline<oStart.. oStart=%d mSline=%d\n",
                                      name,
                                      oStart, msl);
                        msl = oStart;
                    }
                }
            }
        }
        if (msl >= scheduleTable->rangeEnd)
            return 0;
    } while (moved);

    limitDragDownStart(scheduleTable, msl);
    int newStart = msl;
    int ret = newStart - oStart;
    scheduleTable->rangeStart = newStart;
    newStart = scheduleTable->loopSize;
    msl = newStart;
    refreshDragDownRange(scheduleTable, newStart);
    return ret;
}
