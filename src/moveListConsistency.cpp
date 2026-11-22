#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

void verifyPendingMoveList(Block *block)
{
    int moveCount = 0;
    for (Instr *instruction = block->moveListHead; instruction; instruction = instruction->nextMoveInstruction, moveCount++)
        instruction->dumpOperandDiagnostics(false);
    if (moveCount != block->pendingMoveCount) {
        reportError("oh no! consistency\n");
        abortAfterError();
    }
}
