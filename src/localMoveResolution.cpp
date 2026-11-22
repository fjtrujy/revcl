#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);


bool queueMoveForResolution(Block *blk, Instr *instruction)
{
    if (!instruction)
        return false;
    if (instruction->concreteMoveOpcode)
        unresolveMove(blk, instruction);
    instruction->nextMoveInstruction = blk->moveListHead;
    blk->pendingMoveCount++;
    blk->moveListHead = instruction;
    return true;
}

