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

bool flowBlock::func_080843d0()
{
    for (int i = 0; i != 2; i++) {
        Instr *in = unk_0c[i];
        if (in) {
            char t = in->unk_13cd;
            if (t == 11 || t == 3 || t == 1)
                return true;
        }
    }
    return false;
}
