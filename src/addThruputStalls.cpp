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

bool addThruputStalls(Program *program, instStream *targetStream, RegPoolEntry *reg, int part)
{
    regUse *registerUse = targetStream->backtraceRegisterUse;
    if (!registerUse) {
        reportError("bThru enabled - but no btRegRef in the target!\n");
        return false;
    }
    for (instStream *sourceStream = program->sourceStreams; sourceStream; sourceStream = sourceStream->nextSourceStream) {
        regUse *sourceRegisterUse = sourceStream->backtraceRegisterUse;
        if (!sourceRegisterUse) {
            reportError("no btRegRef for something on the match list %s.%d target=%s stream is %s!\n",
                          reg->registerName(), part, targetStream->streamName(), sourceStream->streamName());
            return false;
        }
        int key = buildMinimumPathKey(program, sourceStream);
        if (key <= 0) {
            reportError("minimal path key was bad\n");
            return false;
        }
        ol_s *location = registerPart(sourceRegisterUse, part)->sourceLocations.head;
        if (!location)
            reportError("there's no local setters for thruput chase for ext setter\n");
        for (; location; location = location->next) {
            ol_s *newInputLocation = registerUse->slots[part].externalInputLocations.copyOperandLocation(location, true);
            if (!newInputLocation) {
                reportError("failed to add an external setter\n");
                return false;
            }
            newInputLocation->pathKey = key;
        }
        location = registerPart(registerUse, part)->sourceLocations.head;
        if (!location) {
            reportError("there's no local setters.. for a thruput chase .. for target\n");
            return false;
        }
        for (; location; location = location->next) {
            ol_s *newSourceLocation = sourceRegisterUse->slots[part].externalSourceLocations.copyOperandLocation(location, true);
            if (!newSourceLocation) {
                reportError("failed to add post setter!\n");
                return false;
            }
            newSourceLocation->pathKey = key;
        }
        if (isPathRaw(program, key)) {
            int dist = sourceStream->minimumPathLength;
            int last = sourceRegisterUse->slots[part].timingValues[11];
            short *lastAddress = thruputTimes(sourceRegisterUse, part);
            int tend = thruputTail(sourceStream, sourceRegisterUse, part);
            int fst = registerUse->slots[part].timingValues[7];
            if (fst < 0 || tend < 0 || dist < 0) {
                reportError("something wrong checking for thruput stalls fst=%d tend=%d path_dist=%d\n",
                              fst, tend, dist);
                return false;
            }
            dist = (dist + tend) + fst;
            last = lastAddress[1];
            if (dist < last) {
                regUseSlot *slot = &sourceRegisterUse->slots[part];
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
