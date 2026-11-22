#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

static inline void copyShort(short &destination, const short &from)
{
    destination = from;
}

int nextReservedInstructionKey = 1;

workTable *prepUta(Program *program, instStream *stream, int loopSize, char smart)
{
    workTable *scheduleTable = stream->getTable();
    if (loopSize) {
        if (!stream->canLoop()) {
            reportError("prepLockTable: can't loop, but loop size requested\n");
            return 0;
        }
        if (scheduleTable->codeModeValue == 3) {
            reportError("prepLockTable: loopSize requested, but mode is RAW\n");
            return 0;
        }
    }
    if (!optReset(program)) {
        reportError("failed to optReset cpu\n");
        return 0;
    }
    if (!scheduleTable->clearClones()) {
        reportError("failed to flush workClones table for %s\n", stream->streamName());
        return 0;
    }
    workTable *newWorkTable = new workTable;
    if (!newWorkTable) {
        reportError("iStream::prepUta - failed alloc table\n");
        exitAfterCleanup(-1);
    }
    if (!newWorkTable->init(2, stream->physicalInstructionBudget)) {
        reportError("failed to allocate large table\n");
        delete newWorkTable;
        return 0;
    }
    newWorkTable->ownerStream = stream;
    if (loopSize) {
        newWorkTable->minimumSafeLoopCount = stream->safeMinimumLoopCount;
        newWorkTable->setRange(loopSize);
    } else
        newWorkTable->setRange();
    setRangeFrom(newWorkTable);
    fatalReadBeforeReady = smart;
    newWorkTable->statistics.copyCountersFrom(&stream->schedulingStatistics);
    copyShort(newWorkTable->streamDependencyLength, scheduleTable->streamDependencyLength);
    copyShort(newWorkTable->streamLogicalLength, scheduleTable->streamLogicalLength);
    copyShort(newWorkTable->logicalInstructionLength, scheduleTable->logicalInstructionLength);
    int mode = (unsigned char)scheduleTable->allocationMode;
    newWorkTable->allocationMode = mode;
    newWorkTable->nextInsertionSerial = 0;
    newWorkTable->externalMinimumLength = scheduleTable->externalMinimumLength;
    regUseList &regs = scheduleTable->registerUses;
    for (regUse *registerUse = regs.head; registerUse; registerUse = registerUse->next) {
        if (!newWorkTable->queueRegisterUse(registerUse)) {
            reportError("failed to add a cloned regref in %s\n", stream->streamName());
            scheduleTable->dump();
            delete newWorkTable;
            return 0;
        }
    }
    for (Instr *currentInstruction = scheduleTable->linearInstructions.head; currentInstruction; currentInstruction = currentInstruction->linearLink.nextInstruction) {
        if (currentInstruction->getOwningStream() != stream) {
            reportError("owner not set or mismatch for prepUta .. istOwner = %x we are %x!\n",
                          currentInstruction->getOwningStream(), program);
            currentInstruction->dumpInstructionContext();
            exitAfterCleanup(-1);
        }
        currentInstruction->candidateAttemptLimit = 0;
        if (currentInstruction->isNonSynthetic() && !currentInstruction->isNop() && !newWorkTable->queueInstruction(currentInstruction)) {
            reportError("iStream: failed to queue instruction into table for opt\n");
            exitAfterCleanup(-1);
        }
    }
    if (!fixupCloneFdeps(newWorkTable)) {
        reportError("failed to fix fdeps to new table\n");
        return 0;
    }
    return newWorkTable;
}

