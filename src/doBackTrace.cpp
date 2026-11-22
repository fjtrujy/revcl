#include "os.h"
#include "backTrace_shared.h"

static inline bool clearTraceStoreInline(Program *program, unsigned *traceStore)
{
    if (!traceStore) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int wordIndex = 0; wordIndex != program->traceStoreWordCount; wordIndex++)
        traceStore[wordIndex] = 0;
    return true;
}

static inline bool initializeTraceStoreInline(Program *program, unsigned *traceStore, unsigned bit)
{
    if (!clearTraceStoreInline(program, traceStore)) {
        reportError("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    traceStore[idx] = mask;
    return true;
}

static inline bool testTraceStoreInline(Program *program, unsigned *traceStore, unsigned bit)
{
    if (!traceStore) {
        reportError("store not set for test tstore\n");
        exitAfterCleanup(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (traceStore[idx] & mask) != 0;
}

static inline void dumpBackTrace(Program *program)
{
    dumpProgramStreamsAndRegisters(program, 0);
}

bool doBackTrace(Program *program, instStream *targetStream, RegPoolEntry *registerEntry, int part, int earliestUse, int ioMode, bool thruput)
{
    if (!targetStream || !registerEntry) {
        reportError("doBackTrace - null arg\n");
        return false;
    }
    if (ioMode && earliestUse < 0) {
        reportError("-ve earliestUse passed into doBackTrace %s.%d at %s iomode=%d\n",
                      registerEntry->registerName(), part, targetStream->streamName(), ioMode);
        return false;
    }
    bool blocking = false;
    bool latency = false;
    switch (ioMode) {
    case 0:
        break;
    case 2:
        blocking = true;
    case 1:
        latency = true;
        break;
    default:
        reportError("bad ioMode for backtrace\n");
        return false;
    }
    program->trackMaximumPathLength = latency && !blocking;
    program->traceStoreEnabled = latency && !blocking;
    program->traceTargetStream = targetStream;
    for (instStream *initialStream = program->streamHead; initialStream; initialStream = initialStream->nextStream) {
        if (!initializeRegisterBacktraceLocation(initialStream, registerEntry, part)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
        if (program->traceStoreEnabled && !initializeTraceStoreInline(program, initialStream->traceStore, initialStream->streamId)) {
            reportError("failed to init tstore\n");
            return false;
        }
    }
    program->sourceStreams = 0;
    program->resolvedStreams = 0;
    program->activeStreams = 0;
    if (!extendBackTrace(program, targetStream)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (instStream *activeStream = program->activeStreams; activeStream; activeStream = activeStream->nextActiveStream) {
        if (program->traceStoreEnabled && !initializeTraceStoreInline(program, activeStream->traceStore, activeStream->streamId)) {
            reportError("clear a tstore after first init\n");
            return false;
        }
        activeStream->shortestPathPredecessor = 0;
        activeStream->longestPathPredecessor = 0;
        activeStream->minimumPathLength = 0;
        activeStream->maximumPathLength = 0;
    }
    if (!processActiveList(program)) {
        reportError("the initial trace failed\n");
        return false;
    }
    if (thruput && !addThroughputStalls(program, targetStream, registerEntry, part)) {
        reportError("internal thruput check failed\n");
        return false;
    }
    if (latency) {
        if (!linkMatchList(program, targetStream, registerEntry, part, earliestUse, blocking)) {
            reportError("internal latency check failed\n");
            return false;
        }
        if (blocking) {
            if (program->activeStreams) {
                reportError("in blocking mode , things got added to the active list\n");
                return false;
            }
        } else {
            while (program->activeStreams) {
                if (!processActiveList(program)) {
                    reportError("failed to continue back trace\n");
                    return false;
                }
                if (!linkMatchList(program, targetStream, registerEntry, part, earliestUse, blocking)) {
                    reportError("internal latency check failed\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *visitedStream = program->streamHead; visitedStream; visitedStream = visitedStream->nextStream) {
            unsigned char bit = 1 << part;
            if (visitedStream->visitState == 4) {
                reportInfo("UBS: %s %s.%d\n", program->traceTargetStream->streamName(), registerEntry->registerName(), part);
                if (!program->proc.usedBeforeSet.addUBS(visitedStream, program->traceTargetStream, registerEntry, bit)) {
                    reportError("failed to add ubs\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *linkedStream = program->streamHead; linkedStream; linkedStream = linkedStream->nextStream) {
            if (linkedStream->visitState == 2 || linkedStream->visitState == 4) {
                if (!linkedStream->backtraceRegisterUse) {
                    workTable *scheduleTable = linkedStream->getTable();
                    if (!scheduleTable) {
                        reportError("no work table\n");
                        return false;
                    }
                    linkedStream->backtraceRegisterUse = scheduleTable->registerUses.addRegisterUse(registerEntry);
                    if (!linkedStream->backtraceRegisterUse) {
                        reportError("couldn't add regref to table\n");
                        return false;
                    }
                }
                linkedStream->backtraceRegisterUse->throughputMask |= 1 << part;
                linkedStream->pendingHazardBroadcast = 1;
            }
        }
        if (!blocking) {
        for (instStream *hazardSourceStream = program->streamHead; hazardSourceStream; hazardSourceStream = hazardSourceStream->nextStream) {
            if (!hazardSourceStream->hazardFlag)
                continue;
            instStream *hazardCandidateStream;
            for (hazardCandidateStream = program->streamHead; hazardCandidateStream; hazardCandidateStream = hazardCandidateStream->nextStream)
                hazardCandidateStream->hazardVisited = 0;
            hazardSourceStream->hazardVisited = 1;
            bool changed;
            do {
                changed = false;
                for (hazardCandidateStream = program->streamHead; hazardCandidateStream; hazardCandidateStream = hazardCandidateStream->nextStream) {
                    if (hazardCandidateStream->hazardVisited)
                        continue;
                    hazardCandidateStream->hazardVisited = 1;
                    if (testTraceStoreInline(program, hazardSourceStream->traceStore, hazardCandidateStream->streamId)) {
                        regUse *registerUse = hazardCandidateStream->backtraceRegisterUse;
                        if (!registerUse) {
                            reportError("couldn't get regref for hazard broadcast op for reg %s.%d in %s hazard start=%s target=%s mode=%d\n",
                                          registerEntry->registerName(), part, hazardCandidateStream->streamName(), hazardSourceStream->streamName(),
                                          targetStream->streamName(), hazardCandidateStream->visitState);
                            dumpBackTrace(program);
                            return false;
                        }
                        registerUse->hazardBroadcastMask |= 1 << part;
                        int mergedTraceState = mergeBacktraceTraceStores(program, hazardSourceStream->traceStore, hazardCandidateStream->traceStore);
                        if (mergedTraceState < 0) {
                            reportError("failed to merge  stores\n");
                            return false;
                        }
                        if (mergedTraceState > 0)
                            changed = true;
                    }
                }
            } while (changed);
        }
        }
        if (!blocking) {
        for (instStream *resolvedStreamCandidate = program->streamHead; resolvedStreamCandidate; resolvedStreamCandidate = resolvedStreamCandidate->nextStream) {
            if (resolvedStreamCandidate->pendingHazardBroadcast) {
                resolvedStreamCandidate->nextResolvedStream = program->resolvedStreams;
                program->resolvedStreams = resolvedStreamCandidate;
                int key = buildMinimumPathKey(program, resolvedStreamCandidate);
                resolvedStreamCandidate->destinationKey = key;
                if (key < 0) {
                    reportError("failed to get minimal path for localT tracker\n");
                    return false;
                }
            }
        }
        for (instStream *resolvedStream = program->resolvedStreams; resolvedStream; resolvedStream = resolvedStream->nextResolvedStream) {
            if (!buildCrossStreamThroughputPairs(program, registerEntry, part, resolvedStream, targetStream))
                reportError("failed to build tpairs for register %s with target %s at stream %s\n",
                              registerEntry->registerName(), targetStream->streamName(), resolvedStream->streamName());
        }
        }
    }
    return true;
}

int mergeBacktraceTraceStores(Program *program, unsigned *dst, unsigned *src)
{
    if (!src || !dst) {
        reportError("mergetTstores - null input\n");
        return -1;
    }
    unsigned changed = 0;
    for (int wordIndex = 0; wordIndex != program->traceStoreWordCount; wordIndex++) {
        unsigned destinationBits = dst[wordIndex];
        changed |= ~destinationBits & src[wordIndex];
        dst[wordIndex] = destinationBits | src[wordIndex];
    }
    return changed != 0;
}

bool clearBacktraceTraceStore(Program *program, unsigned *traceStore)
{
    if (!traceStore) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int wordIndex = 0; wordIndex != program->traceStoreWordCount; wordIndex++)
        traceStore[wordIndex] = 0;
    return true;
}

bool initializeBacktraceTraceStore(Program *program, unsigned *traceStore, unsigned bit)
{
    if (!clearTraceStoreInline(program, traceStore)) {
        reportError("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    traceStore[idx] = mask;
    return true;
}

bool testBacktraceTraceStore(Program *program, unsigned *traceStore, unsigned bit)
{
    if (!traceStore) {
        reportError("store not set for test tstore\n");
        exitAfterCleanup(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (traceStore[idx] & mask) != 0;
}
