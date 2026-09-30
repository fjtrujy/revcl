#include "workTable.h"
#include "Instr.h"
#include "os.h"

bool setRange(int start, int end, int lsz);
bool func_0807cc6c(Instr *i, cls08058718 *pos);
int func_0807dc44(workTable *wt, Instr *i, cls08058718 *pos);

bool fiddleBDL(workTable *wt)
{
    setRange(wt->unk_30, wt->unk_34, wt->unk_38);
    bool skip = false;
    if (wt->unk_09 == 3 && (wt->unk_0a == 2 || wt->unk_0a == 4))
        skip = true;
    if (!skip) {
    int len = wt->unk_34 - wt->unk_30;
    if (len < wt->unk_44) {
        func_0804ad38("table already smaller than extminlength in fiddleBDL\n");
        return false;
    }
    if (len == wt->unk_44)
        return true;
    if (wt->unk_38)
        return true;

    // reduceExt
    Instr *br = wt->getBranch();
    len = wt->unk_34 - wt->unk_30;
    if (len <= 2 || !br)
        return true;
    short m = br->unk_13b2 < br->unk_13b0 ? br->unk_13b0 : br->unk_13b2;
    if (m > len - 3)
        return true;
    wtRow *last = wt->func_080724f0(wt->unk_34 - 1);
    if (!last) {
        func_0804ad38("reduceExt: failed to get last ie\n");
        return false;
    }
    if (last->func_08072304())
        return true;
    wtRow *brow = wt->func_080724f0(wt->unk_34 - 2);
    if (!brow) {
        func_0804ad38("reduceExt: failed to get branch ie\n");
        return false;
    }
    wtRow *prow = wt->func_080724f0(wt->unk_34 - 3);
    if (!prow) {
        func_0804ad38("reduceExt: failed to get prev ie\n");
        return false;
    }

    cls08058718 posP;
    cls08058718 posB;
    posP.setRow(wt->unk_34 - 3);
    posB.setRow(wt->unk_34 - 2);
    bool ok = true;
    if (br != brow->unk_00[1].unk_00) {
        func_0804ad38("the branch wasn't where I thought it was\n");
        return false;
    }
    Instr *prev = prow->unk_00[1].unk_00;
    if (func_0805449c(prev, br))
        return true;

    if (prev && !wt->takeOut(prev)) {
        func_0804ad38("failed to takeout in_p\n");
        return false;
    }
    if (func_0807dc44(wt, br, &posP))
        ok = false;
    if (prev) {
        if (!func_0807cc6c(prev, &posP)) {
            func_0804ad38("failed to undo takeout\n");
            return false;
        }
        if (!wt->putIn(prev)) {
            func_0804ad38("failed to takeout in_p\n");
            return false;
        }
    }
    if (!ok)
        return true;

    if (prev) {
        if (!wt->takeOut(br)) {
            func_0804ad38("failed to takeout in_b\n");
            return false;
        }
        if (func_0807dc44(wt, prev, &posB))
            ok = false;
        int m2 = prev->unk_13b6 < prev->unk_13b4 ? prev->unk_13b4 : prev->unk_13b6;
        if ((short)m2)
            ok = false;
        if (!func_0807cc6c(br, &posB)) {
            func_0804ad38("failed to undo branch\n");
            return false;
        }
        if (!wt->putIn(br)) {
            func_0804ad38("failed to put branch back in\n");
            return false;
        }
    }
    if (!ok)
        return true;

    if (!wt->takeOut(br)) {
        func_0804ad38("failed to takeout in_b\n");
        return false;
    }
    if (prev && !wt->takeOut(prev)) {
        func_0804ad38("failed to takeout in_p\n");
        return false;
    }
    if (!func_0807cc6c(br, &posP)) {
        func_0804ad38("after all that - failed to move branch\n");
        return false;
    }
    if (prev) {
        if (!func_0807cc6c(prev, &posB)) {
            func_0804ad38("after all that - failed to prev\n");
            return false;
        }
        if (!wt->putIn(prev)) {
            func_0804ad38("failed to put prev back in\n");
            return false;
        }
    }
    if (!wt->putIn(br)) {
        func_0804ad38("failed to put branch back in\n");
        return false;
    }
    wt->unk_34--;
    }
    return true;
}
