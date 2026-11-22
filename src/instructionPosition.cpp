#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"

int activeLoopSize = -1;
int activeSchedulingRangeStart = -2;
int activeSchedulingRangeEnd = -3;

void cls08058718::clearInstructionPosition()
{
    linearRow = -1;
    loopRow = -1;
    pipelineIndex = -1;
    loopIteration = -1;
}

cls08058718::cls08058718()
{
    linearRow = -1;
    loopRow = -1;
    pipelineIndex = -1;
    loopIteration = -1;
}

cls08058718::~cls08058718()
{
}

void cls08058718::noopInstructionPositionHook()
{
}

void cls08058718::setFromLinearRow(short v)
{
    int g = activeLoopSize;
    if (g == 0 || v < g) {
        loopRow = v;
        linearRow = v;
        loopIteration = 0;
    } else {
        linearRow = v;
        loopRow = v % g;
        loopIteration = v / g;
    }
}

void cls08058718::setInstructionPipe(char c)
{
    pipelineIndex = c;
}

int alwaysOnePositionCheck()
{
    return 1;
}

void resetRange()
{
    activeLoopSize = -1;
    activeSchedulingRangeStart = -2;
    activeSchedulingRangeEnd = -3;
}

bool setRange(int start, int end, int lsz);

int setRangeFrom(workTable *t)
{
    if (!t) {
        reportError("setRangeFrom - null ptr\n");
        return 0;
    }
    if (!t->rows) {
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
    activeSchedulingRangeStart = start;
    activeSchedulingRangeEnd = end;
    activeLoopSize = lsz;
    return true;
}
