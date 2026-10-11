#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

static inline void refreshDragDownRange(workTable *wt, int lsz)
{
    setRange(wt->unk_30, wt->unk_34, lsz);
}

static inline void limitDragDownStart(workTable *wt, int &start)
{
    if (wt->unk_34 - start < wt->unk_44)
        start = wt->unk_34 - wt->unk_44;
}

int dragDown(workTable *wt)
{
    if (wt->unk_34 == wt->unk_30)
        return 0;
    wt->getBranch();
    if (wt->unk_38) {
        func_0804ad38("hit this debug assert - loopsize!=0\n");
        func_0804aeb0();
    }
    setRange(wt->unk_30, wt->unk_34, wt->unk_38);

    int moved;
    int msl;
    int oStart = wt->unk_30;
    do {
        moved = 0;
        msl = wt->unk_34;
        for (int l = wt->unk_34 - 1; l >= wt->unk_30; l--) {
            wtRow *row = wt->func_080724f0(l);
            for (int p = 0; p != 2; p++) {
                Instr *in = row->getEntry(p);
                if (in && !in->unk_1388) {
                    int rl = in->unk_13e8.getRow();
                    short mte = in->unk_13b6 < in->unk_13b4 ? in->unk_13b4 : in->unk_13b6;
                    int mn = wt->unk_34 - mte - 1;
                    if (rl != l) {
                        func_0804ad38("rl not l!\n");
                        func_0804adf0(-1);
                    }
                    int k;
                    Operand *op;
                    for (k = 0; (op = in->getOp(k)) != 0; k++) {
                        RegPoolEntry *reg = registerOperand(op, 2);
                        if (reg) {
                            for (int j = 0; j != reg->unk_28; j++) {
                                OpSlot *sl = &op->unk_2c[j];
                                if ((op->unk_07 >> j) & 1) {
                                    int e = wt->unk_34 - 1;
                                    if (sl->unk_2e)
                                        e = sl->unk_3e - sl->unk_18;
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
                            func_0804ad38("failed to do local move for raw down\n");
                            return -1;
                        }
                        if (in->unk_13e8.getRow() != rl)
                            moved++;
                    }
                }
                if (in) {
                    int s = in->unk_13e8.getRow()
                          - (in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2);
                    if (s < msl)
                        msl = s;
                    if (msl < oStart) {
                        short mfs = in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2;
                        func_0804ac7c("%s %d-%d < mSline=%d [%d,%d]\n",
                                      in->unk_34->unk_04, in->unk_13e8.getRow(),
                                      mfs,
                                      msl, wt->unk_30, wt->unk_34);
                        const char *name = in->unk_34 ? in->unk_34->unk_04 : "NO_BASE_INST";
                        func_0804ac7c("%s: fixing mSline<oStart.. oStart=%d mSline=%d\n",
                                      name,
                                      oStart, msl);
                        msl = oStart;
                    }
                }
            }
        }
        if (msl >= wt->unk_34)
            return 0;
    } while (moved);

    limitDragDownStart(wt, msl);
    int newStart = msl;
    int ret = newStart - oStart;
    wt->unk_30 = newStart;
    newStart = wt->unk_38;
    msl = newStart;
    refreshDragDownRange(wt, newStart);
    return ret;
}
