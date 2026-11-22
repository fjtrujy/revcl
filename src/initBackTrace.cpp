#include "os.h"

#include "backTrace_shared.h"

bool initBackTrace(Program *p)
{
    int mx = -1;
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        if (s->streamId < 0) {
            reportError("flowUID<0 for g_initBackTrace\n");
            return false;
        }
        if (s->streamId > mx)
            mx = s->streamId;
    }
    if (mx < 0) {
        reportError("mxUID was 0... nothing to backtrace ??\n");
        return false;
    }
    int n = mx + 1;
    for (s = p->streamHead; s; s = s->nextStream) {
        if (!g_initBackTrace(s, n)) {
            reportError("[Depend] failed to g_initBackTrace a stream\n");
            return false;
        }
    }
    p->evenPipeMoveCount = (n + 31) >> 2;
    if (!p->backTraceVisitCache.init(n)) {
        reportError("flowMin::flowReset - failed to init visit cache\n");
        return false;
    }
    if (!p->backTraceVisitCache.startRecord()) {
        reportError("flowReset: failed to start record\n");
        return false;
    }
    int key = p->backTraceVisitCache.endRecord();
    if (key != 0) {
        reportError("flowMon::flowReset - got an unexpected key back for null bit vector (%d)\n", key);
        return false;
    }
    return true;
}

bool disposeBackTrace(Program *p)
{
    p->evenPipeMoveCount = 0;
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (s->traceStore)
            delete[] s->traceStore;
        s->traceStore = 0;
    }
    return true;
}

bool g_initBackTrace(instStream *s, int n)
{
    s->backtraceDependencyLength = s->getOrGuessDepLength();
    if (s->backtraceDependencyLength < 0) {
        reportError("ist::g_initBackTrace - failed to get or guess the [depend] length of this stream\n");
        return false;
    }
    int len = s->getOrGuessLogicalLength();
    if (len < 0) {
        reportError("ist::g_initBackTrace - failed to get or guess the logical length of this stream\n");
        return false;
    }
    if (len > s->backtraceDependencyLength) {
        reportError("minimal length > dep length for %s (%d > %d)\n", s->streamName(), len, s->backtraceDependencyLength);
        return false;
    }
    if (s->endType == 2)
        s->backtraceDependencyLength += 1000;
    if (s->traceStore) {
        reportError("m_tStore is already set !\n");
        return false;
    }
    if (!n) {
        reportError("no t store entries\n");
        return false;
    }
    s->traceStore = new unsigned[(n + 31) / 4];
    if (!s->traceStore) {
        reportError("failed to get new tSTore\n");
        return false;
    }
    return true;
}

bool g_disposeBackTrace(instStream *s)
{
    if (s->traceStore)
        delete[] s->traceStore;
    s->traceStore = 0;
    return true;
}

