#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

static inline void refreshDragDownRange(workTable *wt, int lsz)
{
    setRange(wt->rangeStart, wt->rangeEnd, lsz);
}

static inline void limitDragDownStart(workTable *wt, int &start)
{
    if (wt->rangeEnd - start < wt->externalMinimumLength)
        start = wt->rangeEnd - wt->externalMinimumLength;
}

int dragDown(workTable *wt)
{
    if (wt->rangeEnd == wt->rangeStart)
        return 0;
    wt->getBranch();
    if (wt->loopSize) {
        reportError("hit this debug assert - loopsize!=0\n");
        abortAfterError();
    }
    setRange(wt->rangeStart, wt->rangeEnd, wt->loopSize);

    int moved;
    int msl;
    int oStart = wt->rangeStart;
    do {
        moved = 0;
        msl = wt->rangeEnd;
        for (int l = wt->rangeEnd - 1; l >= wt->rangeStart; l--) {
            wtRow *row = wt->getRowByIndex(l);
            for (int p = 0; p != 2; p++) {
                Instr *instruction = row->getEntry(p);
                if (instruction && !instruction->branchFlags) {
                    int rl = instruction->position.getRow();
                    short mte = instruction->externalOutputConstraint < instruction->externalOutputPairedConstraint ? instruction->externalOutputPairedConstraint : instruction->externalOutputConstraint;
                    int mn = wt->rangeEnd - mte - 1;
                    if (rl != l) {
                        reportError("rl not l!\n");
                        exitAfterCleanup(-1);
                    }
                    int k;
                    Operand *operand;
                    for (k = 0; (operand = instruction->getOp(k)) != 0; k++) {
                        RegPoolEntry *reg = registerOperand(operand, 2);
                        if (reg) {
                            for (int j = 0; j != reg->partCount; j++) {
                                OpSlot *sl = &operand->parts[j];
                                if ((operand->componentMask >> j) & 1) {
                                    int e = wt->rangeEnd - 1;
                                    if (sl->scheduledConsumerCount)
                                        e = sl->earliestConsumerFreeCycle - sl->issueLatency;
                                    if (e < mn)
                                        mn = e;
                                }
                            }
                        }
                        if (registerOperand(operand, 1) && operand->reductionHazardMask)
                            mn = rl;
                    }
                    if (mn > rl) {
                        if (!doLocalMove(wt, instruction, mn, rl, -1)) {
                            reportError("failed to do local move for raw down\n");
                            return -1;
                        }
                        if (instruction->position.getRow() != rl)
                            moved++;
                    }
                }
                if (instruction) {
                    int s = instruction->position.getRow()
                          - (instruction->externalInputConstraint < instruction->externalInputPairedConstraint ? instruction->externalInputPairedConstraint : instruction->externalInputConstraint);
                    if (s < msl)
                        msl = s;
                    if (msl < oStart) {
                        short mfs = instruction->externalInputConstraint < instruction->externalInputPairedConstraint ? instruction->externalInputPairedConstraint : instruction->externalInputConstraint;
                        reportWarning("%s %d-%d < mSline=%d [%d,%d]\n",
                                      instruction->opcodeInfo->mnemonic, instruction->position.getRow(),
                                      mfs,
                                      msl, wt->rangeStart, wt->rangeEnd);
                        const char *name = instruction->opcodeInfo ? instruction->opcodeInfo->mnemonic : "NO_BASE_INST";
                        reportWarning("%s: fixing mSline<oStart.. oStart=%d mSline=%d\n",
                                      name,
                                      oStart, msl);
                        msl = oStart;
                    }
                }
            }
        }
        if (msl >= wt->rangeEnd)
            return 0;
    } while (moved);

    limitDragDownStart(wt, msl);
    int newStart = msl;
    int ret = newStart - oStart;
    wt->rangeStart = newStart;
    newStart = wt->loopSize;
    msl = newStart;
    refreshDragDownRange(wt, newStart);
    return ret;
}
