#pragma implementation "tPair.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"

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

int func_0805879c()
{
    return 1;
}

void resetRange()
{
    g_080b336c = -1;
    g_080b3370 = -2;
    g_080b3374 = -3;
}

bool setRange(int start, int end, int lsz);

int setRangeFrom(workTable *t)
{
    if (!t) {
        func_0804ad38("setRangeFrom - null ptr\n");
        return 0;
    }
    if (!t->unk_2c) {
        func_0804ad38("setRangeFrom - can't set range .. there's no table\n");
        return 0;
    }
    if (t->codeMode() != 3 && t->codeMode() != 2 && t->codeMode() != 5) {
        func_0804ad38("can't set range from a non raw/raw_preexp/mid table\n");
        return 0;
    }
    return setRange(t->func_0807411c(), t->func_080741c0(), t->func_080741b4());
}

bool setRange(int start, int end, int lsz)
{
    if (lsz < 0) {
        func_0804ad38("-ve l\n");
        func_0804adf0(-1);
        return false;
    }
    if (start < 0) {
        func_0804ad38("bad range, 1\n");
        func_0804adf0(-1);
        return false;
    }
    if (end >= 0) {
        if (lsz > 0 && end - start != lsz) {
            func_0804ad38("start/end not match lsz\n");
            func_0804adf0(-1);
            return false;
        }
    } else if (lsz != 0) {
        func_0804ad38("lsz set - but e<0\n");
        func_0804adf0(-1);
        return false;
    }
    g_080b3370 = start;
    g_080b3374 = end;
    g_080b336c = lsz;
    return true;
}
