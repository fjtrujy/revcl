#include "os.h"
#include "backTrace_shared.h"

bool resetStreamPathSearch(instStream *n);

static inline regUseSlot *registerPart(regUse *u, int part)
{
    return &u->slots[part];
}

bool initializeRegisterBacktraceLocation(instStream *n, RegPoolEntry *reg, int part)
{
    workTable *wt = n->getTable();
    if (!wt) {
        reportError("failed to get work table\n");
        return 0;
    }
    regUse *u = wt->registerUses.findByRegister(reg);
    n->pendingHazardBroadcast = 0;
    n->hazardFlag = 0;
    n->nextResolvedStream = 0;
    n->backtraceRegisterUse = u;
    if (u && ((u->outputMask >> part) & 1)) {
        if (wt->codeMode() == 1 || wt->codeMode() == 4) {
            n->outputLocationCount = 1;
            n->earliestOutputLocation = 0;
            n->latestOutputLocation = 0;
        } else {
            int count = u->slots[part].outputLocations.count;
            n->outputLocationCount = count;
            ol_s *head = u->slots[part].outputLocations.head;
            if (!head) {
                reportError("visible outputs has no head for l_init.. but it should!\n");
                return 0;
            }
            int lo = head->unk_12;
            n->earliestOutputLocation = lo;
            ol_s *last = registerPart(u, part)->findPreviousOutputAtPosition(0, true);
            if (!last) {
                reportError("visible outputs had nothing visible at infinity\n");
                return 0;
            }
            int hi = last->unk_12;
            n->latestOutputLocation = hi;
            if (n->earliestOutputLocation > n->latestOutputLocation) {
                reportError("minimal writeVal was > maximal writeVal\n");
                return 0;
            }
        }
    } else {
        n->outputLocationCount = 0;
        n->earliestOutputLocation = -1;
        n->latestOutputLocation = -2;
    }
    return resetStreamPathSearch(n);
}

bool resetStreamPathSearch(instStream *n)
{
    n->visitState = 0;
    for (linkNode *l = n->inputLinks.head; l; l = l->next)
        l->unk_10 = l->linkType != 3 && l->linkType != 4;
    n->minimumPathLength = -1;
    n->shortestPathPredecessor = 0;
    n->maximumPathLength = -1;
    n->longestPathPredecessor = 0;
    n->nextSourceStream = 0;
    n->nextMatchedStream = 0;
    n->nextActiveStream = 0;
    return true;
}

int buildMinimumPathKey(Program *p, instStream *n)
{
    VisitCache *vc = &p->backTraceVisitCache;
    if (!vc->startRecord()) {
        reportError("failed to start a record\n");
        return 0;
    }
    if (!vc->setBit(p->traceTargetStream->streamId)) {
        reportError("failed to add target to min path key\n");
        return 0;
    }
    for (; n; n = n->shortestPathPredecessor) {
        if (!vc->setBit(n->streamId)) {
            reportError("failed to add to to min path record\n");
            return 0;
        }
    }
    return vc->endRecord();
}

int getMaxPathKey(Program *p, instStream *n)
{
    VisitCache *vc = &p->backTraceVisitCache;
    if (!vc->startRecord()) {
        reportError("failed to start a record\n");
        return 0;
    }
    if (!vc->setBit(p->traceTargetStream->streamId)) {
        reportError("failed to add target to max path key\n");
        return 0;
    }
    for (; n; n = n->longestPathPredecessor) {
        if (!vc->setBit(n->streamId)) {
            reportError("failed to add to record to max path\n");
            return 0;
        }
    }
    return vc->endRecord();
}

bool isPathRaw(Program *p, int key)
{
    if (key <= 0) {
        reportError("<=0 input to isPathRaw\n");
        exitAfterCleanup(-1);
    }
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        int id = s->streamId;
        if (p->backTraceVisitCache.testBit(key, id) && s->getCodeMode() == 1)
            return false;
    }
    return true;
}

int guessKeyLen(Program *p, int key, int start, int end)
{
    instStream *s = p->streamHead;
    int len = 0;
    bool foundStart = false;
    bool foundEnd = false;
    for (; s; s = s->nextStream) {
        int id = s->streamId;
        if (p->backTraceVisitCache.testBit(key, id)) {
            bool hit = false;
            if (id == start) {
                foundStart = true;
                hit = true;
            }
            if (id == end) {
                foundEnd = true;
                hit = true;
            }
            if (!hit)
                len += s->getOrGuessDepLength();
        }
    }
    if (!foundEnd || !foundStart) {
        reportError("guessKeyLen - didn't contain one of start or end\n");
        return -1;
    }
    return len;
}

void dumpMinPathToTarget(instStream *s)
{
    reportInfo("%s  minPathToTarget\n", s->streamName());
    int i = 0;
    for (instStream *q = s->shortestPathPredecessor; q; q = q->shortestPathPredecessor)
        reportInfo("    %2d:   [%3d] %s\n", ++i, q->minimumPathLength, q->streamName());
    reportInfo("done .. %s  minPathToTarget\n", s->streamName());
}

void dumpMaxPathToTarget(instStream *s)
{
    reportInfo("%s  maxPathToTarget\n", s->streamName());
    int i = 0;
    for (instStream *q = s->longestPathPredecessor; q; q = q->longestPathPredecessor)
        reportInfo("    %2d:   [%3d] %s\n", ++i, q->maximumPathLength, q->streamName());
    reportInfo("done .. %s  maxPathToTarget\n", s->streamName());
}
