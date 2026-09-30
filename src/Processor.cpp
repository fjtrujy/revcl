#include "Processor.h"

Processor::Processor()
{
    unk_34 = 0;
    g_080b33a8 = 0;
    unk_00 = 0;
    g_080de6c0 = 0;
    unk_1c = "Nameless";
    unk_18 = "";
    unk_64 = 0;
    unk_65 = 0;
    unk_44 = -1;
    unk_38 = 0;
    unk_60 = 0;
    unk_3d = 0;
    unk_3e = 0;
    for (int i = 0; i != 12; i++)
        g_080de6e0[i] = 0;
    for (int i = 0; i != 3; i++)
        g_080de714[i] = 0;
}

Processor::~Processor()
{
}
