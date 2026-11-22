#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

bool buildPassThru(Program *prog, instStream *s, DecodedLine *blocks, int count)
{
    reportInfo("BLD_PASS\n");
    if (count != 1) {
        reportError("cnum isn't one for pass thru case.. it's %d\n", count);
        return false;
    }
    s->passThroughText = blocks->passThroughText;
    blocks->passThroughText = 0;
    workTable *uta = &s->sourceTable;
    if (!uta->init(1, 0)) {
        reportError("failed to init uta\n");
        return false;
    }
    uta->unk_10 = s;
    s->branchType = 0;
    s->unk_2d6 = 1;
    s->unk_2d7 = 1;
    return true;
}

