#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

void verifyPendingMoveList(Block *blk)
{
    int n = 0;
    for (Instr *instruction = blk->moveListHead; instruction; instruction = instruction->nextMoveInstruction, n++)
        instruction->dumpOperandDiagnostics(false);
    if (n != blk->pendingMoveCount) {
        reportError("oh no! consistency\n");
        abortAfterError();
    }
}
