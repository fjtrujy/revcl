#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"
#include "Program.h"
#include "instStream.h"

int reportError(const char *fmt, ...);
bool resetStreamPathSearch(InstructionStream *n);
bool extendBackTrace(Program *p, InstructionStream *n);
bool processActiveList(Program *p);
int buildMinimumPathKey(Program *p, InstructionStream *n);

bool tPairList::clear()
{
    while (head) {
        tPair *transitionPair = head;
        head = transitionPair->next;
        if (transitionPair)
            ::operator delete(transitionPair);
    }
    count = 0;
    return true;
}

void tPairList::dump()
{
    int actualPairCount = 0;
    for (tPair *transitionPair = head; transitionPair; transitionPair = transitionPair->next)
        actualPairCount++;
    if (actualPairCount != count) {
        reportError("     tpairdump : entry count mismatch\n");
        exitAfterCleanup(-1);
    }
}

bool tPairList::addPair(void *sourceObject, void *destinationObject, int src, int dst)
{
    if (!sourceObject || !destinationObject) {
        reportError("tPairList::addPair - null arg\n");
        return false;
    }
    if (src <= 0) {
        reportError("tPairList::addKey() src key is bad\n");
        return false;
    }
    if (dst < 0) {
        reportError("tPairList::addKey() dst_key is bad\n");
        return false;
    }
    tPair *transitionPair = new tPair(tPair::INIT);
    if (!transitionPair) {
        reportError("failed to get a new pair\n");
        return false;
    }
    transitionPair->a = sourceObject;
    transitionPair->b = destinationObject;
    transitionPair->src = src;
    transitionPair->dst = dst;
    transitionPair->next = head;
    count++;
    head = transitionPair;
    return true;
}

tPair::tPair()
{
    next = 0;
    src = -1;
    dst = -1;
    a = 0;
    b = 0;
}

tPair::~tPair()
{
}

void tPair::reset()
{
    next = 0;
    src = -1;
    dst = -1;
    a = 0;
    b = 0;
}

void tPair::noopThroughputPairHook()
{
}

bool buildCrossStreamThroughputPairs(Program *program, RegisterEntry *registerEntry, int part, InstructionStream *stream, void *targetStream)
{
    if (!stream) {
        reportError("int_buildTpairs - null arg\n");
        return false;
    }
    program->traceTargetStream = stream;
    program->sourceStreams = 0;
    program->resolvedStreams = 0;
    program->activeStreams = 0;
    program->traceStoreEnabled = 0;
    program->trackMaximumPathLength = 0;
    for (InstructionStream *currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (!resetStreamPathSearch(currentStream)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
    }
    if (!extendBackTrace(program, stream)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (InstructionStream *activeStream = program->activeStreams; activeStream; activeStream = activeStream->nextActiveStream) {
        activeStream->shortestPathPredecessor = 0;
        activeStream->longestPathPredecessor = 0;
        activeStream->minimumPathLength = 0;
        activeStream->maximumPathLength = 0;
    }
    if (!processActiveList(program)) {
        reportError("failed to do internal trace for  a t pair\n");
        return false;
    }
    RegisterUse *registerUse = stream->backtraceRegisterUse;
    if (!registerUse) {
        reportError("there's  no regref set in t target\n");
        return false;
    }
    if (registerUse->registerEntry != registerEntry) {
        reportError("there's a mismatch between reg and regref\n");
        return false;
    }
    for (InstructionStream *sourceStream = program->sourceStreams; sourceStream; sourceStream = sourceStream->nextSourceStream) {
        RegisterUse *sourceRegisterUse = sourceStream->backtraceRegisterUse;
        if (!sourceRegisterUse) {
            reportError("there's no regref in t src\n");
            return false;
        }
        if (sourceRegisterUse->registerEntry != registerEntry) {
            reportError("mismatch between regref reg and rg\n");
            return false;
        }
        int dst_key = stream->destinationKey;
        int src_key = buildMinimumPathKey(program, sourceStream);
        if (dst_key < 0) {
            reportError("dst_key is bad in tpair\n");
            return false;
        }
        if (src_key < 0) {
            reportError("src_key is bad in tpair\n");
            return false;
        }
        if (!registerUse->slots[part].throughputPairs.addPair(sourceStream, targetStream, src_key, dst_key)) {
            reportError("failed to add tpair for tcheck\n");
            return false;
        }
    }
    return true;
}
