#include "lock_shared.h"

bool cls0805c244::catLocks(cls0805bcc4 *a, cls0805bcc4 *b, int *s, int *e, int *end)
{
    bool found = false;
    int lsz = g_080b336c;
    if (a->unk_2e) {
        *s = a->unk_36 + lsz;
        *e = a->unk_3a + lsz;
        *end = a->unk_4e + lsz;
        found = true;
    }
    int bs = b->unk_36;
    int be = b->unk_3a;
    int bend = b->unk_4e;
    if (!found) {
        *s = bs;
        *e = be;
        *end = bend;
    } else {
        *s = bs;
        *e = be;
        if (bend > *end)
            *end = bend;
    }
    return true;
}

static inline bool isAAInputLock(cls0805bcc4 *in)
{
    return in->unk_19 == 2;
}

static inline bool sameLockOwner(cls0805bcc4 *a, cls0805bcc4 *b)
{
    return a->unk_24 == b->unk_24;
}

int cls0805c244::canMoveInputLock(cls0805bcc4 *in, short *pos, int slotBit)
{
    int p = *pos;
    cls0805bcc4 *lk = unk_1c;
    int lsz = g_080b336c;
    if (unk_29)
        return 0;
    if (isAAInputLock(in)) {
        func_0804ad38("canMoveInputLock - aa input\n");
        func_0804aeb0();
        return 0;
    } else {
        cls0805bcc4 *par = in->getParent();
        bool cat = false;
        int s, e, end;
        if (lsz && isRegLock(par)) {
            cls0805bcc4 *x = unk_0c[0];
            if (x->unk_2c) {
                if (sameLockOwner(in, x))
                    return 0;
                p += lsz;
                if (!catLocks(par, x, &s, &e, &end)) {
                    func_0804ad38("failed to concatenate reg ref with xo\n");
                    func_0804adf0(-1);
                }
                cat = true;
                par = x;
            } else if (par->unk_2e) {
                s = par->unk_36;
                e = par->unk_3a;
                end = par->unk_4e;
                if (p - in->unk_18 < s)
                    s = p - in->unk_18;
                if (p - in->unk_18 < e)
                    e = p - in->unk_18;
                if (p > end)
                    end = p;
                if (end - s >= lsz)
                    return 0x2000;
            } else {
                s = p - in->unk_18;
                e = p;
                end = p;
            }
        } else if (lsz && unk_28 && par->unk_16) {
            if (!catLocks(unk_20, par, &s, &e, &end)) {
                func_0804ad38("case B failed to cat xo\n");
                checkCounts();
                func_0804ad38("we are :\n");
                in->print();
                func_0804adf0(-1);
            }
            cat = true;
        } else {
            s = par->unk_36;
            e = par->unk_3a;
            end = par->unk_4e;
        }

        if (p - in->unk_18 < e)
            return 8;
        if (p <= end)
            return 0;
        end = p;
        if (lsz && end - s >= lsz)
            return 0x40000;
        int ws = s, we = end;
        wrapRangeLocal(ws, we);
        if (lk) do {
            lk->print();
            if (lk == par)
                continue;
            if (sameLockOwner(lk, in))
                continue;
            if (cat && isRegLock(lk))
                continue;
            int ls = lk->unk_34;
            int le = lk->unk_4c;
            wrapRangeLocal(ls, le);
            if (lk->unk_22) {
                if (ws < le && we >= ls)
                    return 0x10;
            } else if (func_0805db74(ws, we, ls, le))
                return 0x20;
        } while ((lk = lk->unk_08) != 0);
    }
    return 0;
}

