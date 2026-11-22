#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"

static inline bool extendForEnd_inl(workTable *scheduleTable)
{
    do {
        int i = scheduleTable->rangeEnd++;
        wtRow *workRow = scheduleTable->getRowByIndex(i);
        if (!workRow) {
            reportError("failed to get ie for end patch\n");
            return false;
        }
        workRow->clearEntries();
    } while (scheduleTable->rangeEnd - scheduleTable->rangeStart < 2);
    return true;
}

bool tidyProgramStreams(Program *program)
{
    instStream *stream;
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->workTablePtr) {
            reportError("work table not set in %s\n", stream->streamName());
            exitAfterCleanup(-1);
        }
        workTable *scheduleTable = stream->workTablePtr;
        switch (scheduleTable->codeModeValue) {
        case 1:
            reportError("work table code mode is uta..%s\n", stream->streamName());
            break;
        case 3:
        case 4:
            stream->emissionTable = scheduleTable;
            break;
        default:
            reportError("bad code mode for Tidy\n");
            return false;
        }
        stream->mergeIntoSuccessor = 1;
        if (stream->endType != 1)
            stream->mergeIntoSuccessor = 0;
        linkNode *link = stream->outputLinks.head;
        if (!link)
            stream->mergeIntoSuccessor = 0;
        while (link) {
            if (link->otherStream->endType != 3) {
                stream->mergeIntoSuccessor = 0;
                break;
            }
            link = link->next;
        }
    }
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!stream->mergeIntoSuccessor)
            continue;
        linkNode *n = stream->outputLinks.head;
        while (n) {
            linkNode *next = n->next;
            if (!program->unLinkStreams(stream, n->otherStream, n->linkType)) {
                reportError("failed disappear linkage\n");
                return false;
            }
            n = next;
        }
        Instr *instruction = stream->getBranch();
        if (instruction)
            instruction->tildePrefix = 1;
        stream->branchType = 0;
        stream->endType = 3;
        if (!stream->workTablePtr) {
            reportError("work table not set in %s\n", stream->streamName());
            exitAfterCleanup(-1);
        }
        workTable *wt = stream->workTablePtr;
        if (wt->codeModeValue == 4)
            continue;
        if (wt->getRangeStart() < 0)
            continue;
        if (wt->getPhysicalLen() > 1 && wt->hasSafeTrailingInstructions())
            continue;
        if (!extendForEnd_inl(wt)) {
            reportError("failed to extend for e\n");
            return false;
        }
    }
    return true;
}

bool extendForEnd(workTable *scheduleTable)
{
    do {
        int i = scheduleTable->rangeEnd++;
        wtRow *workRow = scheduleTable->getRowByIndex(i);
        if (!workRow) {
            reportError("failed to get ie for end patch\n");
            return false;
        }
        workRow->clearEntries();
    } while (scheduleTable->rangeEnd - scheduleTable->rangeStart < 2);
    return true;
}

extern const char fiddleBranchPosError[] = "the branch wasn't where I thought it was\n";
extern const char fiddleFailedPrevError[] = "after all that - failed to prev\n";
extern const char fiddleMoveBranchError[] = "after all that - failed to move branch\n";
extern const char fiddleGetPrevIeError[] = "reduceExt: failed to get prev ie\n";
extern const char fiddleGetBranchIeError[] = "reduceExt: failed to get branch ie\n";
extern const char fiddleGetLastIeError[] = "reduceExt: failed to get last ie\n";
extern const char fiddleSmallerError[] = "table already smaller than extminlength in fiddleBDL\n";
