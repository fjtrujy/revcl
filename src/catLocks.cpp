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
            } else if (lockRangesOverlap(ws, we, ls, le))
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
        if (lockRangesOverlap(s, e, ls, le))
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

