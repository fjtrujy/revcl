#include "wrangle_shared.h"

bool findInsertPosition(workTable *wt, Instr *in, int *row, int *pipe)
{
    bool both = false;
    if (in->unk_138c == 3)
        both = true;
    cls08058718 pos;
    if (both) {
        int p;
        int best;
        int r;
        for (;;) {
            best = 30000;
            p = *pipe;
            for (; p <= 1; p++) {
                r = *row;
                pos.func_0805878c(p);
                pos.func_0805874c(r);
                if (checkInsert(wt, in, &pos, &r) & ~0x100) {
                    if (r < best)
                        best = r;
                } else {
                    *row = r;
                    *pipe = p;
                    return true;
                }
            }
            if (best >= 30000)
                return false;
            if (best == *row) {
                func_0804ad38("min_nl was nl and all failed .. %d->%d\n", best, best);
                func_0804aeb0();
            }
            *row = best;
            *pipe = 0;
        }
    } else {
        *pipe = in->unk_138c != 1;
        pos.func_0805874c(*row);
        pos.func_0805878c(*pipe);
        while (checkInsert(wt, in, &pos, row) & ~0x100) {
            if (*row >= 30000)
                return false;
            pos.func_0805874c(*row);
        }
        if (*row >= 30000)
            return false;
    }
    return true;
}
