#include "os.h"
#include "backTrace_shared.h"

static inline regUseSlot *registerPart(regUse *u, int part)
{
    return &u->unk_20[part];
}

static inline short *thruputTimes(regUse *u, int part)
{
    return &u->unk_20[part].unk_3c[10];
}

static inline int thruputTail(instStream *s, regUse *u, int part)
{
    int last = u->unk_20[part].unk_3c[10];
    int length = s->unk_22c;
    return length - last;
}

bool addThruputStalls(Program *p, instStream *t, RegPoolEntry *reg, int part)
{
    regUse *tr = t->unk_234;
    if (!tr) {
        reportError("bThru enabled - but no btRegRef in the target!\n");
        return false;
    }
    for (instStream *s = p->unk_dc; s; s = s->unk_250) {
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
        ol_s *ol = registerPart(sr, part)->unk_20.head;
        if (!ol)
            reportError("there's no local setters for thruput chase for ext setter\n");
        for (; ol; ol = ol->unk_00) {
            ol_s *n = tr->unk_20[part].unk_18.copyOperandLocation(ol, true);
            if (!n) {
                reportError("failed to add an external setter\n");
                return false;
            }
            n->unk_08 = key;
        }
        ol = registerPart(tr, part)->unk_20.head;
        if (!ol) {
            reportError("there's no local setters.. for a thruput chase .. for target\n");
            return false;
        }
        for (; ol; ol = ol->unk_00) {
            ol_s *n = sr->unk_20[part].unk_28.copyOperandLocation(ol, true);
            if (!n) {
                reportError("failed to add post setter!\n");
                return false;
            }
            n->unk_08 = key;
        }
        if (isPathRaw(p, key)) {
            int dist = s->unk_240;
            int last = sr->unk_20[part].unk_3c[11];
            short *lastAddress = thruputTimes(sr, part);
            int tend = thruputTail(s, sr, part);
            int fst = tr->unk_20[part].unk_3c[7];
            if (fst < 0 || tend < 0 || dist < 0) {
                reportError("something wrong checking for thruput stalls fst=%d tend=%d path_dist=%d\n",
                              fst, tend, dist);
                return false;
            }
            dist = (dist + tend) + fst;
            last = lastAddress[1];
            if (dist < last) {
                regUseSlot *slot = &sr->unk_20[part];
                ol_s *setter = slot->func_0805ba2c(0, true);
                int again = lastAddress[1];
                if (!p->proc.unk_74.addStall(setter, tr->unk_20[part].unk_20.head, 1,
                                        again - dist)) {
                    reportError("failed to add an interstream latency stall\n");
                    return false;
                }
            }
        }
    }
    return true;
}
