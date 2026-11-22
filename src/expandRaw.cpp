#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"

int expandRaw(workTable *wt)
{
    if (wt->codeModeValue != 5) {
        reportError("expand raw - table not raw!\n");
        return 0;
    }
    return wt->changeMode(3);
}

bool expandRawTables(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->workTablePtr) {
            reportError("work table not set in %s\n", stream->streamName());
            exitAfterCleanup(-1);
        }
        workTable *wt = stream->workTablePtr;
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

