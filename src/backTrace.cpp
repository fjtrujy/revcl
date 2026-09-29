#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"
#include "workTable.h"
#include "RegPool.h"

bool g_initBackTrace(instStream *s, int n);
int func_08061cb0(Program *p, instStream *n);
bool func_08061e00(Program *p, int key);

void dumpPathKey(Program *p, int key)
{
    func_0804acb0("dumping path key '%d'\n", key);
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (p->unk_bc.testBit(key, s->unk_2c0))
            func_0804acb0("    %d: %s\n", s->func_0806d674());
    }
    func_0804acb0(" done dumping path key '%d'\n", key);
}

bool initBackTrace(Program *p)
{
    int mx = -1;
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_2c0 < 0) {
            func_0804ad38("flowUID<0 for g_initBackTrace\n");
            return false;
        }
        if (s->unk_2c0 > mx)
            mx = s->unk_2c0;
    }
    if (mx < 0) {
        func_0804ad38("mxUID was 0... nothing to backtrace ??\n");
        return false;
    }
    int n = mx + 1;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (!g_initBackTrace(s, n)) {
            func_0804ad38("[Depend] failed to g_initBackTrace a stream\n");
            return false;
        }
    }
    p->unk_b8 = (n + 31) >> 2;
    if (!p->unk_bc.init(n)) {
        func_0804ad38("flowMin::flowReset - failed to init visit cache\n");
        return false;
    }
    if (!p->unk_bc.startRecord()) {
        func_0804ad38("flowReset: failed to start record\n");
        return false;
    }
    int key = p->unk_bc.endRecord();
    if (key != 0) {
        func_0804ad38("flowMon::flowReset - got an unexpected key back for null bit vector (%d)\n", key);
        return false;
    }
    return true;
}

bool disposeBackTrace(Program *p)
{
    p->unk_b8 = 0;
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_238)
            delete[] s->unk_238;
        s->unk_238 = 0;
    }
    return true;
}

bool g_initBackTrace(instStream *s, int n)
{
    s->unk_22c = s->getOrGuessDepLength();
    if (s->unk_22c < 0) {
        func_0804ad38("ist::g_initBackTrace - failed to get or guess the [depend] length of this stream\n");
        return false;
    }
    int len = s->getOrGuessLogicalLength();
    if (len < 0) {
        func_0804ad38("ist::g_initBackTrace - failed to get or guess the logical length of this stream\n");
        return false;
    }
    if (len > s->unk_22c) {
        func_0804ad38("minimal length > dep length for %s (%d > %d)\n", s->func_0806d674(), len, s->unk_22c);
        return false;
    }
    if (s->unk_2cd == 2)
        s->unk_22c += 1000;
    if (s->unk_238) {
        func_0804ad38("m_tStore is already set !\n");
        return false;
    }
    if (!n) {
        func_0804ad38("no t store entries\n");
        return false;
    }
    s->unk_238 = new unsigned[(n + 31) / 4];
    if (!s->unk_238) {
        func_0804ad38("failed to get new tSTore\n");
        return false;
    }
    return true;
}

bool g_disposeBackTrace(instStream *s)
{
    if (s->unk_238)
        delete[] s->unk_238;
    s->unk_238 = 0;
    return true;
}

bool addThruputStalls(Program *p, instStream *t, RegPoolEntry *reg, int part)
{
    regUse *tr = t->unk_234;
    if (!tr) {
        func_0804ad38("bThru enabled - but no btRegRef in the target!\n");
        return false;
    }
    for (instStream *s = p->unk_dc; s; s = s->unk_250) {
        regUse *sr = s->unk_234;
        if (!sr) {
            func_0804ad38("no btRegRef for something on the match list %s.%d target=%s stream is %s!\n",
                          reg->func_0805a8e0(), part, t->func_0806d674(), s->func_0806d674());
            return false;
        }
        int key = func_08061cb0(p, s);
        if (key <= 0) {
            func_0804ad38("minimal path key was bad\n");
            return false;
        }
        ol_s *ol = sr->unk_20[part].unk_20.unk_00;
        if (!ol)
            func_0804ad38("there's no local setters for thruput chase for ext setter\n");
        for (; ol; ol = ol->unk_00) {
            ol_s *n = tr->unk_20[part].unk_18.func_08058348(ol, true);
            if (!n) {
                func_0804ad38("failed to add an external setter\n");
                return false;
            }
            n->unk_08 = key;
        }
        ol = tr->unk_20[part].unk_20.unk_00;
        if (!ol) {
            func_0804ad38("there's no local setters.. for a thruput chase .. for target\n");
            return false;
        }
        for (; ol; ol = ol->unk_00) {
            ol_s *n = sr->unk_20[part].unk_28.func_08058348(ol, true);
            if (!n) {
                func_0804ad38("failed to add post setter!\n");
                return false;
            }
            n->unk_08 = key;
        }
        if (func_08061e00(p, key)) {
            int dist = s->unk_240;
            int tend = s->unk_22c - sr->unk_20[part].unk_3c[10];
            int fst = tr->unk_20[part].unk_3c[7];
            if (fst < 0 || tend < 0 || dist < 0) {
                func_0804ad38("something wrong checking for thruput stalls fst=%d tend=%d path_dist=%d\n",
                              fst, tend, dist);
                return false;
            }
            int total = fst + (tend + dist);
            if (total < sr->unk_20[part].unk_3c[11]) {
                ol_s *setter = sr->unk_20[part].func_0805ba2c(0, true);
                if (!p->unk_74.addStall(setter, tr->unk_20[part].unk_20.unk_00, 1,
                                        sr->unk_20[part].unk_3c[11] - total)) {
                    func_0804ad38("failed to add an interstream latency stall\n");
                    return false;
                }
            }
        }
    }
    return true;
}
