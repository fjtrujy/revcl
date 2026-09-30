#include "flowBlock.h"
#include "Instr.h"

void flowBlock::func_08084220()
{
    unk_04 = 0;
    unk_05 = 0;
    unk_0a = 0;
    unk_06 = 0;
    unk_08 = -1;
    unk_14.func_0804b1b0();
    unk_00 = 0;
    unk_0c[0] = 0;
    unk_0c[1] = 0;
}

void flowBlock::func_08084268(int index)
{
    mList *labels = &unk_14;
    if (labels->unk_08)
        labels->func_0804b534();
    for (int i = 0; i != 2; i++) {
        if (unk_0c[i]) {
            Instr *in = unk_0c[i];
            in->func_08054008(false);
        }
    }
}

void func_080842b4(flowBlock *blocks, int count)
{
    if (blocks)
        for (int i = 0; i != count; i++)
            blocks[i].func_08084268(i);
}

void flowBlock::func_080842f0()
{
    unk_14.func_0804b1b0();
    if (unk_00)
        delete[] unk_00;
    unk_00 = 0;
    for (int i = 0; i != 2; i++) {
        if (unk_0c[i])
            delete unk_0c[i];
        unk_0c[i] = 0;
    }
}

flowBlock *func_0808435c(flowBlock *blocks, int count, const char *name)
{
    for (int i = 0; i != count; i++)
        if (blocks[i].unk_14.func_0804b284(name))
            return &blocks[i];
    return 0;
}

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
