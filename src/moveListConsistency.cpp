#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"

bool setRange(int start, int end, int lsz);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);
int unresolveMove(Block *blk, Instr *mv);

void func_0807f308(Block *blk)
{
    int n = 0;
    for (Instr *in = blk->moveListHead; in; in = in->unk_2c, n++)
        in->func_08054008(false);
    if (n != blk->unk_50) {
        reportError("oh no! consistency\n");
        abortAfterError();
    }
}
