#include "os.h"
#include "backTrace_shared.h"

bool resetStreamPathSearch(InstructionStream *n);

static inline regUseSlot *registerPart(RegisterUse *registerUse, int part)
{
    return &registerUse->slots[part];
}

bool initializeRegisterBacktraceLocation(InstructionStream *stream, RegisterEntry *registerEntry, int part)
{
    ScheduleTable *scheduleTable = stream->getTable();
    if (!scheduleTable) {
        reportError("failed to get work table\n");
        return 0;
    }
    RegisterUse *registerUse = scheduleTable->registerUses.findByRegister(registerEntry);
    stream->pendingHazardBroadcast = 0;
    stream->hazardFlag = 0;
    stream->nextResolvedStream = 0;
    stream->backtraceRegisterUse = registerUse;
    if (registerUse && ((registerUse->outputMask >> part) & 1)) {
        if (scheduleTable->codeMode() == VCL_CODE_UTA || scheduleTable->codeMode() == VCL_CODE_SYNTHETIC) {
            stream->outputLocationCount = 1;
            stream->earliestOutputLocation = 0;
            stream->latestOutputLocation = 0;
        } else {
            int count = registerUse->slots[part].outputLocations.count;
            stream->outputLocationCount = count;
            OperandLocation *head = registerUse->slots[part].outputLocations.head;
            if (!head) {
                reportError("visible outputs has no head for l_init.. but it should!\n");
                return 0;
            }
            int earliestCycle = head->cycleOffset;
            stream->earliestOutputLocation = earliestCycle;
            OperandLocation *last = registerPart(registerUse, part)->findPreviousOutputAtPosition(0, true);
            if (!last) {
                reportError("visible outputs had nothing visible at infinity\n");
                return 0;
            }
            int latestCycle = last->cycleOffset;
            stream->latestOutputLocation = latestCycle;
            if (stream->earliestOutputLocation > stream->latestOutputLocation) {
                reportError("minimal writeVal was > maximal writeVal\n");
                return 0;
            }
        }
    } else {
        stream->outputLocationCount = 0;
        stream->earliestOutputLocation = -1;
        stream->latestOutputLocation = -2;
    }
    return resetStreamPathSearch(stream);
}

bool resetStreamPathSearch(InstructionStream *stream)
{
    stream->visitState = 0;
    for (StreamLinkNode *link = stream->inputLinks.head; link; link = link->next)
        link->includedInPathTraversal = link->linkType != 3 && link->linkType != 4;
    stream->minimumPathLength = -1;
    stream->shortestPathPredecessor = 0;
    stream->maximumPathLength = -1;
    stream->longestPathPredecessor = 0;
    stream->nextSourceStream = 0;
    stream->nextMatchedStream = 0;
    stream->nextActiveStream = 0;
    return true;
}

int buildMinimumPathKey(Program *program, InstructionStream *stream)
{
    VisitCache *visitCache = &program->backTraceVisitCache;
    if (!visitCache->startRecord()) {
        reportError("failed to start a record\n");
        return 0;
    }
    if (!visitCache->setBit(program->traceTargetStream->streamId)) {
        reportError("failed to add target to min path key\n");
        return 0;
    }
    for (; stream; stream = stream->shortestPathPredecessor) {
        if (!visitCache->setBit(stream->streamId)) {
            reportError("failed to add to to min path record\n");
            return 0;
        }
    }
    return visitCache->endRecord();
}

int getMaxPathKey(Program *program, InstructionStream *stream)
{
    VisitCache *visitCache = &program->backTraceVisitCache;
    if (!visitCache->startRecord()) {
        reportError("failed to start a record\n");
        return 0;
    }
    if (!visitCache->setBit(program->traceTargetStream->streamId)) {
        reportError("failed to add target to max path key\n");
        return 0;
    }
    for (; stream; stream = stream->longestPathPredecessor) {
        if (!visitCache->setBit(stream->streamId)) {
            reportError("failed to add to record to max path\n");
            return 0;
        }
    }
    return visitCache->endRecord();
}

bool isPathRaw(Program *program, int key)
{
    if (key <= 0) {
        reportError("<=0 input to isPathRaw\n");
        exitAfterCleanup(-1);
    }
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        int streamId = stream->streamId;
        if (program->backTraceVisitCache.testBit(key, streamId) && stream->getCodeMode() == 1)
            return false;
    }
    return true;
}

int guessKeyLen(Program *program, int key, int start, int end)
{
    InstructionStream *stream = program->streamHead;
    int len = 0;
    bool foundStart = false;
    bool foundEnd = false;
    for (; stream; stream = stream->nextStream) {
        int streamId = stream->streamId;
        if (program->backTraceVisitCache.testBit(key, streamId)) {
            bool hit = false;
            if (streamId == start) {
                foundStart = true;
                hit = true;
            }
            if (streamId == end) {
                foundEnd = true;
                hit = true;
            }
            if (!hit)
                len += stream->getOrGuessDepLength();
        }
    }
    if (!foundEnd || !foundStart) {
        reportError("guessKeyLen - didn't contain one of start or end\n");
        return -1;
    }
    return len;
}

void dumpMinPathToTarget(InstructionStream *stream)
{
    reportInfo("%s  minPathToTarget\n", stream->streamName());
    int pathDepth = 0;
    for (InstructionStream *predecessorStream = stream->shortestPathPredecessor; predecessorStream; predecessorStream = predecessorStream->shortestPathPredecessor)
        reportInfo("    %2d:   [%3d] %s\n", ++pathDepth, predecessorStream->minimumPathLength, predecessorStream->streamName());
    reportInfo("done .. %s  minPathToTarget\n", stream->streamName());
}

void dumpMaxPathToTarget(InstructionStream *stream)
{
    reportInfo("%s  maxPathToTarget\n", stream->streamName());
    int pathDepth = 0;
    for (InstructionStream *predecessorStream = stream->longestPathPredecessor; predecessorStream; predecessorStream = predecessorStream->longestPathPredecessor)
        reportInfo("    %2d:   [%3d] %s\n", ++pathDepth, predecessorStream->maximumPathLength, predecessorStream->streamName());
    reportInfo("done .. %s  maxPathToTarget\n", stream->streamName());
}
