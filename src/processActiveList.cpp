
#include "backTrace_shared.h"

bool processActiveList(Program *p)
{
    p->unk_d8 = 0;
    while (p->unk_e4) {
        instStream *s = p->unk_e4;
        p->unk_e4 = s->unk_25c;
        if (s->unk_226 != 1) {
            func_0804ad38("an inconsistency in the active list?\n");
            return false;
        }
        if (s->unk_224 != 0) {
            s->unk_226 = 3;
            if (s->unk_254) {
                func_0804ad38("new match is already set\n");
                return false;
            }
            s->unk_254 = p->unk_d8;
            p->unk_d8 = s;
            if (s->unk_250) {
                func_0804ad38("new match is already set\n");
                return false;
            }
            s->unk_250 = p->unk_dc;
            p->unk_dc = s;
        } else {
            linkNode *head = s->unk_128.unk_00;
            if (head) {
                s->unk_226 = 2;
                if (!extendBackTrace(p, s)) {
                    func_0804ad38("failed to extend back trace at %s\n", s->func_0806d674());
                    return false;
                }
            } else {
                s->unk_226 = 4;
            }
        }
    }
    return true;
}

bool extendBackTrace(Program *p, instStream *n)
{
    int cnt = 0;
    if (n->unk_226 != 2 && n->unk_226 != 0) {
        func_0804ad38("extend: called - but not visited or unvisited\n");
        return false;
    }
    linkNode *l;
    for (l = n->unk_128.unk_00; l; l = l->unk_08) {
        if (l->unk_14 == 3 || l->unk_14 == 4) {
            if (l->unk_10)
                break;
            cnt++;
        }
    }
    bool all = false;
    if (cnt && !l)
        all = true;

    for (l = n->unk_128.unk_00; l; l = l->unk_08) {
        int branch = 0;
        if (l->unk_14 == 2 || l->unk_14 == 4 || l->unk_14 == 5)
            branch = 1;
        if (branch) {
            instStream *prev = n->unk_10c;
            if (!prev) {
                func_0804ad38("to register branch had no previous\n");
                return false;
            }
            for (linkNode *b = prev->unk_134.unk_00; b; b = b->unk_08) {
                instStream *d = b->unk_0c;
                if (!d) {
                    func_0804ad38("no dest stream set for and link\n");
                    return false;
                }
                if (d->unk_226 == 2 && prev->unk_226 == 0) {
                    if (!addNewActive(p, prev, n)) {
                        func_0804ad38("failed to add a new active node for predecessor in bal case\n");
                        return false;
                    }
                    if (p->unk_b5 && mergeTstores(p, prev->unk_238, n->unk_238) < 0) {
                        func_0804ad38("failed to merge tstores\n");
                        return false;
                    }
                }
                linkNode *back = d->unk_128.func_0806dff4(prev, b->unk_14);
                if (!back) {
                    func_0804ad38("destination had no link back\n");
                    return false;
                }
                back->unk_10 = 1;
            }
        }
        if (all || l->unk_10) {
            instStream *o = l->unk_0c;
            if (!o) {
                func_0804ad38("can't get input stream for ir during trace\n");
                return false;
            }
            if (p->unk_b5 && mergeTstores(p, o->unk_238, n->unk_238) < 0) {
                func_0804ad38("failed to merge tstores\n");
                return false;
            }
            switch (o->unk_226) {
            case 0:
                if (!addNewActive(p, o, n)) {
                    func_0804ad38("failedt to add a new active\n");
                    return false;
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4: {
                int dmin = o->unk_240;
                if (dmin < 0) {
                    func_0804ad38("think I've already been here, but there's no distance to target\n");
                    return false;
                }
                int len = n->unk_240 + n->unk_22c;
                if (len < dmin) {
                    o->unk_244 = n;
                    if (!recalcMinTraceLengths(p)) {
                        func_0804ad38("failed to recalc trace lengths\n");
                        return false;
                    }
                    if (o->unk_240 != len) {
                        func_0804ad38("after recalculating the minimal path lengths, got an unexpected result\n");
                        return false;
                    }
                }
                if (p->unk_b4) {
                    int dmax = o->unk_248;
                    if (dmax < 0) {
                        func_0804ad38("think I've already been here, but there's no maximal distance to target\n");
                        return false;
                    }
                    bool notOnPath = true;
                    for (instStream *q = n; q; q = q->unk_24c) {
                        if (q == o) {
                            notOnPath = false;
                            break;
                        }
                    }
                    if (notOnPath) {
                        int len2 = n->unk_248 + n->unk_22c;
                        if (len2 > dmax) {
                            o->unk_24c = n;
                            if (!recalcMaxTraceLengths(p)) {
                                func_0804ad38("failed to recalc trace lengths\n");
                                return false;
                            }
                            if (o->unk_248 != len2) {
                                func_0804ad38("after recalculating the maximal path lengths, got an unexpected result\n");
                                return false;
                            }
                        }
                    }
                }
                break;
            }
            default:
                func_0804ad38("bad visit mode\n");
                return false;
            }
        }
    }
    return true;
}

bool addNewActive(Program *p, instStream *n, instStream *from)
{
    if (n->unk_226) {
        func_0804ad38("addNewActive called - but ist_n(%s) is not unvisited.ist_o is %s\n",
                      n->func_0806d674(), from->func_0806d674());
        return false;
    }
    if (n->unk_25c) {
        func_0804ad38("can't add to active list\n");
        return false;
    }
    n->unk_25c = p->unk_e4;
    p->unk_e4 = n;
    n->unk_226 = 1;
    n->unk_244 = from;
    n->unk_240 = from->unk_240 + from->unk_22c;
    if (p->unk_b4) {
        n->unk_24c = from;
        n->unk_248 = from->unk_248 + from->unk_22c;
    }
    return true;
}

bool recalcMinTraceLengths(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (!s->unk_226)
            continue;
        int len = 0;
        for (instStream *q = s->unk_244; q; q = q->unk_244)
            len += q->unk_22c;
        if (s->unk_240 < len) {
            func_0804ad38("recalcPaths - code produced a longer path.. but should be tracking shortest ..path is %s nLen=%d, but old length was %d!\n",
                          s->func_0806d674(), len, s->unk_240);
            return false;
        }
        s->unk_240 = len;
    }
    return true;
}

