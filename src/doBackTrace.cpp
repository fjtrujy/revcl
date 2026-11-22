#include "os.h"
#include "backTrace_shared.h"

static inline bool zeroTstore_inl(Program *p, unsigned *t)
{
    if (!t) {
        reportError("zeroTstore - it's null\n");
        return false;
    }
    for (int i = 0; i != p->unk_b8; i++)
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
    if (idx >= (unsigned)p->unk_b8) {
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
    if (idx >= (unsigned)p->unk_b8) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}

static inline void dumpBackTrace(Program *p)
{
    func_0806ed78(p, 0);
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
    p->unk_b4 = latency && !blocking;
    p->unk_b5 = latency && !blocking;
    p->unk_d4 = t;
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!func_08061ab0(s, reg, part)) {
            reportError("failed to do a local init for the back trace\n");
            return false;
        }
        if (p->unk_b5 && !initTstore_inl(p, s->unk_238, s->unk_2c0)) {
            reportError("failed to init tstore\n");
            return false;
        }
    }
    p->unk_dc = 0;
    p->unk_e0 = 0;
    p->unk_e4 = 0;
    if (!extendBackTrace(p, t)) {
        reportError("failed on the initial extend\n");
        return false;
    }
    for (instStream *s = p->unk_e4; s; s = s->unk_25c) {
        if (p->unk_b5 && !initTstore_inl(p, s->unk_238, s->unk_2c0)) {
            reportError("clear a tstore after first init\n");
            return false;
        }
        s->unk_244 = 0;
        s->unk_24c = 0;
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
            if (p->unk_e4) {
                reportError("in blocking mode , things got added to the active list\n");
                return false;
            }
        } else {
            while (p->unk_e4) {
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
            if (s->unk_226 == 4) {
                reportInfo("UBS: %s %s.%d\n", p->unk_d4->streamName(), reg->registerName(), part);
                if (!p->proc.usedBeforeSet.addUBS(s, p->unk_d4, reg, bit)) {
                    reportError("failed to add ubs\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = p->streamHead; s; s = s->nextStream) {
            if (s->unk_226 == 2 || s->unk_226 == 4) {
                if (!s->unk_234) {
                    workTable *wt = s->getTable();
                    if (!wt) {
                        reportError("no work table\n");
                        return false;
                    }
                    s->unk_234 = wt->unk_14.AddNewRegRef(reg);
                    if (!s->unk_234) {
                        reportError("couldn't add regref to table\n");
                        return false;
                    }
                }
                s->unk_234->unk_10 |= 1 << part;
                s->unk_227 = 1;
            }
        }
        if (!blocking) {
        for (instStream *k = p->streamHead; k; k = k->nextStream) {
            if (!k->unk_228)
                continue;
            instStream *n;
            for (n = p->streamHead; n; n = n->nextStream)
                n->unk_229 = 0;
            k->unk_229 = 1;
            bool changed;
            do {
                changed = false;
                for (n = p->streamHead; n; n = n->nextStream) {
                    if (n->unk_229)
                        continue;
                    n->unk_229 = 1;
                    if (testTstore_inl(p, k->unk_238, n->unk_2c0)) {
                        regUse *u = n->unk_234;
                        if (!u) {
                            reportError("couldn't get regref for hazard broadcast op for reg %s.%d in %s hazard start=%s target=%s mode=%d\n",
                                          reg->registerName(), part, n->streamName(), k->streamName(),
                                          t->streamName(), n->unk_226);
                            dumpBackTrace(p);
                            return false;
                        }
                        u->unk_14 |= 1 << part;
                        int r = mergeTstores(p, k->unk_238, n->unk_238);
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
                s->unk_258 = p->unk_e0;
                p->unk_e0 = s;
                int key = func_08061cb0(p, s);
                s->unk_23c = key;
                if (key < 0) {
                    reportError("failed to get minimal path for localT tracker\n");
                    return false;
                }
            }
        }
        for (instStream *s = p->unk_e0; s; s = s->unk_258) {
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
    for (int i = 0; i != p->unk_b8; i++) {
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
    for (int i = 0; i != p->unk_b8; i++)
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
    if (idx >= (unsigned)p->unk_b8) {
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
    if (idx >= (unsigned)p->unk_b8) {
        reportError("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}