int cls0805c244::canMoveOutputLock(cls0805bcc4 *out, short *pos, int slotBit)
{
    int ret;
    int lsz = g_080b336c;
    int p = ((cls08058718 *)pos)->unk_00;
    int sz = ((cls08058718 *)pos)->unk_04;
    int mp = p;
    ret = checkOverlap(out, pos, &mp);
    if (ret)
        return ret;
    int first = out->unk_15 ? p + 1 : out->unk_18 + p;
    int last = out->unk_18 + p;

    int lim;
    if (out->unk_20) {
        int a = out->unk_48, b = out->unk_4a;
        int d = out->unk_46;
        int hi = a, lo = b;
        if (a < b) {
            hi = b;
            lo = a;
        }
        d -= hi - lo;
        if (d > 0)
            hi += (d + 1) >> 1;
        int off = hi;
        if (hi > 0)
            off = hi - 1;
        lim = last + off;
    } else
        lim = first;

    if (out->unk_2e) {
        if (out->unk_42 > lim)
            lim = out->unk_42;
        if (last > out->unk_3e)
            return 2;
    }

    bool ext = false;
    if (out->unk_16) {
        for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
            if (lk == out)
                continue;
            if (first <= lk->unk_4e)
                return 0x1000;
        }
        if (lsz && unk_28) {
            cls0805bcc4 *h = unk_20;
            ext = true;
            if (h->unk_2e) {
                int hs = h->unk_36 + lsz;
                int he = h->unk_4e + lsz;
                if (hs < last)
                    return 2;
                if (he > lim)
                    lim = he;
            }
        }
    } else if (lsz && unk_28) {
        cls0805bcc4 *h = unk_20;
        if (h->unk_2e) {
            if (first < h->unk_4e)
                return 0x4000;
        } else
            ext = true;
    }
    if (lsz && lim - first > lsz)
        return 0x2000;

    int s = sz - p + first;
    int e = sz - p + lim;
    wrapRangeLocal(s, e);
    for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
        if (lk == out)
            continue;
        if (ext && lk->unk_2d == 2)
            continue;
        int ls = lk->unk_34;
        int le = lk->unk_4c;
        wrapRangeLocal(ls, le);
        if (func_0805db74(s, e, ls, le))
            return 2;
    }
    if (ret)
        return ret;
    if (unk_2a && !lsz && !out->unk_15) {
        cls0805bcc4 *x = unk_0c[0];
        if (!x) {
            func_0804ad38("external lock not set \n");
            func_0804aeb0();
        }
        if (x->unk_2c) {
            if (out->unk_32 == p)
                return 0;
            if (p > x->unk_32)
                return 0x200000;
        }
    }
    return 0;
}

cls0805e2e4::cls0805e2e4()
{
}

cls0805e2e4::~cls0805e2e4()
{
}

void cls0805e2e4::func_0805e2fc()
{
}

void cls0805e2e4::func_0805e304()
{
    unk_00 = unk_02 = -1;
    unk_04 = unk_06 = -1;
    unk_08 = unk_0c = -1;
    unk_10 = -1;
    unk_14 = -1;
}

void cls0805e2e4::func_0805e340()
{
    unk_00 = unk_02 = 0;
    unk_04 = unk_06 = 0;
    unk_08 = unk_0c = 0;
    unk_10 = 0;
    unk_14 = 0;
}

void cls0805e2e4::func_0805e37c(cls0805e2e4 *src)
{
    unk_00 = src->unk_00;
    unk_02 = src->unk_02;
    unk_04 = src->unk_04;
    unk_06 = src->unk_06;
    unk_08 = src->unk_08;
    unk_0c = src->unk_0c;
    unk_10 = src->unk_10;
    unk_14 = src->unk_14;
}

void cls0805e2e4::func_0805e3c0(cls0805e2e4 *o)
{
    if (o->unk_00 > unk_00) unk_00 = o->unk_00;
    if (o->unk_02 > unk_02) unk_02 = o->unk_02;
    if (o->unk_04 > unk_04) unk_04 = o->unk_04;
    if (o->unk_06 > unk_06) unk_06 = o->unk_06;
    if (o->unk_08 > unk_08) unk_08 = o->unk_08;
    if (o->unk_10 > unk_10) unk_10 = o->unk_10;
    if (o->unk_0c > unk_0c) unk_0c = o->unk_0c;
    if (o->unk_14 > unk_14) unk_14 = o->unk_14;
}

extern char g_080de2a0[];  // stall reason text buffer

const char *stallReasonStr(short why)
{
    const char *s = "dunno why";
    switch (why & 0xff) {
    case 1:
        s = "instruction latency";
        break;
    case 2:
        s = "instruction thruput";
        break;
    case 3:
        s = "number of upper instructions";
        break;
    case 4:
        s = "number of lower instructions";
        break;
    case 5:
        s = "can't loop";
        break;
    case 6:
        s = "dependency chain depth";
        break;
    case 7:
        s = "dependency chain height";
        break;
    case 8:
        s = "branch instruction dependency chain height";
        break;
    case 9:
        func_0804adbc(g_080de2a0, "issue pressure on %s", func_0806c6a0((char)((why & 0xff00) >> 8)));
        s = g_080de2a0;
        break;
    case 10:
        s = "circular dependency chain";
        break;
    }
    return s;
}

