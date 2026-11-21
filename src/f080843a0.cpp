#include "flowBlock.h"
#include "Instr.h"

int func_080843a0(flowBlock *blocks, int count)
{
    int max = 0;
    for (int i = 0; i != count; i++)
        if (blocks[i].unk_08 > max)
            max = blocks[i].unk_08;
    return max;
}

