#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"

int g_080b336c = -1;
int g_080b3370 = -2;
int g_080b3374 = -3;

void cls08058718::clearInstructionPosition()
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

void cls08058718::setFromLinearRow(short v)
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

void cls08058718::setInstructionPipe(char c)
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
        reportError("setRangeFrom - null ptr\n");
        return 0;
    }
    if (!t->unk_2c) {
        reportError("setRangeFrom - can't set range .. there's no table\n");
        return 0;
    }
    if (t->codeMode() != 3 && t->codeMode() != 2 && t->codeMode() != 5) {
        reportError("can't set range from a non raw/raw_preexp/mid table\n");
        return 0;
    }
    return setRange(t->getRangeStart(), t->getRangeEnd(), t->getLoopSize());
}

bool setRange(int start, int end, int lsz)
{
    if (lsz < 0) {
        reportError("-ve l\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (start < 0 && end >= 0) {
        reportError("bad range, 1\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (start < 0) {
        reportError("bad range, 1\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (end >= 0) {
        if (lsz > 0 && end - start != lsz) {
            reportError("start/end not match lsz\n");
            exitAfterCleanup(-1);
            return false;
        }
    } else if (lsz != 0) {
        reportError("lsz set - but e<0\n");
        exitAfterCleanup(-1);
        return false;
    }
    g_080b3370 = start;
    g_080b3374 = end;
    g_080b336c = lsz;
    return true;
}
