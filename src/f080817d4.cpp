#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int timeout_080b3010;


bool func_080817d4(workTable *wt, wrangleCand *c)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *in = wt->unk_4c; in; in = in->unk_2c) {
        if (in->unk_13e4 < min) {
            best = in;
            min = in->unk_13e4;
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = best->unk_141e + best->unk_141c;
    if (!best->unk_1428 && c->unk_04 < wt->unk_54.getRow())
        c->unk_04 = wt->unk_54.getRow();
    c->unk_06 = best->unk_1418 & 1;
    return true;
}

bool func_0808184c(workTable *wt, wrangleCand *c)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *in = wt->unk_4c; in; in = in->unk_2c) {
        if (in->unk_13e4 < min) {
            best = in;
            min = in->unk_13e4;
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = best->unk_141e + best->unk_141c;
    if (!best->unk_1428 && c->unk_04 < wt->unk_54.getRow() - 64)
        c->unk_04 = wt->unk_54.getRow() - 64;
    if (c->unk_04 < 0)
        c->unk_04 = 0;
    c->unk_06 = best->unk_1418 & 1;
    return true;
}
