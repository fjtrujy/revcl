#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"

int expandRaw(ScheduleTable *scheduleTable)
{
    if (scheduleTable->codeModeValue != VCL_CODE_RAW_PREEXPANDED) {
        reportError("expand raw - table not raw!\n");
        return 0;
    }
    return scheduleTable->changeMode(3);
}

bool expandRawTables(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->workTablePtr) {
            reportError("work table not set in %s\n", stream->streamName());
            exitAfterCleanup(-1);
        }
        ScheduleTable *scheduleTable = stream->workTablePtr;
        if (scheduleTable->codeMode() == VCL_CODE_RAW) {
            reportError("hit a raw table during expraw\n");
            return false;
        }
        if (scheduleTable->codeMode() == VCL_CODE_RAW_PREEXPANDED && !scheduleTable->changeMode(3)) {
            reportError("processor failed to expand raw table\n");
            return false;
        }
    }
    return true;
}

