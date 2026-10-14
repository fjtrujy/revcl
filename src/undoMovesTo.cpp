#include "iInfo_shared.h"

bool undoMove(Block *, Instr *);

bool undoMovesTo(Block *blk, Instr *stop)
{
    bool done;
    Instr *i = blk->unk_98;
    do {
        Instr *prev = i->unk_24.unk_04;
        done = (i == stop);
        if (i->unk_1428) {
            if (!undoMove(blk, i)) {
                func_0804ad38("failed to undo move !\n");
                func_0804aeb0();
            }
        }
        i = prev;
    } while (!done);
    return true;
}
