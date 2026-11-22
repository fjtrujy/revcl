#include "linearReduce_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(workTable *t);
bool moveLocks(Instr *i, InstructionPosition *pos);
int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
void func_0806ed78(Program *p, int a);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int func_0807dc44(workTable *wt, Instr *i, InstructionPosition *pos);
bool setRange(int start, int end, int lsz);
int pullUp(workTable *wt);
int dragDown(workTable *wt);

static inline bool setPullUpRange(workTable *wt, int lsz)
{
    return setRange(wt->unk_30, wt->unk_34, lsz);
}

int pullUp(workTable *wt)
{
    if (wt->unk_34 == wt->unk_30)
        return 0;
    if (wt->unk_38) {
        reportError("raw_PullUp it has loop\n");
        return -1;
    }
    setRangeFrom(wt);

    int moved;
    int mrl;
    Instr *lbdli = 0;
    Instr *br = wt->getBranch();
    if (br) {
        lbdli = wt->unk_2c[wt->unk_34 - 1].entries[1].unk_00;
        if (lbdli)
            lbdli = func_0805449c(lbdli, br) ? lbdli : 0;
    }

    do {
        moved = 0;
        mrl = wt->unk_30;
        for (int l = wt->unk_30; l != wt->unk_34; l++) {
            wtRow *row = wt->func_080724f0(l);
            for (int p = 0; p != 2; p++) {
                Instr *in = row->getEntry(p);
                if (!in || in == br || in == lbdli)
                    continue;
                int rl = in->unk_13e8.getRow();
                if (rl != l) {
                    reportError("rl not l!\n");
                    exitAfterCleanup(-1);
                }
                int e = earliestInsert(wt, in, 0);
                if (e < rl) {
                    if (!doLocalMove(wt, in, e, rl, 1)) {
                        reportError("failed to do local move for raw up\n");
                        return -1;
                    }
                    if (in->unk_13e8.getRow() != rl)
                        moved++;
                }
                short mte = in->unk_13b6 < in->unk_13b4 ? in->unk_13b4 : in->unk_13b6;
                int end = in->unk_13e8.getRow() + mte;
                if (end > mrl)
                    mrl = end;
                if (mrl >= wt->unk_34) {
                    reportError("mRline would be pushed after m_iEnd..%d, %d\n", mrl, wt->unk_34);
                    wt->dump();
                    int diagnosticMte = in->unk_13b6 < in->unk_13b4 ? in->unk_13b4 : in->unk_13b6;
                    reportError("faulting instruction is (mte=%d):\n", diagnosticMte);
                    in->leDump();
                    return -1;
                }
            }
        }
    } while (moved);

    if (mrl - wt->unk_30 + 1 < wt->unk_44)
        mrl = wt->unk_30 + wt->unk_44 - 1;

    int branchPresent = br != 0;
    if (branchPresent) {
        int l = mrl - 1;
        bool done = false;
        if (l < wt->unk_30)
            l = wt->unk_30;
        while (!done) {
            if (l >= wt->unk_34) {
                reportError("branch line overrun\n");
                return -1;
            }
            InstructionPosition pos;
            pos.func_0805878c(br->unk_13e8.unk_02);
            pos.func_0805874c(l);
            if (func_0807dc44(wt, br, &pos)) {
                l++;
            } else {
                if (!wt->takeOut(br)) {
                    reportError("failed to take out instruction\n");
                    exitAfterCleanup(-1);
                }
                if (!moveLocks(br, &pos)) {
                    reportError("failed to move !\n");
                    exitAfterCleanup(-1);
                }
                if (!wt->putIn(br)) {
                    reportError("failed to put in back in\n");
                    exitAfterCleanup(-1);
                }
                l++;
                if (l > mrl)
                    mrl = l;
                if (lbdli) {
                    pos.func_0805874c(l);
                    pos.func_0805878c(lbdli->unk_13e8.unk_02);
                    if (l >= wt->unk_34) {
                        reportError("over run for bdli\n");
                        return -1;
                    }
                    if (func_0807dc44(wt, lbdli, &pos))
                        continue;
                    if (!wt->takeOut(lbdli)) {
                        reportError("failed to take out lbdli instruction\n");
                        exitAfterCleanup(-1);
                    }
                    if (!moveLocks(lbdli, &pos)) {
                        reportError("failed to move lbdli !\n");
                        exitAfterCleanup(-1);
                    }
                    if (!wt->putIn(lbdli)) {
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

    int ret = wt->unk_34 - mrl - 1;
    int end = mrl + 1;
    if (end > wt->unk_34) {
        reportError("table would grow ! mrl=%d..m_iE=%d\n", mrl, wt->unk_34);
        wt->dump();
        return -1;
    }
    wt->unk_34 = end;
    end = wt->unk_38;
    setPullUpRange(wt, end);
    return ret;
}
