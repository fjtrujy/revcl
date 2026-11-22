#include "os.h"
#include "backTrace_shared.h"

static inline regUseSlot *registerPart(regUse *u, int part)
{
    return &u->slots[part];
}

static inline short *thruputTimes(regUse *u, int part)
{
    return &u->slots[part].timingValues[10];
}

static inline int thruputTail(instStream *s, regUse *u, int part)
{
    int last = u->slots[part].timingValues[10];
    int length = s->backtraceDependencyLength;
    return length - last;
}

bool addThruputStalls(Program *p, instStream *t, RegPoolEntry *reg, int part)
{
    regUse *tr = t->unk_234;
    if (!tr) {
        reportError("bThru enabled - but no btRegRef in the target!\n");
        return false;
    }
    for (instStream *s = p->sourceStreams; s; s = s->nextSourceStream) {
        regUse *sr = s->unk_234;
        if (!sr) {
            reportError("no btRegRef for something on the match list %s.%d target=%s stream is %s!\n",
                          reg->registerName(), part, t->streamName(), s->streamName());
            return false;
        }
        int key = func_08061cb0(p, s);
        if (key <= 0) {
            reportError("minimal path key was bad\n");
            return false;
        }
        ol_s *ol = registerPart(sr, part)->sourceLocations.head;
        if (!ol)
            reportError("there's no local setters for thruput chase for ext setter\n");
        for (; ol; ol = ol->next) {
            ol_s *n = tr->slots[part].externalInputLocations.copyOperandLocation(ol, true);
            if (!n) {
                reportError("failed to add an external setter\n");
                return false;
            }
            n->pathKey = key;
        }
        ol = registerPart(tr, part)->sourceLocations.head;
        if (!ol) {
            reportError("there's no local setters.. for a thruput chase .. for target\n");
            return false;
        }
        for (; ol; ol = ol->next) {
            ol_s *n = sr->slots[part].externalSourceLocations.copyOperandLocation(ol, true);
            if (!n) {
                reportError("failed to add post setter!\n");
                return false;
            }
            n->pathKey = key;
        }
        if (isPathRaw(p, key)) {
            int dist = s->unk_240;
            int last = sr->slots[part].timingValues[11];
            short *lastAddress = thruputTimes(sr, part);
            int tend = thruputTail(s, sr, part);
            int fst = tr->slots[part].timingValues[7];
            if (fst < 0 || tend < 0 || dist < 0) {
                reportError("something wrong checking for thruput stalls fst=%d tend=%d path_dist=%d\n",
                              fst, tend, dist);
                return false;
            }
            dist = (dist + tend) + fst;
            last = lastAddress[1];
            if (dist < last) {
                regUseSlot *slot = &sr->slots[part];
                ol_s *setter = slot->findPreviousOutputAtPosition(0, true);
                int again = lastAddress[1];
                if (!p->proc.stalls.addStall(setter, tr->slots[part].sourceLocations.head, 1,
                                        again - dist)) {
                    reportError("failed to add an interstream latency stall\n");
                    return false;
                }
            }
        }
    }
    return true;
}
