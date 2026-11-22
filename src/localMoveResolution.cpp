#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);


bool func_0807f2c4(Block *blk, Instr *mv)
{
    if (!mv)
        return false;
    if (mv->concreteMoveOpcode)
        unresolveMove(blk, mv);
    mv->nextMoveInstruction = blk->moveListHead;
    blk->unk_50++;
    blk->moveListHead = mv;
    return true;
}

