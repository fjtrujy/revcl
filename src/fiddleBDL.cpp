#include "workTable.h"
#include "Instr.h"
#include "os.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, cls08058718 *pos);
int func_0807dc44(workTable *wt, Instr *i, cls08058718 *pos);

static inline bool isRawMode(workTable *wt)
{
    if (wt->codeMode() == 3)
        return true;
    return false;
}

static inline bool noReduce(workTable *wt)
{
    return isRawMode(wt) && (wt->unk_0a == 2 || wt->unk_0a == 4);
}

bool fiddleBDL(workTable *wt)
{
    setRange(wt->unk_30, wt->unk_34, wt->unk_38);
    if (noReduce(wt)) {
        return true;
    } else {
        int len = wt->unk_34 - wt->unk_30;
        if (len < wt->unk_44) {
            func_0804ad38("table already smaller than extminlength in fiddleBDL\n");
            return false;
        }
        if (len == wt->unk_44)
            return true;
        if (wt->unk_38)
            goto done;

        // reduceExt
        Instr *br = wt->getBranch();
        int extlen = wt->unk_34 - wt->unk_30;
        if (extlen <= 2 || !br)
            goto done;
        short m = br->unk_13b2 < br->unk_13b0 ? br->unk_13b0 : br->unk_13b2;
        if (m > extlen - 3)
            goto done;
        wtRow *last = wt->func_080724f0(wt->unk_34 - 1);
        if (!last) {
            func_0804ad38("reduceExt: failed to get last ie\n");
            return false;
        }
        if (last->func_08072304())
            goto done;
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
        {
            cls08058718 posB;
            short row = wt->unk_34;
            row -= 3;
            posP.setRow(row);
            row = wt->unk_34;
            row -= 2;
            posB.setRow(row);
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
                if (!moveLocks(prev, &posP)) {
                    func_0804ad38("failed to undo takeout\n");
                    return false;
                }
                if (!wt->putIn(prev)) {
                    func_0804ad38("failed to takeout in_p\n");
                    return false;
                }
            }
            if (!ok)
                goto success;

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
                if (!moveLocks(br, &posB)) {
                    func_0804ad38("failed to undo branch\n");
                    return false;
                }
                if (!wt->putIn(br)) {
                    func_0804ad38("failed to put branch back in\n");
                    return false;
                }
            }
            if (!ok)
                goto success;

            if (!wt->takeOut(br)) {
                func_0804ad38("failed to takeout in_b\n");
                return false;
            }
            if (prev && !wt->takeOut(prev)) {
                func_0804ad38("failed to takeout in_p\n");
                return false;
            }
            if (!moveLocks(br, &posP)) {
                func_0804ad38("after all that - failed to move branch\n");
                return false;
            }
            if (prev) {
                if (!moveLocks(prev, &posB)) {
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
success:
            ;
        }
    }
done:
    return true;
}
