#include "os.h"
#include "backTrace_shared.h"

static inline regUseSlot *registerPart(regUse *registerUse, int part)
{
    return &registerUse->slots[part];
}

static inline short *thruputTimes(regUse *registerUse, int part)
{
    return &registerUse->slots[part].timingValues[10];
}

static inline int thruputTail(instStream *stream, regUse *registerUse, int part)
{
    int last = registerUse->slots[part].timingValues[10];
    int length = stream->backtraceDependencyLength;
    return length - last;
}

bool addThruputStalls(Program *program, instStream *t, RegPoolEntry *reg, int part)
{
    regUse *registerUse = t->backtraceRegisterUse;
    if (!registerUse) {
        reportError("bThru enabled - but no btRegRef in the target!\n");
        return false;
    }
    for (instStream *s = program->sourceStreams; s; s = s->nextSourceStream) {
        regUse *sr = s->backtraceRegisterUse;
        if (!sr) {
            reportError("no btRegRef for something on the match list %s.%d target=%s stream is %s!\n",
                          reg->registerName(), part, t->streamName(), s->streamName());
            return false;
        }
        int key = buildMinimumPathKey(program, s);
        if (key <= 0) {
            reportError("minimal path key was bad\n");
            return false;
        }
        ol_s *location = registerPart(sr, part)->sourceLocations.head;
        if (!location)
            reportError("there's no local setters for thruput chase for ext setter\n");
        for (; location; location = location->next) {
            ol_s *n = registerUse->slots[part].externalInputLocations.copyOperandLocation(location, true);
            if (!n) {
                reportError("failed to add an external setter\n");
                return false;
            }
            n->pathKey = key;
        }
        location = registerPart(registerUse, part)->sourceLocations.head;
        if (!location) {
            reportError("there's no local setters.. for a thruput chase .. for target\n");
            return false;
        }
        for (; location; location = location->next) {
            ol_s *n = sr->slots[part].externalSourceLocations.copyOperandLocation(location, true);
            if (!n) {
                reportError("failed to add post setter!\n");
                return false;
            }
            n->pathKey = key;
        }
        if (isPathRaw(program, key)) {
            int dist = s->minimumPathLength;
            int last = sr->slots[part].timingValues[11];
            short *lastAddress = thruputTimes(sr, part);
            int tend = thruputTail(s, sr, part);
            int fst = registerUse->slots[part].timingValues[7];
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
                if (!program->proc.stalls.addStall(setter, registerUse->slots[part].sourceLocations.head, 1,
                                        again - dist)) {
                    reportError("failed to add an interstream latency stall\n");
                    return false;
                }
            }
        }
    }
    return true;
}
