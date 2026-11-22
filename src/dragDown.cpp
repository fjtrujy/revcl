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
                Instr *in = row->getEntry(p);
                if (in && !in->branchFlags) {
                    int rl = in->position.getRow();
                    short mte = in->unk_13b6 < in->unk_13b4 ? in->unk_13b4 : in->unk_13b6;
                    int mn = wt->rangeEnd - mte - 1;
                    if (rl != l) {
                        reportError("rl not l!\n");
                        exitAfterCleanup(-1);
                    }
                    int k;
                    Operand *op;
                    for (k = 0; (op = in->getOp(k)) != 0; k++) {
                        RegPoolEntry *reg = registerOperand(op, 2);
                        if (reg) {
                            for (int j = 0; j != reg->partCount; j++) {
                                OpSlot *sl = &op->parts[j];
                                if ((op->componentMask >> j) & 1) {
                                    int e = wt->rangeEnd - 1;
                                    if (sl->unk_2e)
                                        e = sl->unk_3e - sl->issueLatency;
                                    if (e < mn)
                                        mn = e;
                                }
                            }
                        }
                        if (registerOperand(op, 1) && op->unk_28)
                            mn = rl;
                    }
                    if (mn > rl) {
                        if (!doLocalMove(wt, in, mn, rl, -1)) {
                            reportError("failed to do local move for raw down\n");
                            return -1;
                        }
                        if (in->position.getRow() != rl)
                            moved++;
                    }
                }
                if (in) {
                    int s = in->position.getRow()
                          - (in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2);
                    if (s < msl)
                        msl = s;
                    if (msl < oStart) {
                        short mfs = in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2;
                        reportWarning("%s %d-%d < mSline=%d [%d,%d]\n",
                                      in->opcodeInfo->mnemonic, in->position.getRow(),
                                      mfs,
                                      msl, wt->rangeStart, wt->rangeEnd);
                        const char *name = in->opcodeInfo ? in->opcodeInfo->mnemonic : "NO_BASE_INST";
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
