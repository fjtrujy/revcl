#include "Instr.h"

void cls08058718::func_080586e0()
{
    unk_00 = -1;
    unk_04 = -1;
    unk_02 = -1;
    unk_03 = -1;
}

cls08058718::cls08058718()
{
    unk_00 = -1;
    unk_04 = -1;
    unk_02 = -1;
    unk_03 = -1;
}

cls08058718::~cls08058718()
{
}

void cls08058718::func_08058744()
{
}

void cls08058718::func_0805874c(short v)
{
    int g = g_080b336c;
    if (g == 0 || v < g) {
        unk_04 = v;
        unk_00 = v;
        unk_03 = 0;
    } else {
        unk_00 = v;
        unk_04 = v % g;
        unk_03 = v / g;
    }
}

void cls08058718::func_0805878c(char c)
{
    unk_02 = c;
}
