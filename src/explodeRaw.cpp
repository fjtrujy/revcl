#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegisterEntry *collapseImmediateSourceRegister;
extern TypedValue *collapseImmediateValue;
extern int collapseImmediateOffset;
int decodeCollapsibleMoveImmediate(Instr *in);
int expandOneLoopIntoRegions(Program *p, InstructionStream *s);


bool explodeRaw(Program *program)
{
    InstructionStream *next;
    for (InstructionStream *currentStream = program->streamHead; currentStream; currentStream = next) {
        next = currentStream->nextStream;
        ScheduleTable *scheduleTable = currentStream->getTable();
        if (!scheduleTable) {
            reportError("floBuild - there's no work table!\n");
            return false;
        }
        if (!scheduleTable->getLoopSize())
            continue;
        if (scheduleTable->codeModeValue != VCL_CODE_RAW) {
            reportError("explodeRaw: a non raw stream has a loop size set\n");
            return false;
        }
        if (currentStream->knownMinimumLoopCount <= 0) {
            reportWarning("setting known min count to 1 for %s\n", currentStream->streamName());
            currentStream->knownMinimumLoopCount = 1;
        }
        if (currentStream->safeMinimumLoopCount < 0) {
            reportWarning("setting minimum safe count to 1000 for %s\n", currentStream->streamName());
            currentStream->safeMinimumLoopCount = 10000;
        }
        int expandedRegionCount = expandOneLoopIntoRegions(program, currentStream);
        if (expandedRegionCount < 0) {
            reportError("failed to construct epilogue/prologues\n");
            return false;
        }
        if (expandedRegionCount == 0) {
            reportError("no new streams added during build?\n");
            return false;
        }
        delete currentStream;
    }
    return true;
}

extern const char propagateValidMaskError[] = "valid bit mask from new register doesn't contain externally used bits for move %s<-%s\n";
extern const char propagateValidDetailError[] = "valid mask = %x ext bits on output operand = %x\n";
extern const char propagateTwoWritesError[] = "seem to have two writes on same line!\n";
extern const char propagateNormaliseError[] = "failed to normalise instruction\n";
