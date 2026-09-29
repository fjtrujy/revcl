#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"
#include "workTable.h"
#include "RegPool.h"

bool g_initBackTrace(instStream *s, int n);
bool extendBackTrace(Program *p, instStream *n);
bool addNewActive(Program *p, instStream *n, instStream *from);
bool recalcMinTraceLengths(Program *p);
bool recalcMaxTraceLengths(Program *p);
int mergeTstores(Program *p, unsigned *dst, unsigned *src);
int func_08061cb0(Program *p, instStream *n);
int func_08061d58(Program *p, instStream *n);
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

bool linkMatchList(Program *p, instStream *t, RegPoolEntry *reg, int part, int ofs, bool ext)
{
    regUse *tr = t->unk_234;
    if (!tr) {
        func_0804ad38("no rfu ?\n");
        return false;
    }
    for (instStream *s = p->unk_d8; s; s = s->unk_254) {
        if (s->unk_226 != 3) {
            func_0804ad38("not end_match on the newmatchlist\n");
            return false;
        }
        int minKey = func_08061cb0(p, s);
        if (minKey <= 0) {
            func_0804ad38("minimal path key was bad\n");
            return false;
        }
        int maxKey = -1;
        if (p->unk_b4) {
            maxKey = func_08061d58(p, s);
            if (maxKey <= 0) {
                func_0804ad38("error getting max path key\n");
                return false;
            }
        }
        regUse *sr = s->unk_234;
        if (!sr) {
            func_0804ad38("an entry on the new match list had no regref?\n");
            return false;
        }
        if (s->unk_226 != 3) {
            func_0804ad38("newmatchlist had non -end-match \n");
            return false;
        }
        int d = s->unk_230 - (s->unk_240 + ofs);
        bool extended = false;
        if (d > 0) {
            if (ext) {
                if (!p->unk_74.addStall(sr->unk_20[part].unk_08.unk_00,
                                        tr->unk_20[part].unk_00.unk_00, 0, d)) {
                    func_0804ad38("failed to add an external stall\n");
                    return false;
                }
            } else {
                s->unk_226 = 2;
                extended = true;
                if (!extendBackTrace(p, s)) {
                    func_0804ad38("failed to extend this back\n");
                    return false;
                }
            }
        }
        for (ol_s *ol = tr->unk_20[part].unk_00.unk_00; ol; ol = ol->unk_00) {
            ol_s *last = sr->unk_20[part].func_0805ba2c(0, true);
            ol_s *match = 0;
            if (ext)
                match = last;
            else {
                int mn = s->unk_240;
                int mx = s->unk_248;
                if (mx < mn) {
                    func_0804ad38("max_ilen<min_ilen for a non-blocking instruction match.. %d < %d and logmax is %d\n",
                                  mx, mn, p->unk_b4);
                    return false;
                }
                if (mn < 0) {
                    func_0804ad38("minimal length<0\n");
                    return false;
                }
                int pos = ol->unk_04->unk_1394;
                if (pos < 0) {
                    func_0804ad38("instruction not inserted or emu'd??\n");
                    return false;
                }
                ol_s *min_s = sr->unk_20[part].func_0805ba2c(pos + mn, false);
                ol_s *max_s = sr->unk_20[part].func_0805ba2c(pos + mx, false);
                if (min_s) {
                    if (!max_s) {
                        func_0804ad38("min_s set, but couldn't find max_s\n");
                        return false;
                    }
                    if (min_s != max_s) {
                        unsigned char mode = t->func_0806d968();
                        if (mode >= 2 && mode <= 4) {
                            func_0804ac7c("multiple same-sequence output operands match to a fixed stream %s..setting hazard flags\n",
                                          t->func_0806d674());
                            tr->unk_12 |= 1 << part;
                            sr->unk_13 |= 1 << part;
                        } else {
                            func_0804ac7c("vcl found a RAW stream with a non blocking operation for '%s' where by going down differing paths, an operand was set in different places..\n",
                                          reg->func_0805a8e0());
                            func_0804ac7c("this is a bit dodgy - min_ilen=%d.. max_ilen=%d.. target is %s, and match is  %s\n",
                                          mn, mx, t->func_0806d674(), s->func_0806d674());
                            func_0804ac7c("I can't really guarantee that the code is correct after this - but - since vcl would stop anyway here, let's carry on and see what happens... \n");
                            func_0804ac7c("carrying on.. fixed raw mode. carrying on..\n");
                            func_0804ac7c("paths - min..\n");
                            func_0804ac7c("well - setting hiBits and localH this time..\n");
                            tr->unk_12 |= 1 << part;
                            sr->unk_13 |= 1 << part;
                        }
                        s->unk_228 = 1;
                    }
                }
                if (min_s != last) {
                    s->unk_228 = 1;
                    sr->unk_13 |= 1 << part;
                    tr->unk_12 |= 1 << part;
                }
                if (min_s)
                    match = min_s;
                else if (max_s) {
                    func_0804ac7c("min_s not set.. but max_s is..so, taking that\n");
                    match = max_s;
                }
            }
            if (match) {
                func_0805859c(match);
                sr->unk_0d |= 1 << part;
                if (!link_oParts(match, ol, minKey, maxKey)) {
                    func_0804ad38("failed to link_oparts external\n");
                    match->func_08057fbc();
                    return false;
                }
            }
            if (!ext && !match && !extended) {
                func_0804ad38("non blocking, no match, but didn't extend\n");
                return false;
            }
        }
    }
    return true;
}

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
        } else if (s->unk_128.unk_00) {
            s->unk_226 = 2;
            if (!extendBackTrace(p, s)) {
                func_0804ad38("failed to extend back trace at %s\n", s->func_0806d674());
                return false;
            }
        } else {
            s->unk_226 = 4;
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
