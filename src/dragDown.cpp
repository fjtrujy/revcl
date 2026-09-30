#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

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
                    for (int k = 0; ; k++) {
                        Operand *op = in->getOp(k);
                        if (!op)
                            break;
                        RegPoolEntry *reg = op->outputReg();
                        if (reg) {
                            OpSlot *sl = op->unk_2c;
                            for (int j = 0; j != reg->unk_28; j++, sl++) {
                                if ((op->unk_07 >> j) & 1) {
                                    int e = wt->unk_34 - 1;
                                    if (sl->unk_2e)
                                        e = sl->unk_3e - sl->unk_18;
                                    if (e < mn)
                                        mn = e;
                                }
                            }
                        }
                        if (op->getReg() && op->unk_28)
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
                        func_0804ac7c("%s %d-%d < mSline=%d [%d,%d]\n",
                                      in->unk_34->unk_04, in->unk_13e8.getRow(),
                                      in->unk_13b2 < in->unk_13b0 ? in->unk_13b0 : in->unk_13b2,
                                      msl, wt->unk_30, wt->unk_34);
                        func_0804ac7c("%s: fixing mSline<oStart.. oStart=%d mSline=%d\n",
                                      in->unk_34 ? in->unk_34->unk_04 : "NO_BASE_INST",
                                      oStart, msl);
                        msl = oStart;
                    }
                }
            }
        }
        if (msl >= wt->unk_34)
            return 0;
    } while (moved);

    if (wt->unk_34 - msl < wt->unk_44)
        msl = wt->unk_34 - wt->unk_44;
    int ret = msl - oStart;
    wt->unk_30 = msl;
    setRange(wt->unk_30, wt->unk_34, wt->unk_38);
    return ret;
}

bool func_0807f2c4(Block *blk, Instr *mv)
{
    if (!mv)
        return false;
    if (mv->unk_142e)
        unresolveMove(blk, mv);
    mv->unk_2c = blk->unk_4c;
    blk->unk_50++;
    blk->unk_4c = mv;
    return true;
}

void func_0807f308(Block *blk)
{
    int n = 0;
    for (Instr *in = blk->unk_4c; in; in = in->unk_2c, n++)
        in->func_08054008(false);
    if (n != blk->unk_50) {
        func_0804ad38("oh no! consistency\n");
        func_0804aeb0();
    }
}
