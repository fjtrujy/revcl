#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

static inline void copyShort(short &to, const short &from)
{
    to = from;
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
    workTable *nt = new workTable;
    if (!nt) {
        reportError("iStream::prepUta - failed alloc table\n");
        exitAfterCleanup(-1);
    }
    if (!nt->init(2, stream->physicalInstructionBudget)) {
        reportError("failed to allocate large table\n");
        delete nt;
        return 0;
    }
    nt->ownerStream = stream;
    if (loopSize) {
        nt->minimumSafeLoopCount = stream->safeMinimumLoopCount;
        nt->setRange(loopSize);
    } else
        nt->setRange();
    setRangeFrom(nt);
    fatalReadBeforeReady = smart;
    nt->statistics.copyCountersFrom(&stream->schedulingStatistics);
    copyShort(nt->streamDependencyLength, scheduleTable->streamDependencyLength);
    copyShort(nt->streamLogicalLength, scheduleTable->streamLogicalLength);
    copyShort(nt->logicalInstructionLength, scheduleTable->logicalInstructionLength);
    int mode = (unsigned char)scheduleTable->allocationMode;
    nt->allocationMode = mode;
    nt->nextInsertionSerial = 0;
    nt->externalMinimumLength = scheduleTable->externalMinimumLength;
    regUseList &regs = scheduleTable->registerUses;
    for (regUse *registerUse = regs.head; registerUse; registerUse = registerUse->next) {
        if (!nt->qRegRef(registerUse)) {
            reportError("failed to add a cloned regref in %s\n", stream->streamName());
            scheduleTable->dump();
            delete nt;
            return 0;
        }
    }
    for (Instr *in = scheduleTable->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        if (in->getOwningStream() != stream) {
            reportError("owner not set or mismatch for prepUta .. istOwner = %x we are %x!\n",
                          in->getOwningStream(), program);
            in->leDump();
            exitAfterCleanup(-1);
        }
        in->candidateAttemptLimit = 0;
        if (in->isNonSynthetic() && !in->isNop() && !nt->qInstruction(in)) {
            reportError("iStream: failed to queue instruction into table for opt\n");
            exitAfterCleanup(-1);
        }
    }
    if (!fixupCloneFdeps(nt)) {
        reportError("failed to fix fdeps to new table\n");
        return 0;
    }
    return nt;
}

