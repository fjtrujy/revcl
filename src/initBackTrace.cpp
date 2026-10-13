
#include "backTrace_shared.h"

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

