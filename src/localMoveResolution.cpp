#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(ScheduleTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);


bool queueMoveForResolution(Block *block, Instr *instruction)
{
    if (!instruction)
        return false;
    if (instruction->concreteMoveOpcode)
        unresolveMove(block, instruction);
    instruction->nextMoveInstruction = block->moveListHead;
    block->pendingMoveCount++;
    block->moveListHead = instruction;
    return true;
}

