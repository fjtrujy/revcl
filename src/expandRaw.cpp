#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"

int expandRaw(workTable *scheduleTable)
{
    if (scheduleTable->codeModeValue != 5) {
        reportError("expand raw - table not raw!\n");
        return 0;
    }
    return scheduleTable->changeMode(3);
}

bool expandRawTables(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->workTablePtr) {
            reportError("work table not set in %s\n", stream->streamName());
            exitAfterCleanup(-1);
        }
        workTable *scheduleTable = stream->workTablePtr;
        if (scheduleTable->codeMode() == 3) {
            reportError("hit a raw table during expraw\n");
            return false;
        }
        if (scheduleTable->codeMode() == 5 && !scheduleTable->changeMode(3)) {
            reportError("processor failed to expand raw table\n");
            return false;
        }
    }
    return true;
}

