#include "os.h"
#include "lock_shared.h"

bool cls0805c244::catLocks(OperandLock *a, OperandLock *b, int *s, int *e, int *end)
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

static inline bool isAAInputLock(OperandLock *in)
{
    return in->unk_19 == 2;
}

static inline bool sameLockOwner(OperandLock *a, OperandLock *b)
{
    return a->ownerPointer == b->ownerPointer;
}

int cls0805c244::canMoveInputLock(OperandLock *in, short *pos, int slotBit)
{
    int p = *pos;
    OperandLock *lk = outputLocks;
    int lsz = g_080b336c;
    if (unk_29)
        return 0;
    if (isAAInputLock(in)) {
        reportError("canMoveInputLock - aa input\n");
        abortAfterError();
        return 0;
    } else {
        OperandLock *par = in->getParent();
        bool cat = false;
        int s, e, end;
        if (lsz && isRegLock(par)) {
            OperandLock *x = externalOutputLocks[0];
            if (x->unk_2c) {
                if (sameLockOwner(in, x))
                    return 0;
                p += lsz;
                if (!catLocks(par, x, &s, &e, &end)) {
                    reportError("failed to concatenate reg ref with xo\n");
                    exitAfterCleanup(-1);
                }
                cat = true;
                par = x;
            } else if (par->unk_2e) {
                s = par->unk_36;
                e = par->unk_3a;
                end = par->unk_4e;
                if (p - in->issueLatency < s)
                    s = p - in->issueLatency;
                if (p - in->issueLatency < e)
                    e = p - in->issueLatency;
                if (p > end)
                    end = p;
                if (end - s >= lsz)
                    return 0x2000;
            } else {
                s = p - in->issueLatency;
                e = p;
                end = p;
            }
        } else if (lsz && unk_28 && par->unk_16) {
            if (!catLocks(sortedOutputLocks, par, &s, &e, &end)) {
                reportError("case B failed to cat xo\n");
                checkCounts();
                reportError("we are :\n");
                in->print();
                exitAfterCleanup(-1);
            }
            cat = true;
        } else {
            s = par->unk_36;
            e = par->unk_3a;
            end = par->unk_4e;
        }

        if (p - in->issueLatency < e)
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
        } while ((lk = lk->nextSibling) != 0);
    }
    return 0;
}

int cls0805c244::canMoveOutputLock(OperandLock *out, short *pos, int slotBit)
{
    int ret;
    int lsz = g_080b336c;
    int p = ((InstructionPosition *)pos)->linearRow;
    int sz = ((InstructionPosition *)pos)->loopRow;
    int mp = p;
    ret = checkOverlap(out, pos, &mp);
    if (ret)
        return ret;
    int first = out->unk_15 ? p + 1 : out->issueLatency + p;
    int last = out->issueLatency + p;

    int lim;
    if (out->referenceCount) {
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
        for (OperandLock *lk = outputLocks; lk; lk = lk->nextSibling) {
            if (lk == out)
                continue;
            if (first <= lk->unk_4e)
                return 0x1000;
        }
        if (lsz && unk_28) {
            OperandLock *h = sortedOutputLocks;
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
        OperandLock *h = sortedOutputLocks;
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
    for (OperandLock *lk = outputLocks; lk; lk = lk->nextSibling) {
        if (lk == out)
            continue;
        if (ext && lk->ownerType == 2)
            continue;
        int ls = lk->unk_34;
        int le = lk->unk_4c;
        wrapRangeLocal(ls, le);
        if (lockRangesOverlap(s, e, ls, le))
            return 2;
    }
    if (ret)
        return ret;
    if (externalOutputCount && !lsz && !out->unk_15) {
        OperandLock *x = externalOutputLocks[0];
        if (!x) {
            reportError("external lock not set \n");
            abortAfterError();
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

