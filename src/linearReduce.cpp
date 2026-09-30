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

static inline bool isPipeMode(workTable *wt)
{
    return wt->unk_09 == 3 && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

bool func_0807e740(Program *p, workTable *wt, unsigned char up, unsigned char down)
{
    if (!isPipeMode(wt))
        return true;
    if (wt->func_080741b4())
        return true;

    int passes = up != 0;
    if (down)
        passes++;

    wt->func_0807411c();
    wt->func_080741c0();
    setRangeFrom(wt);

    int count = passes;
    while (count) {
        count--;
        if (up) {
            int r = pullUp(wt);
            if (r < 0) {
                func_0804ad38("failed to pull up\n");
                func_0806ed78(p, 0);
                return false;
            }
            if (r)
                count = passes;
        }
        if (down) {
            int r = dragDown(wt);
            if (r < 0) {
                func_0804ad38("failed drag down\n");
                return false;
            }
            if (r)
                count = passes;
        }
    }
    return true;
}

bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl)
{
    cls08058718 pos;
    bool found = false;
    bool upwards;

    if (dl == -1) {
        if (s_test < e_test) {
            func_0804ad38("do_localMove - s_test<e_test\n");
            return false;
        }
        upwards = true;
    } else if (dl == 1) {
        if (s_test > e_test) {
            func_0804ad38("do_localMove + s_test>e_test\n");
            return false;
        }
        upwards = false;
    } else {
        func_0804ad38("bad dl for doLocalMove\t .. %d\n", dl);
        return false;
    }

    for (; s_test != e_test; s_test += dl) {
        pos.func_0805874c(s_test);
        if (!func_0807e130(wt, in, &pos, upwards)) {
            found = true;
            break;
        }
    }

    if (found) {
        if (!wt->takeOut(in)) {
            func_0804ad38("failed to take out instruction\n");
            return false;
        }
        if (!moveLocks(in, &pos)) {
            func_0804ad38("failed to move !\n");
            return false;
        }
        if (!wt->putIn(in)) {
            func_0804ad38("failed to put in back in\n");
            return false;
        }
    }
    return true;
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
    if (mrl + 1 > wt->unk_34) {
        func_0804ad38("table would grow ! mrl=%d..m_iE=%d\n", mrl, wt->unk_34);
        wt->dump();
        return -1;
    }
    wt->unk_34 = ++mrl;
    setRange(wt->unk_30, wt->unk_34, wt->unk_38);
    return ret;
}