bool recalcMaxTraceLengths(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (!s->unk_226)
            continue;
        int len = 0;
        for (instStream *q = s->unk_24c; q; q = q->unk_24c)
            len += q->unk_22c;
        if (s->unk_248 > len) {
            func_0804ad38("recalcPaths - code produced a shorter maximal path.. but should be tracking longest ..path is %s nLen=%d, but old length was %d!\n",
                          s->func_0806d674(), len, s->unk_248);
            return false;
        }
        s->unk_248 = len;
    }
    return true;
}

bool func_08061c34(instStream *n);

bool func_08061ab0(instStream *n, RegPoolEntry *reg, int part)
{
    if (!n->unk_218) {
        func_0804ad38("work table not set in %s\n", n->func_0806d674());
        func_0804adf0(-1);
    }
    workTable *wt = n->unk_218;
    if (!wt) {
        func_0804ad38("failed to get work table\n");
        return 0;
    }
    regUse *u = wt->unk_14.func_0805b5dc(reg);
    n->unk_227 = 0;
    n->unk_228 = 0;
    n->unk_258 = 0;
    n->unk_234 = u;
    if (u && ((u->unk_0f >> part) & 1)) {
        if (wt->unk_09 == 1 || wt->unk_09 == 4) {
            n->unk_224 = 1;
            n->unk_230 = 0;
            n->unk_232 = 0;
        } else {
            int count = u->unk_20[part].unk_08.unk_04;
            n->unk_224 = count;
            ol_s *head = u->unk_20[part].unk_08.unk_00;
            if (!head) {
                func_0804ad38("visible outputs has no head for l_init.. but it should!\n");
                return 0;
            }
            int lo = head->unk_12;
            n->unk_230 = lo;
            ol_s *last = u->unk_20[part].func_0805ba2c(0, true);
            if (!last) {
                func_0804ad38("visible outputs had nothing visible at infinity\n");
                return 0;
            }
            int hi = last->unk_12;
            n->unk_232 = hi;
            if (n->unk_230 > n->unk_232) {
                func_0804ad38("minimal writeVal was > maximal writeVal\n");
                return 0;
            }
        }
    } else {
        n->unk_224 = 0;
        n->unk_230 = -1;
        n->unk_232 = -2;
    }
    return func_08061c34(n);
}

