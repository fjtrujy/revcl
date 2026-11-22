#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"

int expandRaw(workTable *wt)
{
    if (wt->unk_09 != 5) {
        reportError("expand raw - table not raw!\n");
        return 0;
    }
    return wt->changeMode(3);
}

bool expandRawTables(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->unk_108) {
        if (!s->unk_218) {
            reportError("work table not set in %s\n", s->streamName());
            exitAfterCleanup(-1);
        }
        workTable *wt = s->unk_218;
        if (wt->codeMode() == 3) {
            reportError("hit a raw table during expraw\n");
            return false;
        }
        if (wt->codeMode() == 5 && !wt->changeMode(3)) {
            reportError("processor failed to expand raw table\n");
            return false;
        }
    }
    return true;
}

