#include "lock_shared.h"

static inline bool canWrapInputLock(cls0805bcc4 *in)
{
    return in->unk_19 == 1;
}

static inline void wrapInputRange(int &s, int &e)
{
    int n = g_080b336c;
    if (n) {
        if (s >= n) {
            do {
                s -= n;
                e -= n;
            } while (s >= n);
        }
    }
}

int cls0805c244::checkInputLock(cls0805bcc4 *in, short *pos, int *minPos)
{
    cls0805bcc4 *lk = unk_1c;
    int ret;
    int lsz = g_080b336c;
    ret = 0;
    int p = ((cls08058718 *)pos)->unk_00;

    in->unk_1a = 0;
    if (in->unk_19 == 2) {
        func_0804ad38("CheckInputLock::aa input\n");
        func_0804aeb0();
        return 0;
    }
    if (unk_29)
        return 0;

    cls0805bcc4 *par = in->getParent();
    int lo = par->unk_36;
    int hi = par->unk_4e;

    if (lsz && isRegLock(par)) {
        if (par->unk_2e == 0)
            return 0;
        bool inside = true;
        if (p < lo) { lo = p; inside = false; }
        if (p > hi) { hi = p; inside = false; }
        if (!inside) {
            int span = hi - lo;
            int room = lsz;
            if (unk_0c[0])
                room -= unk_0c[0]->unk_18;
            else {
                func_0804ad38("no ext output set. but .. not const for loop\n");
                func_0804adf0(-1);
            }
            if (span > room) {
                int need;
                if (p < par->unk_3e)
                    need = par->unk_4e - room;
                else if (canWrapInputLock(in)) {
                    in->unk_1a = 1;
                    return 0x100;
                } else
                    need = 30000;
                if (*minPos < need)
                    *minPos = need;
                return 0x2000;
            }
        }
        return 0;
    }

    int start = par->unk_3a + in->unk_18;
    if (p < start) {
        if (start > *minPos)
            *minPos = start;
        return 8;
    }
    if (p <= hi)
        return 0;

    int d = p - lo;
    if (lsz && d >= lsz) {
        if (canWrapInputLock(in)) {
            in->unk_1a = 3;
            return 0x100;
        }
        *minPos = 30000;
        return 0x400;
    }

    int s = par->unk_34;
    int e = s + d;
    wrapInputRange(s, e);

    bool isExt = false;
    if (lsz && par == unk_0c[0])
        isExt = true;

    for (; lk; lk = lk->unk_08) {
        if (lk == par)
            continue;
        if (isExt && lk->unk_2d == 2)
            continue;
        int ls = lk->unk_34;
        int le = lk->unk_4c;
        if (lk->unk_22) {
            if (s < le && e >= ls) {
                *minPos = 30000;
                return ret | 0x10;
            }
        } else if (lockRangesOverlap(s, e, ls, le)) {
            if (g_080de288 && lsz && canWrapInputLock(in)) {
                in->unk_1a = 2;
                ret |= 0x100;
            } else {
                *minPos = 30000;
                return ret | 0x20;
            }
        }
    }
    return ret;
}

static inline void nextLoopPosition(int v, int row, int n, int &next)
{
    if (!n)
        next = v;
    else {
        int r = v % n;
        int rem = row % n;
        int q = row / n;
        if (r <= rem)
            q++;
        next = q * n + r;
    }
}

int cls0805c244::checkOutputLock(cls0805bcc4 *out, short *pos, int *minPos)
{
    int lsz = g_080b336c;
    int ret = checkOverlap(out, pos, minPos);
    if (isRegLock(out))
        return 0;
    int p = ((cls08058718 *)pos)->unk_00;
    int first;
    if (out->unk_15)
        first = p + 1;
    else
        first = out->unk_18 + p;
    int last = out->unk_18 + p;

    if (out->unk_16) {
        if (unk_26 > 1) {
            *minPos = 30000;
            return ret | 0x800;
        }
        for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
            if (first <= lk->unk_4e) {
                int v = lk->unk_4e - (first - p) + 1;
                if (*minPos < v)
                    *minPos = v;
                ret |= 0x1000;
            }
        }
        if (ret)
            return ret;
    }

    bool ext = false;
    int lim;
    int he;
    if (!out->unk_20)
        lim = first;
    else {
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
    }

    if (lsz && unk_28) {
        cls0805bcc4 *h = unk_20;
        if (h->unk_22) {
            *minPos = 30000;
            return ret | 4;
        }
        if (h->unk_2e) {
            if (first <= h->unk_4e) {
                int v = h->unk_4e - (first - p) + 1;
                if (*minPos < v)
                    *minPos = v;
                ret |= 0x4000;
            }
            int hs = h->unk_3e + lsz;
            he = h->unk_4e + lsz;
            if (hs < last) {
                *minPos = 30000;
                return 0x20000;
            }
            if (out->unk_16) {
                if (he - first >= lsz) {
                    *minPos = 30000;
                    return 0x2000;
                }
                if (he > lim)
                    lim = he;
                ext = true;
            }
        } else
            ext = true;
    }

    int sz = ((cls08058718 *)pos)->unk_04;
    int s = sz + (first - p);
    int e = sz + (lim - p);
    wrapRangeLocal(s, e);

    for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
        if (ext && lk->unk_2d == 2)
            continue;
        int ls = lk->unk_34;
        int le = lk->unk_4c;
        wrapRangeLocal(ls, le);
        if (lk->unk_22) {
            if (lsz) {
                *minPos = 30000;
                return ret | 4;
            }
            if (e >= ls) {
                *minPos = 30000;
                return ret | 4;
            }
        } else if (lockRangesOverlap(s, e, ls, le)) {
            first -= p;
            int v = lk->unk_4e - first + 1;
            nextLoopPosition(v, ((cls08058718 *)pos)->unk_00, g_080b336c, he);
            if (*minPos < he)
                *minPos = he;
            return ret | 2;
        }
    }
    return ret;
}

int cls0805c244::checkOverlap(cls0805bcc4 *out, short *pos, int *minPos)
{
    int ret = 0;
    if (out->unk_17 <= 1)
        return 0;

    int s = pos[2];
    int e = s + out->unk_17 - 1;
    for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
        if (lk == out)
            continue;
        int os = lk->unk_30;
        int oe = os + lk->unk_17;
        if (oe - 1 < 0)
            continue;
        if (lockRangesOverlap(s, e, os, oe - 1)) {
            ret |= 1;
            int p = ((cls08058718 *)pos)->unk_00;
            int v;
            if (!g_080b336c)
                v = oe;
            else {
                int r = oe % g_080b336c;
                int q = p / g_080b336c;
                if (r <= p % g_080b336c)
                    q++;
                v = q * g_080b336c + r;
            }
            if (*minPos < v)
                *minPos = v;
        }
    }
    return ret;
}

bool lockRangesOverlap(int s1, int e1, int s2, int e2)
{
    int n = g_080b336c;
    if (n) {
        int l1 = e1 - s1;
        int l2 = e2 - s2;
        wrapRangeLocal(s1, e1);
        wrapRangeLocal(s2, e2);
        if (l1 + l2 > n)
            return true;
        if (s1 <= s2)
            return e1 >= s2 || e2 - n >= s1;
        return s1 <= e2 || e1 - n >= s2;
    }
    if (s1 <= s2)
        return e1 >= s2;
    return s1 <= e2;
}
