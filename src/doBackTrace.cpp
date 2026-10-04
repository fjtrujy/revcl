#include "backTrace_shared.h"

static inline void dumpBackTrace(Program *p)
{
    func_0806ed78(p, 0);
}

bool doBackTrace(Program *p, instStream *t, RegPoolEntry *reg, int part, int earliestUse, int ioMode, bool thruput)
{
    if (!t || !reg) {
        func_0804ad38("doBackTrace - null arg\n");
        return false;
    }
    if (ioMode && earliestUse < 0) {
        func_0804ad38("-ve earliestUse passed into doBackTrace %s.%d at %s iomode=%d\n",
                      reg->func_0805a8e0(), part, t->func_0806d674(), ioMode);
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
        func_0804ad38("bad ioMode for backtrace\n");
        return false;
    }
    p->unk_b4 = latency && !blocking;
    p->unk_b5 = latency && !blocking;
    p->unk_d4 = t;
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (!func_08061ab0(s, reg, part)) {
            func_0804ad38("failed to do a local init for the back trace\n");
            return false;
        }
        if (p->unk_b5 && !initTstore(p, s->unk_238, s->unk_2c0)) {
            func_0804ad38("failed to init tstore\n");
            return false;
        }
    }
    p->unk_dc = 0;
    p->unk_e0 = 0;
    p->unk_e4 = 0;
    if (!extendBackTrace(p, t)) {
        func_0804ad38("failed on the initial extend\n");
        return false;
    }
    for (instStream *s = p->unk_e4; s; s = s->unk_25c) {
        if (p->unk_b5 && !initTstore(p, s->unk_238, s->unk_2c0)) {
            func_0804ad38("clear a tstore after first init\n");
            return false;
        }
        s->unk_244 = 0;
        s->unk_24c = 0;
        s->unk_240 = 0;
        s->unk_248 = 0;
    }
    if (!processActiveList(p)) {
        func_0804ad38("the initial trace failed\n");
        return false;
    }
    if (thruput && !addThruputStalls(p, t, reg, part)) {
        func_0804ad38("internal thruput check failed\n");
        return false;
    }
    if (latency) {
        if (!linkMatchList(p, t, reg, part, earliestUse, blocking)) {
            func_0804ad38("internal latency check failed\n");
            return false;
        }
        if (blocking) {
            if (p->unk_e4) {
                func_0804ad38("in blocking mode , things got added to the active list\n");
                return false;
            }
        } else {
            while (p->unk_e4) {
                if (!processActiveList(p)) {
                    func_0804ad38("failed to continue back trace\n");
                    return false;
                }
                if (!linkMatchList(p, t, reg, part, earliestUse, blocking)) {
                    func_0804ad38("internal latency check failed\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = p->unk_a0; s; s = s->unk_108) {
            unsigned char bit = 1 << part;
            if (s->unk_226 == 4) {
                func_0804acb0("UBS: %s %s.%d\n", p->unk_d4->func_0806d674(), reg->func_0805a8e0(), part);
                if (!p->proc.unk_68.addUBS(s, p->unk_d4, reg, bit)) {
                    func_0804ad38("failed to add ubs\n");
                    return false;
                }
            }
        }
    }
    if (latency) {
        for (instStream *s = p->unk_a0; s; s = s->unk_108) {
            if (s->unk_226 == 2 || s->unk_226 == 4) {
                if (!s->unk_234) {
                    workTable *wt = s->getTable();
                    if (!wt) {
                        func_0804ad38("no work table\n");
                        return false;
                    }
                    s->unk_234 = wt->unk_14.AddNewRegRef(reg);
                    if (!s->unk_234) {
                        func_0804ad38("couldn't add regref to table\n");
                        return false;
                    }
                }
                s->unk_234->unk_10 |= 1 << part;
                s->unk_227 = 1;
            }
        }
        if (!blocking) {
        for (instStream *k = p->unk_a0; k; k = k->unk_108) {
            if (!k->unk_228)
                continue;
            instStream *n;
            for (n = p->unk_a0; n; n = n->unk_108)
                n->unk_229 = 0;
            k->unk_229 = 1;
            bool changed;
            do {
                changed = false;
                for (n = p->unk_a0; n; n = n->unk_108) {
                    if (n->unk_229)
                        continue;
                    n->unk_229 = 1;
                    if (testTstore(p, k->unk_238, n->unk_2c0)) {
                        regUse *u = n->unk_234;
                        if (!u) {
                            func_0804ad38("couldn't get regref for hazard broadcast op for reg %s.%d in %s hazard start=%s target=%s mode=%d\n",
                                          reg->func_0805a8e0(), part, n->func_0806d674(), k->func_0806d674(),
                                          t->func_0806d674(), n->unk_226);
                            dumpBackTrace(p);
                            return false;
                        }
                        u->unk_14 |= 1 << part;
                        int r = mergeTstores(p, k->unk_238, n->unk_238);
                        if (r < 0) {
                            func_0804ad38("failed to merge  stores\n");
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
        for (instStream *s = p->unk_a0; s; s = s->unk_108) {
            if (s->unk_227) {
                s->unk_258 = p->unk_e0;
                p->unk_e0 = s;
                int key = func_08061cb0(p, s);
                s->unk_23c = key;
                if (key < 0) {
                    func_0804ad38("failed to get minimal path for localT tracker\n");
                    return false;
                }
            }
        }
        for (instStream *s = p->unk_e0; s; s = s->unk_258) {
            if (!int_buildTpairs(p, reg, part, s, t))
                func_0804ad38("failed to build tpairs for register %s with target %s at stream %s\n",
                              reg->func_0805a8e0(), t->func_0806d674(), s->func_0806d674());
        }
        }
    }
    return true;
}

int mergeTstores(Program *p, unsigned *dst, unsigned *src)
{
    if (!src || !dst) {
        func_0804ad38("mergetTstores - null input\n");
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
