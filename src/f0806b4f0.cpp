#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

bool detectMove(Instr *in, int *opc, unsigned char *mask, int *dstOp, int *srcOp);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);


bool func_0806b4f0(Program *p)
{
    return true;
}
