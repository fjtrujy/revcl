#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quietDiagnostics;
void resetRange();
int setRangeFrom(ScheduleTable *t);
char *dupString(const char *s);

bool buildPassThru(Program *program, InstructionStream *stream, DecodedLine *blocks, int count)
{
    reportInfo("BLD_PASS\n");
    if (count != 1) {
        reportError("cnum isn't one for pass thru case.. it's %d\n", count);
        return false;
    }
    stream->passThroughText = blocks->passThroughText;
    blocks->passThroughText = 0;
    ScheduleTable *uta = &stream->sourceTable;
    if (!uta->init(1, 0)) {
        reportError("failed to init uta\n");
        return false;
    }
    uta->ownerStream = stream;
    stream->branchType = 0;
    stream->containsBarrier = 1;
    stream->mergedBarrierFlag = 1;
    return true;
}

