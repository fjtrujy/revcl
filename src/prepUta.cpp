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

workTable *prepUta(Program *p, instStream *s, int loopSize, char smart)
{
    workTable *wt = s->getTable();
    if (loopSize) {
        if (!s->canLoop()) {
            reportError("prepLockTable: can't loop, but loop size requested\n");
            return 0;
        }
        if (wt->codeModeValue == 3) {
            reportError("prepLockTable: loopSize requested, but mode is RAW\n");
            return 0;
        }
    }
    if (!optReset(p)) {
        reportError("failed to optReset cpu\n");
        return 0;
    }
    if (!wt->clearClones()) {
        reportError("failed to flush workClones table for %s\n", s->streamName());
        return 0;
    }
    workTable *nt = new workTable;
    if (!nt) {
        reportError("iStream::prepUta - failed alloc table\n");
        exitAfterCleanup(-1);
    }
    if (!nt->init(2, s->physicalInstructionBudget)) {
        reportError("failed to allocate large table\n");
        delete nt;
        return 0;
    }
    nt->ownerStream = s;
    if (loopSize) {
        nt->minimumSafeLoopCount = s->safeMinimumLoopCount;
        nt->setRange(loopSize);
    } else
        nt->setRange();
    setRangeFrom(nt);
    fatalReadBeforeReady = smart;
    nt->statistics.copyCountersFrom(&s->schedulingStatistics);
    copyShort(nt->streamDependencyLength, wt->streamDependencyLength);
    copyShort(nt->streamLogicalLength, wt->streamLogicalLength);
    copyShort(nt->logicalInstructionLength, wt->logicalInstructionLength);
    int mode = (unsigned char)wt->allocationMode;
    nt->allocationMode = mode;
    nt->nextInsertionSerial = 0;
    nt->externalMinimumLength = wt->externalMinimumLength;
    regUseList &regs = wt->registerUses;
    for (regUse *ru = regs.head; ru; ru = ru->next) {
        if (!nt->qRegRef(ru)) {
            reportError("failed to add a cloned regref in %s\n", s->streamName());
            wt->dump();
            delete nt;
            return 0;
        }
    }
    for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        if (in->getOwningStream() != s) {
            reportError("owner not set or mismatch for prepUta .. istOwner = %x we are %x!\n",
                          in->getOwningStream(), p);
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

