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
        if (!initializeStreamBacktrace(currentStream, streamCount)) {
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

bool initializeStreamBacktrace(instStream *initialStream, int streamCount)
{
    initialStream->backtraceDependencyLength = initialStream->getOrGuessDepLength();
    if (initialStream->backtraceDependencyLength < 0) {
        reportError("ist::g_initBackTrace - failed to get or guess the [depend] length of this stream\n");
        return false;
    }
    int len = initialStream->getOrGuessLogicalLength();
    if (len < 0) {
        reportError("ist::g_initBackTrace - failed to get or guess the logical length of this stream\n");
        return false;
    }
    if (len > initialStream->backtraceDependencyLength) {
        reportError("minimal length > dep length for %s (%d > %d)\n", initialStream->streamName(), len, initialStream->backtraceDependencyLength);
        return false;
    }
    if (initialStream->endType == 2)
        initialStream->backtraceDependencyLength += 1000;
    if (initialStream->traceStore) {
        reportError("m_tStore is already set !\n");
        return false;
    }
    if (!streamCount) {
        reportError("no t store entries\n");
        return false;
    }
    initialStream->traceStore = new unsigned[(streamCount + 31) / 4];
    if (!initialStream->traceStore) {
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

