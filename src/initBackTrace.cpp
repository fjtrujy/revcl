#include "os.h"

#include "backTrace_shared.h"

bool initBackTrace(Program *p)
{
    int mx = -1;
    instStream *s;
    for (s = p->streamHead; s; s = s->unk_108) {
        if (s->unk_2c0 < 0) {
            reportError("flowUID<0 for g_initBackTrace\n");
            return false;
        }
        if (s->unk_2c0 > mx)
            mx = s->unk_2c0;
    }
    if (mx < 0) {
        reportError("mxUID was 0... nothing to backtrace ??\n");
        return false;
    }
    int n = mx + 1;
    for (s = p->streamHead; s; s = s->unk_108) {
        if (!g_initBackTrace(s, n)) {
            reportError("[Depend] failed to g_initBackTrace a stream\n");
            return false;
        }
    }
    p->unk_b8 = (n + 31) >> 2;
    if (!p->unk_bc.init(n)) {
        reportError("flowMin::flowReset - failed to init visit cache\n");
        return false;
    }
    if (!p->unk_bc.startRecord()) {
        reportError("flowReset: failed to start record\n");
        return false;
    }
    int key = p->unk_bc.endRecord();
    if (key != 0) {
        reportError("flowMon::flowReset - got an unexpected key back for null bit vector (%d)\n", key);
        return false;
    }
    return true;
}

bool disposeBackTrace(Program *p)
{
    p->unk_b8 = 0;
    for (instStream *s = p->streamHead; s; s = s->unk_108) {
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
        reportError("ist::g_initBackTrace - failed to get or guess the [depend] length of this stream\n");
        return false;
    }
    int len = s->getOrGuessLogicalLength();
    if (len < 0) {
        reportError("ist::g_initBackTrace - failed to get or guess the logical length of this stream\n");
        return false;
    }
    if (len > s->unk_22c) {
        reportError("minimal length > dep length for %s (%d > %d)\n", s->streamName(), len, s->unk_22c);
        return false;
    }
    if (s->unk_2cd == 2)
        s->unk_22c += 1000;
    if (s->unk_238) {
        reportError("m_tStore is already set !\n");
        return false;
    }
    if (!n) {
        reportError("no t store entries\n");
        return false;
    }
    s->unk_238 = new unsigned[(n + 31) / 4];
    if (!s->unk_238) {
        reportError("failed to get new tSTore\n");
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