bool instStream::gatherStats()
{
    cls0805e2e4 *st = &unk_278;
    st->unk_00 = st->unk_02 = -1;
    st->unk_04 = st->unk_06 = -1;
    st->unk_08 = st->unk_0c = -1;
    st->unk_10 = -1;
    st->unk_14 = -1;
    int physSum = 0;
    int maxLat = 0;
    unk_2b4 = -1;
    unk_2b6 = -1;
    statCounts *sc = &unk_290;
    for (int i = 0; i != 9; i++)
        sc->unk_00[i] = 0;
    unk_2b8 = 0;
    unk_2ba = 0;
    workTable *wt = getTable();
    if (!wt) {
        func_0804ad38("failed to gather stats - no get work table\n");
        return false;
    }

    Instr *in;
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        cls0805e2e4 *is = &in->unk_13f0;
        is->unk_00 = is->unk_02 = 0;
        is->unk_04 = is->unk_06 = 0;
        is->unk_08 = is->unk_0c = 0;
        is->unk_10 = 0;
        is->unk_14 = 0;
        int logged = 0;
        if (in) {
            for (int j = 0; j != 9; j++) {
                bool b = (in->unk_138d >> j) & 1;
                if (b)
                    sc->unk_00[j] += 2;
            }
            logged = 1;
        }
        if ((unsigned char)logged == 0) {
            func_0804ad38("failed to log\n");
            return false;
        }
    }

    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r = (isRegisterOperand(op, 1) && op->unk_2b != 2) ? op->unk_18 : 0;
            if (r) {
                for (int p = 0; p != r->unk_28; p++) {
                    if ((op->unk_07 >> p) & 1) {
                        for (ol_s *ol = op->unk_2c[p].unk_54.unk_00; ol; ol = ol->unk_00) {
                            if (!ol->unk_04) {
                                func_0804ad38("failed to get setting instruction!\n");
                                return false;
                            }
                            if (iOwn(ol->unk_04)) {
                                Operand *so = func_0805802c(ol);
                                if (!so) {
                                    func_0804ad38("failed to get setting operand for instruction\n");
                                    return false;
                                }
                                int l = so->unk_0a + op->unk_29;
                                if (maxLat < l)
                                    maxLat = l;
                            }
                        }
                    }
                }
            }
        }
    }

    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r;
            if (isRegisterOperand(op, 1) && op->unk_2b != 2)
                r = op->unk_18;
            else
                r = 0;
            if (r) {
                for (int p = 0; p != r->unk_28; p++) {
                    if (((op->unk_07 & ~op->unk_27) >> p) & 1) {
                        if (op->unk_2c[p].unk_54.unk_04 != 1) {
                            func_0804ad38("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->unk_2c[p].unk_54.unk_00->unk_04;
                        Operand *so = func_0805802c(op->unk_2c[p].unk_54.unk_00);
                        int d = src->unk_13f0.unk_04 + so->unk_0a + op->unk_29;
                        int h = src->unk_13f0.unk_06 + 1;
                        if (d > in->unk_13f0.unk_04)
                            in->unk_13f0.unk_04 = d;
                        if (h > in->unk_13f0.unk_06)
                            in->unk_13f0.unk_06 = h;
                    }
                }
            }
        }
    }

    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r = (isRegisterOperand(op, 1) && op->unk_2b != 2) ? op->unk_18 : 0;
            if (r) {
                for (int p = 0; p != r->unk_28; p++) {
                    if (((op->unk_07 & ~op->unk_27) >> p) & 1) {
                        if (op->unk_2c[p].unk_54.unk_04 != 1) {
                            func_0804ad38("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->unk_2c[p].unk_54.unk_00->unk_04;
                        Operand *so = func_0805802c(op->unk_2c[p].unk_54.unk_00);
                        int d = src->unk_13f0.unk_14 + so->unk_0a + op->unk_29;
                        if (d > in->unk_13f0.unk_14)
                            in->unk_13f0.unk_14 = d;
                    }
                }
            }
        }
    }

    wtInstrList *lst = &wt->unk_88;
    for (in = lst->unk_04; in; in = in->unk_1c.unk_04) {
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r;
            if (isRegisterOperand(op, 1) && op->unk_2b != 2)
                r = op->unk_18;
            else
                r = 0;
            if (r) {
                for (int p = 0; p != r->unk_28; p++) {
                    if (((op->unk_07 & ~op->unk_27) >> p) & 1) {
                        if (op->unk_2c[p].unk_54.unk_04 != 1) {
                            func_0804ad38("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->unk_2c[p].unk_54.unk_00->unk_04;
                        Operand *so = func_0805802c(op->unk_2c[p].unk_54.unk_00);
                        int d = in->unk_13f0.unk_00 + so->unk_0a + op->unk_29;
                        int h = in->unk_13f0.unk_02 + 1;
                        if (d > src->unk_13f0.unk_00)
                            src->unk_13f0.unk_00 = d;
                        if (h > src->unk_13f0.unk_02)
                            src->unk_13f0.unk_02 = h;
                    }
                }
            }
        }
    }

    for (in = lst->unk_04; in; ) {
        iLink *link = &in->unk_1c;
        Instr *m = link->unk_04;
        for (; m; m = m->unk_1c.unk_04) {
            if (func_080543bc(m, in)) {
                m->unk_13f0.unk_08++;
                m->unk_13f0.unk_0c += in->unk_13f0.unk_0c + 1;
                m->unk_13f0.unk_10 = in->unk_13f0.unk_10 + 1;
            }
        }
        m = (Instr *)*(int *)&link->unk_04;
        in = m;
    }

    int upper = 0;
    int lower = 0;
    int thru = 0;
    int best = 0;
    int both = 0;
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        if (in->unk_1388)
            unk_2b4 = in->unk_13f0.unk_04;
        char c = in->unk_138c;
        if (c == 3)
            both++;
        else if (c & 2)
            lower++;
        else
            upper++;
        unk_278.func_0805e3c0(&in->unk_13f0);
    }

    int maxIssue = 0;
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r = registerOperand(op, 2);
            if (r) {
                for (int p = 0; p != r->unk_28; p++) {
                    int nBoth = 0;
                    int n1 = 0;
                    int nOther = 0;
                    if ((op->unk_07 >> p) & 1) {
                        for (ol_s *ol = op->unk_2c[p].unk_54.func_08058370(); ol; ol = ol->func_080583ac()) {
                            char c = ol->unk_04->unk_138c;
                            if (c == 3)
                                nBoth++;
                            else if (c == 1)
                                n1++;
                            else
                                nOther++;
                        }
                    }
                    int hi = n1;
                    int lo = nOther;
                    if (n1 < nOther) {
                        hi = nOther;
                        lo = n1;
                    }
                    int rem = nBoth - (hi - lo);
                    if (rem > 0)
                        hi += (rem + 1) >> 1;
                    if (hi)
                        hi--;
                    hi += op->unk_0a;
                    if (hi > maxIssue)
                        maxIssue = hi;
                }
            }
        }
    }

    {
        int u = upper;
        int l = lower;
        for (; both; both--) {
            if (u < l)
                u++;
            else
                l++;
        }
        if (u > best) {
            unk_2ba = 3;
            best = u;
        }
        if (l > best) {
            unk_2ba = 4;
            best = l;
        }
    }
    if (maxIssue > best) {
        best = maxIssue;
        unk_2ba = 1;
    }

    int depth = unk_278.unk_14;
    if (depth < 0)
        depth = 0;
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *r = ru->unk_08;
        if (!r) {
            func_0804ad38("regref had no reg\n");
            return false;
        }
        for (int p = 0; p != r->unk_28; p++) {
            if ((ru->unk_0f >> p) & 1) {
                regUseSlot *s = &ru->unk_20[p];
                int h = s->unk_34[0];
                if (h > thru)
                    thru = h;
                int d = h - s->unk_34[3];
                if (d > depth)
                    depth = d;
                physSum += s->unk_34[1];
            }
        }
    }
    if (thru > best) {
        unk_2ba = 2;
        best = thru;
    }
    if (maxLat > best) {
        unk_2ba = 1;
        best = maxLat;
    }
    for (int i = 0; i != 9; i++) {
        if (i < 0 || i > 8) {
            func_0804ad38("bad stat\n");
            func_0804adf0(-1);
        }
        int n = (sc->unk_00[i] + 1) >> 1;
        if (n > best) {
            best = n;
            unk_2ba = (i << 8) | 9;
        }
    }
    if (physSum < best * 4)
        physSum = best * 4;
    if (physSum < 64)
        physSum = 64;
    if (physSum > 0x4000)
        physSum = 0x4000;
    if (wt->unk_09 == 3 && physSum < wt->getPhysicalLen())
        physSum = wt->getPhysicalLen();
    unk_2bc = physSum;
    if (canLoop()) {
        unk_2b8 = unk_2ba;
        unk_2e4 = best;
        if (best <= 1)
            unk_2e4 = 2;
    } else {
        unk_2b8 = 5;
        unk_2e4 = 0;
    }
    if (unk_278.unk_04 >= best) {
        unk_2ba = 6;
        best = unk_278.unk_04 + 1;
    }
    if (unk_278.unk_00 >= best) {
        unk_2ba = 7;
        best = unk_278.unk_00 + 1;
    }
    Instr *br = wt->getBranch();
    if (br) {
        int h = br->unk_13f0.unk_04 + 2;
        if (best <= h) {
            unk_2ba = 8;
            best = h;
        }
    }
    unk_2e0 = best;
    wt->unk_74 = unk_2e0;
    wt->unk_76 = unk_2e4;
    wt->unk_78 = depth;
    return true;
}

