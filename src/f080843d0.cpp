#include "flowBlock.h"
#include "Instr.h"

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
