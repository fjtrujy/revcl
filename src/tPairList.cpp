#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "workTable.h"
#include "Program.h"
#include "instStream.h"

int reportError(const char *fmt, ...);
bool resetStreamPathSearch(instStream *n);
bool extendBackTrace(Program *p, instStream *n);
bool processActiveList(Program *p);
int buildMinimumPathKey(Program *p, instStream *n);

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
    int n = 0;
    for (tPair *transitionPair = head; transitionPair; transitionPair = transitionPair->next)
        n++;
    if (n != count) {
        reportError("     tpairdump : entry count mismatch\n");
        exitAfterCleanup(-1);
    }
}

bool tPairList::addPair(void *a, void *b, int src, int dst)
{
    if (!a || !b) {
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
    transitionPair->a = a;
    transitionPair->b = b;
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

bool int_buildTpairs(Program *program, RegPoolEntry *reg, int part, instStream *stream, void *b)
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
    for (instStream *i = program->streamHead; i; i = i->nextStream) {
        if (!resetStreamPathSearch(i)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
    }
    if (!extendBackTrace(program, stream)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (instStream *i = program->activeStreams; i; i = i->nextActiveStream) {
        i->shortestPathPredecessor = 0;
        i->longestPathPredecessor = 0;
        i->minimumPathLength = 0;
        i->maximumPathLength = 0;
    }
    if (!processActiveList(program)) {
        reportError("failed to do internal trace for  a t pair\n");
        return false;
    }
    regUse *registerUse = stream->backtraceRegisterUse;
    if (!registerUse) {
        reportError("there's  no regref set in t target\n");
        return false;
    }
    if (registerUse->registerEntry != reg) {
        reportError("there's a mismatch between reg and regref\n");
        return false;
    }
    for (instStream *s = program->sourceStreams; s; s = s->nextSourceStream) {
        regUse *sr = s->backtraceRegisterUse;
        if (!sr) {
            reportError("there's no regref in t src\n");
            return false;
        }
        if (sr->registerEntry != reg) {
            reportError("mismatch between regref reg and rg\n");
            return false;
        }
        int dst_key = stream->destinationKey;
        int src_key = buildMinimumPathKey(program, s);
        if (dst_key < 0) {
            reportError("dst_key is bad in tpair\n");
            return false;
        }
        if (src_key < 0) {
            reportError("src_key is bad in tpair\n");
            return false;
        }
        if (!registerUse->slots[part].throughputPairs.addPair(s, b, src_key, dst_key)) {
            reportError("failed to add tpair for tcheck\n");
            return false;
        }
    }
    return true;
}