bool func_08061c34(instStream *n)
{
    n->unk_226 = 0;
    for (linkNode *l = n->unk_128.unk_00; l; l = l->unk_08)
        l->unk_10 = l->unk_14 != 3 && l->unk_14 != 4;
    n->unk_240 = -1;
    n->unk_244 = 0;
    n->unk_248 = -1;
    n->unk_24c = 0;
    n->unk_250 = 0;
    n->unk_254 = 0;
    n->unk_25c = 0;
    return true;
}

int func_08061cb0(Program *p, instStream *n)
{
    VisitCache *vc = &p->unk_bc;
    if (!vc->startRecord()) {
        func_0804ad38("failed to start a record\n");
        return 0;
    }
    if (!vc->setBit(p->unk_d4->unk_2c0)) {
        func_0804ad38("failed to add target to min path key\n");
        return 0;
    }
    for (; n; n = n->unk_244) {
        if (!vc->setBit(n->unk_2c0)) {
            func_0804ad38("failed to add to to min path record\n");
            return 0;
        }
    }
    return vc->endRecord();
}

int getMaxPathKey(Program *p, instStream *n)
{
    VisitCache *vc = &p->unk_bc;
    if (!vc->startRecord()) {
        func_0804ad38("failed to start a record\n");
        return 0;
    }
    if (!vc->setBit(p->unk_d4->unk_2c0)) {
        func_0804ad38("failed to add target to max path key\n");
        return 0;
    }
    for (; n; n = n->unk_24c) {
        if (!vc->setBit(n->unk_2c0)) {
            func_0804ad38("failed to add to record to max path\n");
            return 0;
        }
    }
    return vc->endRecord();
}

bool isPathRaw(Program *p, int key)
{
    if (key <= 0) {
        func_0804ad38("<=0 input to isPathRaw\n");
        func_0804adf0(-1);
    }
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        int id = s->unk_2c0;
        if (p->unk_bc.testBit(key, id) && s->func_0806d908() == 1)
            return false;
    }
    return true;
}

int guessKeyLen(Program *p, int key, int start, int end)
{
    instStream *s = p->unk_a0;
    int len = 0;
    bool foundStart = false;
    bool foundEnd = false;
    for (; s; s = s->unk_108) {
        int id = s->unk_2c0;
        if (p->unk_bc.testBit(key, id)) {
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
        func_0804ad38("guessKeyLen - didn't contain one of start or end\n");
        return -1;
    }
    return len;
}

void dumpMinPathToTarget(instStream *s)
{
    func_0804acb0("%s  minPathToTarget\n", s->func_0806d674());
    int i = 0;
    for (instStream *q = s->unk_244; q; q = q->unk_244)
        func_0804acb0("    %2d:   [%3d] %s\n", ++i, q->unk_240, q->func_0806d674());
    func_0804acb0("done .. %s  minPathToTarget\n", s->func_0806d674());
}

void dumpMaxPathToTarget(instStream *s)
{
    func_0804acb0("%s  maxPathToTarget\n", s->func_0806d674());
    int i = 0;
    for (instStream *q = s->unk_24c; q; q = q->unk_24c)
        func_0804acb0("    %2d:   [%3d] %s\n", ++i, q->unk_248, q->func_0806d674());
    func_0804acb0("done .. %s  maxPathToTarget\n", s->func_0806d674());
}
