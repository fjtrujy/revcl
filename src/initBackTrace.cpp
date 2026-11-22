#include "os.h"

#include "backTrace_shared.h"

bool initBackTrace(Program *program)
{
    int maximumStreamId = -1;
    instStream *currentStream;
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (currentStream->streamId < 0) {
            reportError("flowUID<0 for g_initBackTrace\n");
            return false;
        }
        if (currentStream->streamId > maximumStreamId)
            maximumStreamId = currentStream->streamId;
    }
    if (maximumStreamId < 0) {
        reportError("mxUID was 0... nothing to backtrace ??\n");
        return false;
    }
    int streamCount = maximumStreamId + 1;
    for (currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        if (!g_initBackTrace(currentStream, streamCount)) {
            reportError("[Depend] failed to g_initBackTrace a stream\n");
            return false;
        }
    }
    program->traceStoreWordCount = (streamCount + 31) >> 2;
    if (!program->backTraceVisitCache.init(streamCount)) {
        reportError("flowMin::flowReset - failed to init visit cache\n");
        return false;
    }
    if (!program->backTraceVisitCache.startRecord()) {
        reportError("flowReset: failed to start record\n");
        return false;
    }
    int key = program->backTraceVisitCache.endRecord();
    if (key != 0) {
        reportError("flowMon::flowReset - got an unexpected key back for null bit vector (%d)\n", key);
        return false;
    }
    return true;
}

bool disposeBackTrace(Program *program)
{
    program->traceStoreWordCount = 0;
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        if (stream->traceStore)
            delete[] stream->traceStore;
        stream->traceStore = 0;
    }
    return true;
}

bool g_initBackTrace(instStream *s, int streamCount)
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
    if (!streamCount) {
        reportError("no t store entries\n");
        return false;
    }
    s->traceStore = new unsigned[(streamCount + 31) / 4];
    if (!s->traceStore) {
        reportError("failed to get new tSTore\n");
        return false;
    }
    return true;
}

bool g_disposeBackTrace(instStream *stream)
{
    if (stream->traceStore)
        delete[] stream->traceStore;
    stream->traceStore = 0;
    return true;
}