int instStream::getOrGuessDepLength()
{
    workTable *wt = getTable();
    if (!wt) {
        func_0804ad38("getOrGuess - no work table\n");
        func_0804adf0(-1);
    }
    int len = 0;
    switch (wt->unk_09) {
    case 1:
        len = func_0805f284();
        break;
    case 3:
        len = wt->func_08072eec();
        break;
    }
    return len;
}

int instStream::func_0805f354()
{
    workTable *wt = getTable();
    if (!wt) {
        func_0804ad38("getOrGuess - no work table\n");
        func_0804adf0(-1);
    }
    int len = 0;
    switch (wt->unk_09) {
    case 1:
    case 3:
        len = func_0805f284();
        break;
    }
    return len;
}

int instStream::getOrGuessLogicalLength()
{
    workTable *wt = unk_218;
    if (!wt)
        wt = getWorkTable();
    if (!wt) {
        func_0804ad38("getOrGuess - no work table\n");
        func_0804adf0(-1);
    }
    return wt->getLogicalLen();
}

void statCounts::clear()
{
    for (int i = 0; i != 9; i++)
        unk_00[i] = 0;
}

bool statCounts::add(Instr *in)
{
    if (!in)
        return false;
    for (int i = 0; i != 9; i++) {
        bool b = (in->unk_138d >> i) & 1;
        if (b)
            unk_00[i] += 2;
    }
    return true;
}

