#include "os.h"

#include "backTrace_shared.h"

bool processActiveList(Program *p)
{
    p->unk_d8 = 0;
    while (p->unk_e4) {
        instStream *s = p->unk_e4;
        p->unk_e4 = s->unk_25c;
        if (s->unk_226 != 1) {
            reportError("an inconsistency in the active list?\n");
            return false;
        }
        if (s->unk_224 != 0) {
            s->unk_226 = 3;
            if (s->unk_254) {
                reportError("new match is already set\n");
                return false;
            }
            s->unk_254 = p->unk_d8;
            p->unk_d8 = s;
            if (s->unk_250) {
                reportError("new match is already set\n");
                return false;
            }
            s->unk_250 = p->unk_dc;
            p->unk_dc = s;
        } else {
            linkNode *head = s->unk_128.unk_00;
            if (head) {
                s->unk_226 = 2;
                if (!extendBackTrace(p, s)) {
                    reportError("failed to extend back trace at %s\n", s->streamName());
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
        reportError("extend: called - but not visited or unvisited\n");
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
        int branch = l->unk_14 == 2 || l->unk_14 == 4 || l->unk_14 == 5;
        if ((char)branch) {
            instStream *prev = n->previousStream;
            if (!prev) {
                reportError("to register branch had no previous\n");
                return false;
            }
            for (linkNode *b = prev->unk_134.unk_00; b; b = b->unk_08) {
                instStream *d = b->unk_0c;
                int type = b->unk_14;
                if (!d) {
                    reportError("no dest stream set for and link\n");
                    return false;
                }
                if (d->unk_226 == 2 && prev->unk_226 == 0) {
                    if (!addNewActive(p, prev, n)) {
                        reportError("failed to add a new active node for predecessor in bal case\n");
                        return false;
                    }
                    if (p->unk_b5 && mergeTstores(p, prev->unk_238, n->unk_238) < 0) {
                        reportError("failed to merge tstores\n");
                        return false;
                    }
                    type = b->unk_14;
                }
                linkNode *back = d->unk_128.func_0806dff4(prev, type);
                if (!back) {
                    reportError("destination had no link back\n");
                    return false;
                }
                back->unk_10 = 1;
            }
        }
        if (all || l->unk_10) {
            instStream *o = l->unk_0c;
            if (!o) {
                reportError("can't get input stream for ir during trace\n");
                return false;
            }
            if (p->unk_b5 && mergeTstores(p, o->unk_238, n->unk_238) < 0) {
                reportError("failed to merge tstores\n");
                return false;
            }
            switch (o->unk_226) {
            case 0:
                if (!addNewActive(p, o, n)) {
                    reportError("failedt to add a new active\n");
                    return false;
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4: {
                int dmin = o->unk_240;
                if (dmin < 0) {
                    reportError("think I've already been here, but there's no distance to target\n");
                    return false;
                }
                int len = n->unk_240 + n->unk_22c;
                if (len < dmin) {
                    o->unk_244 = n;
                    if (!recalcMinTraceLengths(p)) {
                        reportError("failed to recalc trace lengths\n");
                        return false;
                    }
                    if (o->unk_240 != len) {
                        reportError("after recalculating the minimal path lengths, got an unexpected result\n");
                        return false;
                    }
                }
                if (p->unk_b4) {
                    int dmax = o->unk_248;
                    if (dmax < 0) {
                        reportError("think I've already been here, but there's no maximal distance to target\n");
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
                                reportError("failed to recalc trace lengths\n");
                                return false;
                            }
                            if (o->unk_248 != len2) {
                                reportError("after recalculating the maximal path lengths, got an unexpected result\n");
                                return false;
                            }
                        }
                    }
                }
                break;
            }
            default:
                reportError("bad visit mode\n");
                return false;
            }
        }
    }
    return true;
}

bool addNewActive(Program *p, instStream *n, instStream *from)
{
    if (n->unk_226) {
        reportError("addNewActive called - but ist_n(%s) is not unvisited.ist_o is %s\n",
                      n->streamName(), from->streamName());
        return false;
    }
    if (n->unk_25c) {
        reportError("can't add to active list\n");
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
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!s->unk_226)
            continue;
        int len = 0;
        for (instStream *q = s->unk_244; q; q = q->unk_244)
            len += q->unk_22c;
        if (s->unk_240 < len) {
            reportError("recalcPaths - code produced a longer path.. but should be tracking shortest ..path is %s nLen=%d, but old length was %d!\n",
                          s->streamName(), len, s->unk_240);
            return false;
        }
        s->unk_240 = len;
    }
    return true;
}

bool recalcMaxTraceLengths(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!s->unk_226)
            continue;
        int len = 0;
        for (instStream *q = s->unk_24c; q; q = q->unk_24c)
            len += q->unk_22c;
        if (s->unk_248 > len) {
            reportError("recalcPaths - code produced a shorter maximal path.. but should be tracking longest ..path is %s nLen=%d, but old length was %d!\n",
                          s->streamName(), len, s->unk_248);
            return false;
        }
        s->unk_248 = len;
    }
    return true;
}

