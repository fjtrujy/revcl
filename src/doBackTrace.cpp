#include "os.h"
#include "backTrace_shared.h"

static inline bool zeroTstore_inl(Program *p, unsigned *t)
{
    if (!t) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int i = 0; i != p->traceStoreWordCount; i++)
        t[i] = 0;
    return true;
}

static inline bool initTstore_inl(Program *p, unsigned *t, unsigned bit)
{
    if (!zeroTstore_inl(p, t)) {
        reportError("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)p->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    t[idx] = mask;
    return true;
}

static inline bool testTstore_inl(Program *p, unsigned *t, unsigned bit)
{
    if (!t) {
        reportError("store not set for test tstore\n");
        exitAfterCleanup(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)p->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}

static inline void dumpBackTrace(Program *p)
{
    dumpProgramStreamsAndRegisters(p, 0);
}

bool doBackTrace(Program *p, instStream *t, RegPoolEntry *reg, int part, int earliestUse, int ioMode, bool thruput)
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
    p->trackMaximumPathLength = latency && !blocking;
    p->traceStoreEnabled = latency && !blocking;
    p->traceTargetStream = t;
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!func_08061ab0(s, reg, part)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
        if (p->traceStoreEnabled && !initTstore_inl(p, s->traceStore, s->streamId)) {
            reportError("failed to init tstore\n");
            return false;
        }
    }
    p->sourceStreams = 0;
    p->resolvedStreams = 0;
    p->activeStreams = 0;
    if (!extendBackTrace(p, t)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (instStream *s = p->activeStreams; s; s = s->nextActiveStream) {
        if (p->traceStoreEnabled && !initTstore_inl(p, s->traceStore, s->streamId)) {
            reportError("clear a tstore after first init\n");
            return false;
        }
        s->shortestPathPredecessor = 0;
        s->longestPathPredecessor = 0;
        s->unk_240 = 0;
        s->unk_248 = 0;
    }
    if (!processActiveList(p)) {
        reportError("the initial trace failed\n");
        return false;
    }
    if (thruput && !addThruputStalls(p, t, reg, part)) {
        reportError("internal thruput check failed\n");
        return false;
    }
    if (latency) {
        if (!linkMatchList(p, t, reg, part, earliestUse, blocking)) {
            reportError("internal latency check failed\n");
            return false;
        }
        if (blocking) {
            if (p->activeStreams) {
                reportError("in blocking mode , things got added to the active list\n");
                return false;
            }
        } else {
            while (p->activeStreams) {
                if (!processActiveList(p)) {
                    reportError("failed to continue back trace\n");
                    return false;
                }
                if (!linkMatchList(p, t, reg, part, earliestUse, blocking)) {
                    reportError("internal latency check failed\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = p->streamHead; s; s = s->nextStream) {
            unsigned char bit = 1 << part;
            if (s->visitState == 4) {
                reportInfo("UBS: %s %s.%d\n", p->traceTargetStream->streamName(), reg->registerName(), part);
                if (!p->proc.usedBeforeSet.addUBS(s, p->traceTargetStream, reg, bit)) {
                    reportError("failed to add ubs\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = p->streamHead; s; s = s->nextStream) {
            if (s->visitState == 2 || s->visitState == 4) {
                if (!s->unk_234) {
                    workTable *wt = s->getTable();
                    if (!wt) {
                        reportError("no work table\n");
                        return false;
                    }
                    s->unk_234 = wt->registerUses.AddNewRegRef(reg);
                    if (!s->unk_234) {
                        reportError("couldn't add regref to table\n");
                        return false;
                    }
                }
                s->unk_234->throughputMask |= 1 << part;
                s->unk_227 = 1;
            }
        }
        if (!blocking) {
        for (instStream *k = p->streamHead; k; k = k->nextStream) {
            if (!k->hazardFlag)
                continue;
            instStream *n;
            for (n = p->streamHead; n; n = n->nextStream)
                n->hazardVisited = 0;
            k->hazardVisited = 1;
            bool changed;
            do {
                changed = false;
                for (n = p->streamHead; n; n = n->nextStream) {
                    if (n->hazardVisited)
                        continue;
                    n->hazardVisited = 1;
                    if (testTstore_inl(p, k->traceStore, n->streamId)) {
                        regUse *u = n->unk_234;
                        if (!u) {
                            reportError("couldn't get regref for hazard broadcast op for reg %s.%d in %s hazard start=%s target=%s mode=%d\n",
                                          reg->registerName(), part, n->streamName(), k->streamName(),
                                          t->streamName(), n->visitState);
                            dumpBackTrace(p);
                            return false;
                        }
                        u->hazardBroadcastMask |= 1 << part;
                        int r = mergeTstores(p, k->traceStore, n->traceStore);
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
        for (instStream *s = p->streamHead; s; s = s->nextStream) {
            if (s->unk_227) {
                s->nextResolvedStream = p->resolvedStreams;
                p->resolvedStreams = s;
                int key = func_08061cb0(p, s);
                s->destinationKey = key;
                if (key < 0) {
                    reportError("failed to get minimal path for localT tracker\n");
                    return false;
                }
            }
        }
        for (instStream *s = p->resolvedStreams; s; s = s->nextResolvedStream) {
            if (!int_buildTpairs(p, reg, part, s, t))
                reportError("failed to build tpairs for register %s with target %s at stream %s\n",
                              reg->registerName(), t->streamName(), s->streamName());
        }
        }
    }
    return true;
}

int mergeTstores(Program *p, unsigned *dst, unsigned *src)
{
    if (!src || !dst) {
        reportError("mergetTstores - null input\n");
        return -1;
    }
    unsigned changed = 0;
    for (int i = 0; i != p->traceStoreWordCount; i++) {
        unsigned d = dst[i];
        changed |= ~d & src[i];
        dst[i] = d | src[i];
    }
    return changed != 0;
}

bool zeroTstore(Program *p, unsigned *t)
{
    if (!t) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int i = 0; i != p->traceStoreWordCount; i++)
        t[i] = 0;
    return true;
}

bool initTstore(Program *p, unsigned *t, unsigned bit)
{
    if (!zeroTstore_inl(p, t)) {
        reportError("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)p->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    t[idx] = mask;
    return true;
}

bool testTstore(Program *p, unsigned *t, unsigned bit)
{
    if (!t) {
        reportError("store not set for test tstore\n");
        exitAfterCleanup(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)p->traceStoreWordCount) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}