int statCounts::get(int i)
{
    if (i < 0 || i > 8) {
        func_0804ad38("bad stat\n");
        func_0804adf0(-1);
    }
    return (unk_00[i] + 1) >> 1;
}

bool func_0805f550(Block *b)
{
    b->unk_04 = -1;
    b->unk_06 = -1;
    for (Instr *in = b->unk_a0; in; in = in->unk_24.unk_00)
        in->func_0805f4e0();
    for (Instr *in = b->unk_94; in; in = in->unk_24.unk_00)
        in->func_0805f4e0();
    return true;
}

bool dynCalcStats(workTable *wt)
{
    Block *b = (Block *)wt;
    int i;
    Instr *in;
    func_0805f550(b);

    for (in = b->unk_a0; in; in = in->unk_24.unk_00) {
        int v = in->unk_13e8.unk_00;
        in->unk_140c = v;
        in->unk_140a = v;
        if (v > b->unk_04)
            b->unk_04 = v;
    }

    for (in = b->unk_94; in; in = in->unk_24.unk_00) {
        int h = 0;
        for (i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->inputRegB();
            if (reg) {
                for (int p = 0; p != reg->unk_28; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit && s->unk_19 != 2) {
                        cls0805bcc4 *par = s->getParent();
                        int lat = s->unk_18 + par->unk_18;
                        int t = 0;
                        if (par->unk_2d != 2)
                            t = par->ownerInstr()->unk_140a + lat;
                        if (t > h)
                            h = t;
                    }
                }
            }
        }
        for (i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->getReg2();
            if (reg) {
                for (int p = 0; p != reg->unk_28; p++) {
                    rPart *rp = reg->getRP(p);
                    if (in->func_0805544c(reg, 1 << p) && rp->unk_44 != -1) {
                        int v = rp->unk_44;
                        if (op->unk_0b)
                            v = v - 1;
                        else
                            v = v - op->unk_0a;
                        if (h <= v)
                            h = v + 1;
                    }
                }
            }
        }
        in->unk_140a = h;
        if (h > b->unk_04)
            b->unk_04 = h;
        for (i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->getReg();
            if (reg) {
                for (int p = 0; p != reg->unk_28; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit) {
                        rPart *rp = reg->getRP(p);
                        if (s->unk_19 != 2) {
                            if (rp->unk_42 == -1) {
                                rp->unk_42 = in->unk_140a;
                                rp->unk_44 = in->unk_140a;
                            } else {
                                if (rp->unk_42 > in->unk_140a)
                                    rp->unk_42 = in->unk_140a;
                                if (rp->unk_44 < in->unk_140a)
                                    rp->unk_44 = in->unk_140a;
                            }
                        }
                    }
                }
            }
        }
        for (i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->getReg2();
            if (reg) {
                for (int p = 0; p != reg->unk_28; p++) {
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit) {
                        rPart *rp = reg->getRP(p);
                        rp->unk_42 = -1;
                        rp->unk_44 = -1;
                    }
                }
            }
        }
    }

    for (in = b->unk_94; in; in = in->unk_24.unk_00)
        in->unk_140c = b->unk_04;

    for (in = b->unk_98; in; in = in->unk_24.unk_04) {
        for (i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->getReg();
            if (reg) {
                for (int p = 0; p != reg->unk_28; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit && s->unk_19 != 2) {
                        cls0805bcc4 *par = s->getParent();
                        if (par->unk_2d != 2) {
                            Instr *pi = par->ownerInstr();
                            int v = in->unk_140a - (par->unk_18 + s->unk_18);
                            if (v < 0) {
                                func_0804ad38("-ve ip?\n");
                                in->func_08054008(false);
                                wt->dump();
                                in->func_08054008(false);
                                func_0804adf0(-1);
                            }
                            if (pi->unk_140c > v)
                                pi->unk_140c = v;
                        }
                    }
                }
            }
        }
    }

    for (in = b->unk_94; in; in = in->unk_24.unk_00) {
        for (i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->getReg();
            if (reg) {
                for (int p = 0; p != reg->unk_28; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit && s->unk_19 != 2) {
                        cls0805bcc4 *par = s->getParent();
                        if (par->unk_2d != 2) {
                            Instr *pi = par->ownerInstr();
                            int lat;
                            if (s->unk_15)
                                lat = 1;
                            else
                                lat = par->unk_18;
                            int v = pi->unk_140c + lat;
                            if (v < in->unk_1408)
                                in->unk_1408 = v;
                            v = in->unk_140a - lat;
                            if (v > pi->unk_140e)
                                pi->unk_140e = v;
                        }
                    }
                }
            }
        }
    }

    for (in = b->unk_94; in; in = in->unk_24.unk_00) {
        in->unk_13f0.unk_04 = in->unk_140a;
        in->unk_13f0.unk_00 = b->unk_04 - in->unk_140c;
    }
    return true;
}

void dynDumpStats(Block *b)
{
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    func_0804acb0("dynDumpStats - start\n");
    int n = 0;
    for (Instr *in = b->unk_94; in; in = in->unk_24.unk_00) {
        const char *mark = "    ";
        int s = in->unk_140a;
        int e = in->unk_1408;
        int dep = b->unk_38;
        if (s - e > dep)
            mark = "****";
        int ht = b->unk_04 - in->unk_13f0.unk_00;
        dep = in->unk_13f0.unk_04;
        func_0804acb0("[%3d]:%3d [%5d %5d %5d %5d ]%s dep=%2d ht=%2d  %s\n",
                      n, b->unk_38, e, s, in->unk_140c, in->unk_140e, mark,
                      dep, ht, in->unk_34->unk_04);
        n++;
    }
    func_0804acb0("dyDuumpStats - end\n");
    errorOccurred = saved;
}
