#include "linearReduce_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(workTable *t);
bool moveLocks(Instr *i, cls08058718 *pos);
int func_0807e130(workTable *wt, Instr *in, cls08058718 *pos, bool upwards);
void func_0806ed78(Program *p, int a);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int func_0807dc44(workTable *wt, Instr *i, cls08058718 *pos);
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
        func_0804ad38("raw_PullUp it has loop\n");
        return -1;
    }
    setRangeFrom(wt);

    int moved;
    int mrl;
    Instr *lbdli = 0;
    Instr *br = wt->getBranch();
    if (br) {
        lbdli = wt->unk_2c[wt->unk_34 - 1].unk_00[1].unk_00;
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
                    func_0804ad38("rl not l!\n");
                    func_0804adf0(-1);
                }
                int e = earliestInsert(wt, in, 0);
                if (e < rl) {
                    if (!doLocalMove(wt, in, e, rl, 1)) {
                        func_0804ad38("failed to do local move for raw up\n");
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
                    func_0804ad38("mRline would be pushed after m_iEnd..%d, %d\n", mrl, wt->unk_34);
                    wt->dump();
                    func_0804ad38("faulting instruction is (mte=%d):\n",
                                  in->unk_13b6 < in->unk_13b4 ? in->unk_13b4 : in->unk_13b6);
                    in->leDump();
                    return -1;
                }
            }
        }
    } while (moved);

    if (mrl - wt->unk_30 + 1 < wt->unk_44)
        mrl = wt->unk_30 + wt->unk_44 - 1;

    if (br) {
        int l = mrl - 1;
        bool done = false;
        if (l < wt->unk_30)
            l = wt->unk_30;
        while (!done) {
            if (l >= wt->unk_34) {
                func_0804ad38("branch line overrun\n");
                return -1;
            }
            cls08058718 pos;
            pos.func_0805878c(br->unk_13e8.unk_02);
            pos.func_0805874c(l);
            if (func_0807dc44(wt, br, &pos)) {
                l++;
            } else {
                if (!wt->takeOut(br)) {
                    func_0804ad38("failed to take out instruction\n");
                    func_0804adf0(-1);
                }
                if (!moveLocks(br, &pos)) {
                    func_0804ad38("failed to move !\n");
                    func_0804adf0(-1);
                }
                if (!wt->putIn(br)) {
                    func_0804ad38("failed to put in back in\n");
                    func_0804adf0(-1);
                }
                l++;
                if (l > mrl)
                    mrl = l;
                if (lbdli) {
                    pos.func_0805874c(l);
                    pos.func_0805878c(lbdli->unk_13e8.unk_02);
                    if (l >= wt->unk_34) {
                        func_0804ad38("over run for bdli\n");
                        return -1;
                    }
                    if (func_0807dc44(wt, lbdli, &pos))
                        continue;
                    if (!wt->takeOut(lbdli)) {
                        func_0804ad38("failed to take out lbdli instruction\n");
                        func_0804adf0(-1);
                    }
                    if (!moveLocks(lbdli, &pos)) {
                        func_0804ad38("failed to move lbdli !\n");
                        func_0804adf0(-1);
                    }
                    if (!wt->putIn(lbdli)) {
                        func_0804ad38("failed to put in back in lbdli\n");
                        func_0804adf0(-1);
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
        func_0804ad38("table would grow ! mrl=%d..m_iE=%d\n", mrl, wt->unk_34);
        wt->dump();
        return -1;
    }
    wt->unk_34 = end;
    end = wt->unk_38;
    setPullUpRange(wt, end);
    return ret;
}
