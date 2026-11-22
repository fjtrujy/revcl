#include "os.h"
#include "backTrace_shared.h"

static inline bool zeroTstore_inl(Program *program, unsigned *t)
{
    if (!t) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int i = 0; i != program->traceStoreWordCount; i++)
        t[i] = 0;
    return true;
}

static inline bool initTstore_inl(Program *program, unsigned *t, unsigned bit)
{
    if (!zeroTstore_inl(program, t)) {
        reportError("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    t[idx] = mask;
    return true;
}

static inline bool testTstore_inl(Program *program, unsigned *t, unsigned bit)
{
    if (!t) {
        reportError("store not set for test tstore\n");
        exitAfterCleanup(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}

static inline void dumpBackTrace(Program *program)
{
    dumpProgramStreamsAndRegisters(program, 0);
}

bool doBackTrace(Program *program, instStream *t, RegPoolEntry *reg, int part, int earliestUse, int ioMode, bool thruput)
{
    if (!t || !reg) {
        reportError("doBackTrace - null arg\n");
        return false;
    }
    if (ioMode && earliestUse < 0) {
        reportError("-ve earliestUse passed into doBackTrace %s.%d at %s iomode=%d\n",
                      reg->registerName(), part, t->streamName(), ioMode);
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
    program->traceTargetStream = t;
    for (instStream *s = program->streamHead; s; s = s->nextStream) {
        if (!initializeRegisterBacktraceLocation(s, reg, part)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
        if (program->traceStoreEnabled && !initTstore_inl(program, s->traceStore, s->streamId)) {
            reportError("failed to init tstore\n");
            return false;
        }
    }
    program->sourceStreams = 0;
    program->resolvedStreams = 0;
    program->activeStreams = 0;
    if (!extendBackTrace(program, t)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (instStream *s = program->activeStreams; s; s = s->nextActiveStream) {
        if (program->traceStoreEnabled && !initTstore_inl(program, s->traceStore, s->streamId)) {
            reportError("clear a tstore after first init\n");
            return false;
        }
        s->shortestPathPredecessor = 0;
        s->longestPathPredecessor = 0;
        s->minimumPathLength = 0;
        s->maximumPathLength = 0;
    }
    if (!processActiveList(program)) {
        reportError("the initial trace failed\n");
        return false;
    }
    if (thruput && !addThruputStalls(program, t, reg, part)) {
        reportError("internal thruput check failed\n");
        return false;
    }
    if (latency) {
        if (!linkMatchList(program, t, reg, part, earliestUse, blocking)) {
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
                if (!linkMatchList(program, t, reg, part, earliestUse, blocking)) {
                    reportError("internal latency check failed\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = program->streamHead; s; s = s->nextStream) {
            unsigned char bit = 1 << part;
            if (s->visitState == 4) {
                reportInfo("UBS: %s %s.%d\n", program->traceTargetStream->streamName(), reg->registerName(), part);
                if (!program->proc.usedBeforeSet.addUBS(s, program->traceTargetStream, reg, bit)) {
                    reportError("failed to add ubs\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = program->streamHead; s; s = s->nextStream) {
            if (s->visitState == 2 || s->visitState == 4) {
                if (!s->backtraceRegisterUse) {
                    workTable *wt = s->getTable();
                    if (!wt) {
                        reportError("no work table\n");
                        return false;
                    }
                    s->backtraceRegisterUse = wt->registerUses.AddNewRegRef(reg);
                    if (!s->backtraceRegisterUse) {
                        reportError("couldn't add regref to table\n");
                        return false;
                    }
                }
                s->backtraceRegisterUse->throughputMask |= 1 << part;
                s->pendingHazardBroadcast = 1;
            }
        }
        if (!blocking) {
        for (instStream *k = program->streamHead; k; k = k->nextStream) {
            if (!k->hazardFlag)
                continue;
            instStream *n;
            for (n = program->streamHead; n; n = n->nextStream)
                n->hazardVisited = 0;
            k->hazardVisited = 1;
            bool changed;
            do {
                changed = false;
                for (n = program->streamHead; n; n = n->nextStream) {
                    if (n->hazardVisited)
                        continue;
                    n->hazardVisited = 1;
                    if (testTstore_inl(program, k->traceStore, n->streamId)) {
                        regUse *registerUse = n->backtraceRegisterUse;
                        if (!registerUse) {
                            reportError("couldn't get regref for hazard broadcast op for reg %s.%d in %s hazard start=%s target=%s mode=%d\n",
                                          reg->registerName(), part, n->streamName(), k->streamName(),
                                          t->streamName(), n->visitState);
                            dumpBackTrace(program);
                            return false;
                        }
                        registerUse->hazardBroadcastMask |= 1 << part;
                        int r = mergeTstores(program, k->traceStore, n->traceStore);
                        if (r < 0) {
                            reportError("failed to merge  stores\n");
                            return false;
                        }
                        if (r > 0)
                            changed = true;
                    }
                }
            } while (changed);
        }
        }
        if (!blocking) {
        for (instStream *s = program->streamHead; s; s = s->nextStream) {
            if (s->pendingHazardBroadcast) {
                s->nextResolvedStream = program->resolvedStreams;
                program->resolvedStreams = s;
                int key = buildMinimumPathKey(program, s);
                s->destinationKey = key;
                if (key < 0) {
                    reportError("failed to get minimal path for localT tracker\n");
                    return false;
                }
            }
        }
        for (instStream *s = program->resolvedStreams; s; s = s->nextResolvedStream) {
            if (!int_buildTpairs(program, reg, part, s, t))
                reportError("failed to build tpairs for register %s with target %s at stream %s\n",
                              reg->registerName(), t->streamName(), s->streamName());
        }
        }
    }
    return true;
}

int mergeTstores(Program *program, unsigned *dst, unsigned *src)
{
    if (!src || !dst) {
        reportError("mergetTstores - null input\n");
        return -1;
    }
    unsigned changed = 0;
    for (int i = 0; i != program->traceStoreWordCount; i++) {
        unsigned d = dst[i];
        changed |= ~d & src[i];
        dst[i] = d | src[i];
    }
    return changed != 0;
}

bool zeroTstore(Program *program, unsigned *t)
{
    if (!t) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int i = 0; i != program->traceStoreWordCount; i++)
        t[i] = 0;
    return true;
}

bool initTstore(Program *program, unsigned *t, unsigned bit)
{
    if (!zeroTstore_inl(program, t)) {
        reportError("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    t[idx] = mask;
    return true;
}

bool testTstore(Program *program, unsigned *t, unsigned bit)
{
    if (!t) {
        reportError("store not set for test tstore\n");
        exitAfterCleanup(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)program->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}
