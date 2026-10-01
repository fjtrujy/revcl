#pragma implementation "instStreamLen.h"
#pragma implementation "InstrDyn.h"
#include "Instr.h"
#include "os.h"
#include "instStreamLen.h"
#include "InstrDyn.h"
#include "workTable.h"
#include "Block.h"
#include "statCounts.h"

extern int errorOccurred;
extern bool quiet_080bbd98;

bool cls0805bcc4::setOwner(int part, Instr *instr, int opnd)
{
    if (!instr) {
        func_0804ad38("setowner - passed null\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_24) {
        func_0804ad38("owner already set for rlock set owner\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_2d) {
        func_0804ad38("owner: type wasn't null when try to set\n");
        return false;
    }
    if (unk_04) {
        func_0804ad38("setowner - lock was already on list\n");
        func_0804adf0(-1);
        return false;
    }
    unk_2d = 1;
    unk_28 = part;
    unk_29 = opnd;
    unk_24 = instr;
    return true;
}

bool cls0805bcc4::setOwner(int part, regUse *rf)
{
    if (!rf) {
        func_0804ad38("setowner - passed null rf\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_24) {
        func_0804ad38("owner already set for rlock set owner\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_2d) {
        func_0804ad38("owner: type wasn't null when try to set\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_04) {
        func_0804ad38("setowner - lock was already on list\n");
        func_0804adf0(-1);
        return false;
    }
    unk_29 = 0;
    unk_28 = part;
    unk_2a = 0;
    unk_2d = 2;
    unk_24 = rf;
    unk_32 = -1;
    unk_30 = -1;
    return true;
}

bool cls0805bcc4::clearOwner()
{
    unk_24 = 0;
    unk_2d = 0;
    unk_14 = 0;
    unk_04 = 0;
    unk_28 = -1;
    unk_29 = -1;
    return true;
}

Instr *cls0805bcc4::ownerInstr()
{
    if (unk_2d == 1)
        return (Instr *)unk_24;
    return 0;
}

regUse *cls0805bcc4::ownerRegUse()
{
    if (unk_2d == 2)
        return (regUse *)unk_24;
    return 0;
}

Operand *cls0805bcc4::ownerOperand()
{
    if (unk_2d == 1) {
        int opnd = unk_29;
        Instr *instr = (Instr *)unk_24;
        return &instr->unk_38[opnd];
    }
    return 0;
}

int cls0805bcc4::ownerOpnd()
{
    if (unk_2d == 1)
        return unk_29;
    return -1;
}

int cls0805bcc4::getPart()
{
    if (unk_2d == 1 || unk_2d == 2)
        return unk_28;
    func_0804ad38("failed to get part - lock incorrect oType\n");
    func_0804adf0(-1);
    return -1;
}

void cls0805bcc4::printOwner()
{
    switch (unk_2d) {
    case 0:
        break;
    case 1: {
        Instr *instr = unk_2d == 1 ? (Instr *)unk_24 : 0;
        if (!instr) {
            func_0804ad38("type inst..  can't get it though!\n");
            func_0804adf0(-1);
        }
        instr->func_08054008(false);
        break;
    }
    case 2: {
        regUse *rf = unk_2d == 2 ? (regUse *)unk_24 : 0;
        if (!rf) {
            func_0804ad38("type regref..  can't get it though!\n");
            func_0804adf0(-1);
        }
        break;
    }
    default:
        func_0804ad38("??????????? unkown lock\n");
        func_0804adf0(-1);
    }
}

cls0805bcc4 *cls0805bcc4::getParent()
{
    if (unk_14 != 1) {
        func_0804ad38("getParent - called on non input lock\n");
        print();
        return 0;
    }
    if (!unk_0c) {
        func_0804ac7c("getParent: not set!\n");
        print();
        return 0;
    }
    if (unk_0c->unk_14 != 2) {
        func_0804ad38("parent was not output!\n");
        func_0804ad38("we are:\n");
        print();
        func_0804ad38("parent is\n");
        unk_0c->print();
        func_0804adf0(-1);
    }
    return unk_0c;
}

bool cls0805bcc4::setInOut(cls0805bcc4 *olk)
{
    if (!olk) {
        func_0804ad38("setinout - passed null\n");
        func_0804adf0(-1);
    }
    if (olk->unk_14 != 2) {
        func_0804ad38("setinout - olk wasn't output\n");
        func_0804adf0(-1);
    }
    if (!unk_24) {
        func_0804ad38("owner not set for inout\n");
        func_0804adf0(-1);
    }
    if (unk_24 != olk->unk_24) {
        func_0804ad38("setinout -- inst bad\n");
        func_0804adf0(-1);
    }
    if (unk_28 != olk->unk_28) {
        func_0804ad38("set in out - part mismatched\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_1c) {
        func_0804ad38("rlock - set inout - already set\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_14 != 1) {
        func_0804ad38("set inout on non input\n");
        print();
        func_0804adf0(-1);
        return false;
    }
    unk_1c = olk;
    return true;
}

bool cls0805bcc4::incRef()
{
    unk_22++;
    return true;
}

bool cls0805bcc4::decRef()
{
    unk_22--;
    return true;
}

void cls0805bcc4::print()
{
    switch (unk_14) {
    case 0:
    case 1:
        break;
    case 2:
        for (cls0805bcc4 *c = unk_0c; c; c = c->unk_08)
            c->print();
        break;
    }
    printOwner();
}

cls0805c244::cls0805c244()
{
    unk_04 = 0;
    unk_08 = 0;
    for (int i = 0; i != 4; i++)
        unk_0c[i] = 0;
    unk_2a = 0;
    unk_2b = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_26 = 0;
    unk_28 = 0;
    unk_29 = 1;
    unk_20 = 0;
    unk_00 = 0;
}

cls0805c244::cls0805c244(cls0805c244 *next)
{
    unk_08 = next;
    unk_04 = 0;
    for (int i = 0; i != 4; i++)
        unk_0c[i] = 0;
    unk_2a = 0;
    unk_2b = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_26 = 0;
    unk_28 = 0;
    unk_29 = 1;
    unk_20 = 0;
    unk_00 = 0;
}

cls0805c244::~cls0805c244()
{
}

void cls0805c244::checkCounts()
{
    int n = 0;
    for (cls0805bcc4 *o = unk_1c; o; o = o->unk_08) {
        o->printOwner();
        for (cls0805bcc4 *c = o->unk_0c; c; c = c->unk_08)
            c->printOwner();
        n++;
    }
    if (unk_24 != unk_26 + n)
        func_0804ad38("ERROR - nOutLocks != nToGo + nDone\n");
}

bool cls0805c244::reset()
{
    for (int i = 0; i != 4; i++)
        unk_0c[i] = 0;
    unk_2a = 0;
    unk_2b = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_26 = 0;
    unk_28 = 0;
    unk_29 = 1;
    unk_20 = 0;
    unk_00 = 0;
    return true;
}

bool cls0805c244::addInput(cls0805bcc4 *out, cls0805bcc4 *in, char a, unsigned char b)
{
    in->unk_14 = 1;
    in->unk_04 = this;
    out->unk_15 = b;
    in->unk_18 = a;
    out->unk_46 += in->unk_02;
    out->unk_48 += in->unk_01;
    out->unk_20++;
    out->unk_22++;
    in->unk_0c = out;
    out->unk_4a += in->unk_00;
    return true;
}

static inline bool isRegLock(cls0805bcc4 *lk)
{
    return lk->unk_2d == 2;
}

bool cls0805c244::addOutput(cls0805bcc4 *lk, int a, int b, unsigned char c)
{
    if (isRegLock(lk))
        unk_28 = 1;
    lk->unk_14 = 2;
    lk->unk_17 = a;
    lk->unk_18 = b;
    lk->unk_04 = this;
    lk->unk_15 = c;
    unk_26++;
    unk_24++;
    return true;
}

bool cls0805c244::removeInput(cls0805bcc4 *in)
{
    cls0805bcc4 *out = in->unk_0c;
    out->unk_46 -= in->unk_02;
    out->unk_48 -= in->unk_01;
    out->unk_20--;
    out->unk_22--;
    out->unk_4a -= in->unk_00;
    in->unk_04 = 0;
    in->unk_14 = 0;
    in->unk_0c = 0;
    return true;
}

bool cls0805c244::removeOutput(cls0805bcc4 *lk)
{
    lk->unk_14 = 0;
    lk->unk_04 = 0;
    unk_26--;
    unk_24--;
    return true;
}

static inline bool isOwnedBy(cls0805bcc4 *lk, cls0805c244 *ll)
{
    return lk->unk_04 == ll;
}

bool cls0805c244::addOutputLock(cls0805bcc4 *lk)
{
    if (!lk) {
        func_0804ad38("Addoutputlock - null pointer\n");
        return false;
    }
    if (lk->unk_14 != 2) {
        func_0804ad38("Addoutputlock - bad type\n");
        return false;
    }
    if (lk->unk_2c) {
        func_0804ad38("Addoutputlock - already inserted\n");
        func_0804adf0(-1);
        return false;
    }
    if (!isOwnedBy(lk, this)) {
        func_0804ad38("Addoutput lock - I don't own it\n");
        func_0804adf0(-1);
        return false;
    }
    if (lk->unk_0c) {
        func_0804ad38("AddOutputLock: m_Link already set!\n");
        func_0804adf0(-1);
        return false;
    }
    if (lk->unk_22 != lk->unk_20) {
        func_0804ad38("Addoutputlock - mRefCount!=m_RefCount\n");
        func_0804adf0(-1);
        return false;
    }
    if (lk->unk_08) {
        func_0804ad38("AddOutputLock:: m_Next already set\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_26 <= 0) {
        func_0804ad38("add output lock - count was already down to zero or less\n");
        checkCounts();
        func_0804adf0(-1);
        return false;
    }
    unk_26--;
    lk->unk_08 = unk_1c;
    unk_1c = lk;
    if (!lk->calcStats(0)) {
        func_0804ad38("AddOutputLock failed to calcStats\n");
        checkCounts();
        func_0804adf0(-1);
    }
    lk->unk_2c = 1;
    lk->unk_10 = 0;
    cls0805bcc4 *prev = 0;
    for (cls0805bcc4 *cur = unk_20; cur; cur = cur->unk_10) {
        int rl = lk->unk_30;
        int crl = cur->unk_30;
        if ((short)rl == (short)crl && (short)rl >= 0) {
            func_0804ad38("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            func_0804adf0(-1);
        }
        int line = lk->unk_32;
        int cline = cur->unk_32;
        if ((short)line < (short)cline)
            break;
        prev = cur;
    }
    if (prev) {
        lk->unk_10 = prev->unk_10;
        prev->unk_10 = lk;
    } else {
        lk->unk_10 = unk_20;
        unk_20 = lk;
    }
    return true;
}

bool cls0805c244::addInputLock(cls0805bcc4 *in)
{
    cls0805bcc4 *out = in->unk_0c;
    out->unk_22--;
    in->unk_08 = out->unk_0c;
    out->unk_0c = in;
    if (!out->calcStats(0)) {
        func_0804ad38("AddInputLock failed to calcStats\n");
        func_0804adf0(-1);
    }
    in->unk_2c = 1;
    return true;
}

bool cls0805c244::undoInputLock(cls0805bcc4 *in)
{
    cls0805bcc4 *out = in->unk_0c;
    out->unk_22++;
    out->unk_0c = in->unk_08;
    in->unk_08 = 0;
    if (!out->calcStats(0)) {
        func_0804ad38("UndoInputLock failed to calcStats\n");
        func_0804adf0(-1);
        return false;
    }
    in->unk_2c = 0;
    in->resetStats();
    return true;
}

bool cls0805c244::undoOutputLock(cls0805bcc4 *lk)
{
    unk_26++;
    unk_1c = lk->unk_08;
    lk->unk_08 = 0;
    if (!lk->calcStats(0)) {
        func_0804ad38("UndoOutputLock failed to calcStats\n");
        checkCounts();
        func_0804adf0(-1);
        return false;
    }
    lk->unk_2c = 0;
    cls0805bcc4 *prev = 0;
    cls0805bcc4 *cur = unk_20;
    while (cur && cur != lk) {
        prev = cur;
        cur = cur->unk_10;
    }
    if (!cur) {
        func_0804ad38("couldn't find lock on the sorted list\n");
        func_0804adf0(-1);
    }
    if (prev)
        prev->unk_10 = lk->unk_10;
    else
        unk_20 = lk->unk_10;
    lk->unk_10 = 0;
    lk->resetStats();
    return true;
}

int cls0805c244::func_0805c938(int n)
{
    int best = 0;
    for (cls0805bcc4 *lk = unk_20; lk; lk = lk->unk_10) {
        n--;
        if (lk->unk_42 > best)
            best = lk->unk_42;
        if (lk->unk_22 != 0)
            return 30000;
        if (n <= 0)
            break;
    }
    return best;
}

bool cls0805bcc4::isOk(int type, bool inserted)
{
    int loopSize = g_080b336c;
    bool ok = true;
    regUse *rf = unk_2d == 2 ? (regUse *)unk_24 : 0;
    Operand *op = unk_2d == 1 ? &((Instr *)unk_24)->unk_38[unk_29] : 0;

    if (rf) {
        if (op) {
            func_0804ad38("rLock::isOk() - both reg ref and op ok\n");
            ok = false;
        }
    } else if (!op) {
        func_0804ad38("rLock::isOk() - neither op nor regref set\n");
        ok = false;
    }
    if (type >= 0 && unk_14 != type) {
        func_0804ad38("isOk - unexpected type for is ok\n");
        ok = false;
    }
    if ((unk_2c != 0) != inserted) {
        func_0804ad38("isOK - insert state was unexpected!\n");
        ok = false;
    }
    switch (unk_14) {
    case 1:
        if (!getParent()) {
            func_0804ad38("input lock has no parent\n");
            ok = false;
        }
        break;
    case 2: {
        if (!inserted && unk_20 != unk_22) {
            func_0804ad38("output lock - ref count mismatch\n");
            return false;
        }
        regUse *orf = unk_2d == 2 ? (regUse *)unk_24 : 0;
        if (orf && loopSize) {
            if (unk_18 > 1) {
                func_0804ad38("external input had latency for loop\n");
                ok = false;
            }
            if (unk_17 > 1) {
                func_0804ad38("external input had thruput for loop\n");
                ok = false;
            }
        }
        if (loopSize) {
            if (unk_18 > loopSize && unk_15 && unk_20) {
                func_0804ad38("latency bigger than loop!\n");
                ok = false;
            }
            if (unk_17 > loopSize) {
                func_0804ad38("thruput bigger than loop!\n");
                ok = false;
            }
        }
        break;
    }
    default:
        func_0804ad38("rLock::isOk() - bad lock type\n");
        ok = false;
        break;
    }
    if (!ok) {
        func_0804ad38("rLock::isOk - failed\n");
        print();
        func_0804adf0(-1);
    }
    return ok;
}

bool cls0805c244::isOk(cls0805bcc4 *lk, int type, bool inserted)
{
    if (!lk) {
        func_0804ad38("rLockList::isOk  -passed a null lock\n");
        func_0804adf0(-1);
    }
    bool ok = lk->isOk(type, inserted);
    if (lk->unk_14 == 0) {
        cls0805bcc4 *parent = lk->getParent();
        cls0805bcc4 *p;
        for (p = unk_1c; p; p = p->unk_08)
            if (p == parent)
                break;
        if (!p) {
            func_0804ad38("rLockList::isOk - parent is not on insert list\n");
            ok = false;
        }
    }
    if (lk->unk_04 != this) {
        func_0804ad38("isok - I don't own it\n");
        func_0804adf0(-1);
        return false;
    }
    if (lk->unk_14 == 2) {
        if (unk_24 <= 0) {
            func_0804ad38("isOk - output, but no outputs! - <=0 output locks!\n");
            ok = false;
        }
        if (unk_26 < !inserted) {
            func_0804ad38("isOk - output, but no outputs! - <=0 output locks to go!\n");
            ok = false;
        }
    }
    if (!ok)
        func_0804adf0(-1);
    return ok;
}

short cls0805bcc4::func_0805ccc8()
{
    return unk_42;
}

short cls0805bcc4::func_0805ccd4()
{
    return unk_3e;
}

static inline bool hasPartFlag(cls0805bcc4 *lk)
{
    return lk->unk_16 && lk->unk_04->unk_28;
}

bool cls0805bcc4::calcStats(Instr *skip)
{
    int lsz = g_080b336c;
    cls0805c244 *ll = unk_04;
    unk_2e = 0;
    int maxT = -30000;
    int minFree = 30000;
    int minUse = 30000;
    for (cls0805bcc4 *c = unk_0c; c; c = c->unk_08) {
        if (skip) {
            Instr *o = c->unk_2d == 1 ? (Instr *)c->unk_24 : 0;
            if (o == skip)
                continue;
        }
        if (c->unk_19 == 2)
            continue;
        int t = c->unk_32;
        if (t > maxT)
            maxT = t;
        if (t < minUse)
            minUse = t;
        t -= c->unk_18;
        if (t < minFree)
            minFree = t;
        unk_2e++;
    }
    if (ll->unk_29 && lsz) {
        minFree = 0;
        maxT = lsz - 1;
        unk_2e = 0;
    }
    if (unk_2e != 0) {
        unk_3e = minFree;
        unk_42 = maxT;
        if (lsz) {
            if (isRegLock(this)) {
                unk_36 = minFree;
                unk_3a = minFree;
            } else {
                if (unk_15)
                    unk_36 = unk_32 + 1;
                else
                    unk_36 = unk_18 + unk_32;
                unk_3a = unk_18 + unk_32;
            }
        } else if (isRegLock(this)) {
            if (unk_18 == 0) {
                unk_36 = 0;
                unk_3a = 0;
            } else {
                if (unk_15)
                    unk_36 = 0;
                else
                    unk_36 = unk_18 + unk_32;
                unk_3a = unk_18 + unk_32;
            }
        } else {
            if (unk_15)
                unk_36 = unk_32 + 1;
            else
                unk_36 = unk_18 + unk_32;
            unk_3a = unk_18 + unk_32;
        }
        if (unk_3e < unk_3a) {
            if (g_080de284) {
                func_0804ad38("rLock::calcStats - rfUsed before result ready..rfu=%d rr=%d!\n", unk_3e, unk_3a);
                print();
                return false;
            }
            if (minUse < unk_36)
                func_0804ac7c("rLock::calcStats - stalls non fatal, but.. rfUsed_nolh before result write..rfu=%d rw=%d..this maybe important\n", minUse, unk_36);
        }
    } else {
        if (isRegLock(this)) {
            if (unk_18 == 0) {
                unk_36 = 0;
                unk_3a = 0;
            } else {
                if (unk_15)
                    unk_36 = 0;
                else
                    unk_36 = unk_18 + unk_32;
                unk_3a = unk_18 + unk_32;
            }
        } else {
            if (unk_15)
                unk_36 = unk_32 + 1;
            else
                unk_36 = unk_18 + unk_32;
            unk_3a = unk_18 + unk_32;
        }
        unk_3e = unk_36;
        unk_42 = unk_36;
    }
    int shift = unk_30 - unk_32;
    unk_34 = shift + unk_36;
    unk_38 = shift + unk_3a;
    unk_3c = shift + unk_3e;
    unk_40 = shift + unk_42;
    if (unk_20 != 0 || hasPartFlag(this)) {
        unk_4e = unk_42;
        if (!lsz || unk_2d != 2) {
            int hi = unk_48;
            int lo = unk_4a;
            if (hi < lo) {
                hi = unk_4a;
                lo = unk_48;
            }
            int slack = unk_46 - (hi - lo);
            if (slack > 0)
                hi += (slack + 1) >> 1;
            if (hi > 0)
                hi--;
            hi += unk_3a;
            if (hi > unk_42)
                unk_4e = hi;
        }
    } else
        unk_4e = unk_36;
    unk_4c = shift + unk_4e;
    return true;
}

void cls0805bcc4::func_0805d020()
{
    resetStats();
}

void cls0805bcc4::setFrom(cls08058718 *r)
{
    int rline = r->unk_00;
    unk_32 = rline;
    int loop = r->unk_04;
    unk_30 = loop;
    int lsz = g_080b336c;
    if (lsz) {
        if (rline % lsz == loop)
            return;
        func_0804ad38("rLock::setFrom: rline/loop mismatch %d vs %d (lsz%d)\n", rline, loop, lsz);
    } else {
        if (unk_30 == unk_32)
            return;
        func_0804ad38("setFrom rline/non-loop mismatch\n");
    }
    func_0804adf0(-1);
}

short cls0805bcc4::func_0805d0dc()
{
    if (g_080b336c)
        return unk_32 / g_080b336c;
    return 0;
}

bool cls0805c244::moveOutputLock(cls0805bcc4 *lk)
{
    if (!lk) {
        func_0804ad38("moveoutputlock - null pointer\n");
        return false;
    }
    if (lk->unk_14 != 2) {
        func_0804ad38("moveoutputlock - bad type\n");
        return false;
    }
    if (!lk->unk_2c) {
        func_0804ad38("moveoutputlock - not already inserted\n");
        func_0804adf0(-1);
        return false;
    }
    if (!lk->calcStats(0)) {
        func_0804ad38("moveoutputlock failed to set lUsed\n");
        checkCounts();
        func_0804adf0(-1);
    }
    cls0805bcc4 *cur = unk_20;
    cls0805bcc4 *prev = 0;
    unk_20 = 0;
    while (cur) {
        cls0805bcc4 *next = cur->unk_10;
        cur->unk_10 = 0;
        if (cur != lk) {
            if (!unk_20)
                unk_20 = cur;
            else
                prev->unk_10 = cur;
            prev = cur;
        }
        cur = next;
    }
    prev = 0;
    for (cur = unk_20; cur; cur = cur->unk_10) {
        int rl = lk->unk_30;
        int crl = cur->unk_30;
        if ((short)rl == (short)crl) {
            func_0804ad38("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            func_0804adf0(-1);
        }
        int line = lk->unk_32;
        int cline = cur->unk_32;
        if ((short)line < (short)cline)
            break;
        prev = cur;
    }
    if (prev) {
        lk->unk_10 = prev->unk_10;
        prev->unk_10 = lk;
    } else {
        lk->unk_10 = unk_20;
        unk_20 = lk;
    }
    return true;
}

bool cls0805c244::moveInputLock(cls0805bcc4 *in)
{
    if (!in->unk_0c->calcStats(0)) {
        func_0804ad38("moveinputlock failed to calcStats\n");
        func_0804adf0(-1);
    }
    return true;
}

bool cls0805c244::addExtOutput(cls0805bcc4 *lk, bool flag)
{
    bool ok = true;
    if (lk->unk_16) {
        func_0804ad38("lock is already external output\n");
        ok = false;
    } else if (unk_2a > 3) {
        func_0804ad38("too many extern outputs\n");
        ok = false;
    } else if (unk_2a && g_080de284 == 1) {
        if (!flag) {
            func_0804ad38("more than one xo for rlock uta..count=%d\n", unk_2a);
            checkCounts();
            ok = false;
        }
    } else {
        unk_2b = flag;
        unk_0c[unk_2a] = lk;
        lk->unk_16 = ++unk_2a;
    }
    return ok;
}

Instr *cls0805c244::earliestExt()
{
    Instr *res = 0;
    cls0805bcc4 *lk = unk_20;
    if (!lk || lk->unk_2d != 2)
        return res;
    int best = 30000;
    for (cls0805bcc4 *ch = lk->unk_0c; ch; ch = ch->unk_08) {
        if (ch->unk_19 != 2) {
            Instr *in = ch->unk_2d == 1 ? (Instr *)ch->unk_24 : 0;
            if (!in) {
                func_0804ad38("not get in for earliest ext\n");
                func_0804adf0(-1);
            }
            if (in->unk_13e8.unk_00 < best) {
                best = in->unk_1396;
                res = in;
            }
        }
    }
    return res;
}

int cls0805c244::func_0805d3c4()
{
    cls0805bcc4 *lk = unk_1c;
    int n = 0;
    while (lk) {
        if (lk->unk_22)
            n++;
        lk = lk->unk_08;
    }
    return n;
}

bool freeLockLists(cls0805c244 *p)
{
    while (p) {
        cls0805c244 *next = p->unk_08;
        if (p)
            ::operator delete(p);
        p = next;
    }
    return true;
}

cls0805c244 *findLockList(cls0805c244 *p, RegPoolEntry *reg, int part)
{
    for (; p; p = p->unk_08) {
        if (p->unk_04 == reg && p->unk_02 == part) {
            if (reg->func_0805a908(part) && reg->func_0805a908(part) != (int)p) {
                func_0804ad38("rg points to rll, but rll list not it\n");
                func_0804adf0(-1);
            }
            return p;
        }
    }
    return 0;
}

const char *rlmtName(unsigned char t)
{
    switch (t) {
    case 1:
        return "RLMT_MOVEAGE";
    case 2:
        return "RLMT_AUTOAGE";
    case 0:
        return "RLMT_NOMOVE";
    default:
        return "RLMT_UNKNOWN";
    }
}

const char *cls0805bcc4::regName()
{
    if (!unk_04)
        return "NOT_ON_A_LOCKLIST";
    if (!unk_04->unk_04)
        return "RLL_HAS_NO_REG";
    return unk_04->unk_04->func_0805a8e0();
}

bool cls0805c244::setRepReg(RegPoolEntry *rg)
{
    if (unk_04 && rg != unk_04) {
        func_0804ad38("can't set representative reg - it's already set\n");
        return false;
    }
    unk_04 = rg;
    return true;
}

static inline void wrapRange(int &s, int &e)
{
    if (g_080b336c) {
        while (s >= g_080b336c) {
            s -= g_080b336c;
            e -= g_080b336c;
        }
    }
}

static inline void wrapRangeLocal(int &s, int &e)
{
    int n = g_080b336c;
    if (n) {
        while (s >= n) {
            s -= n;
            e -= n;
        }
    }
}

int cls0805c244::checkInputLock(cls0805bcc4 *in, short *pos, int *minPos)
{
    cls0805bcc4 *lk = unk_1c;
    int ret;
    int lsz = g_080b336c;
    ret = 0;
    int p = *pos;

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
        if (inside)
            return 0;
        int span = hi - lo;
        int room = lsz;
        if (unk_0c[0])
            room -= unk_0c[0]->unk_18;
        else {
            func_0804ad38("no ext output set. but .. not const for loop\n");
            func_0804adf0(-1);
        }
        if (span <= room)
            return 0;
        int need;
        if (p < par->unk_3e)
            need = par->unk_4e - room;
        else if (in->unk_19 == 1) {
            in->unk_1a = 1;
            return 0x100;
        } else
            need = 30000;
        if (*minPos < need)
            *minPos = need;
        return 0x2000;
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
        if (in->unk_19 == 1) {
            in->unk_1a = 3;
            return 0x100;
        }
        *minPos = 30000;
        return 0x400;
    }

    int s = par->unk_34;
    int e = s + d;
    wrapRangeLocal(s, e);

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
        } else if (func_0805db74(s, e, ls, le)) {
            if (g_080de288 && lsz && in->unk_19 == 1) {
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

int cls0805c244::checkOutputLock(cls0805bcc4 *out, short *pos, int *minPos)
{
    int lsz = g_080b336c;
    int ret = checkOverlap(out, pos, minPos);
    if (out->unk_2d == 2)
        return 0;
    int p = *pos;
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
    if (!out->unk_20)
        lim = first;
    else {
        int hi = out->unk_48, lo = out->unk_4a;
        if (hi < lo) { int t = hi; hi = lo; lo = t; }
        int d = out->unk_46 - (hi - lo);
        if (d > 0)
            hi += (d + 1) >> 1;
        if (hi > 0)
            hi--;
        lim = last + hi;
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
            if (h->unk_3e + lsz < last) {
                *minPos = 30000;
                return 0x20000;
            }
            int he = h->unk_4e + lsz;
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

    int sz = pos[2];
    int s = (first - p) + sz;
    int e = (lim - p) + sz;
    if (g_080b336c) {
        while (s >= g_080b336c) {
            s -= g_080b336c;
            e -= g_080b336c;
        }
    }

    for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
        if (ext && lk->unk_2d == 2)
            continue;
        int ls = lk->unk_34;
        int le = lk->unk_4c;
        if (g_080b336c) {
            while (ls >= g_080b336c) {
                ls -= g_080b336c;
                le -= g_080b336c;
            }
        }
        if (lk->unk_22) {
            if (lsz) {
                *minPos = 30000;
                return ret | 4;
            }
            if (e >= ls) {
                *minPos = 30000;
                return ret | 4;
            }
        } else if (func_0805db74(s, e, ls, le)) {
            first -= p;
            int v = lk->unk_4e - first + 1;
            if (g_080b336c) {
                int r = v % g_080b336c;
                int q = *pos / g_080b336c;
                if (r <= *pos % g_080b336c)
                    q++;
                v = q * g_080b336c + r;
            }
            if (*minPos < v)
                *minPos = v;
            return ret | 2;
        }
    }
    return ret;
}

int cls0805c244::checkOverlap(cls0805bcc4 *out, short *pos, int *minPos)
{
    int ret = 0;
    if (out->unk_17 > 1) {
        int s = pos[2];
        int e = out->unk_17 + s - 1;
        for (cls0805bcc4 *lk = unk_1c; lk; lk = lk->unk_08) {
            if (lk == out)
                continue;
            int os = lk->unk_30;
            int oe = lk->unk_17 + os;
            if (oe - 1 < 0)
                continue;
            if (func_0805db74(s, e, os, oe - 1)) {
                ret |= 1;
                int p = *pos;
                int v = oe;
                if (g_080b336c) {
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
    }
    return ret;
}

bool func_0805db74(int s1, int e1, int s2, int e2)
{
    if (g_080b336c) {
        int l1 = e1 - s1;
        int l2 = e2 - s2;
        wrapRange(s1, e1);
        wrapRange(s2, e2);
        bool r = true;
        if (l1 + l2 <= g_080b336c) {
            if (s1 <= s2)
                r = e1 >= s2 || e2 - g_080b336c >= s1;
            else
                r = s1 <= e2 || e1 - g_080b336c >= s2;
        }
        return r;
    }
    if (s1 <= s2)
        return e1 >= s2;
    return s1 <= e2;
}

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

int cls0805c244::canMoveInputLock(cls0805bcc4 *in, short *pos, int *minPos)
{
    int p = *pos;
    cls0805bcc4 *lk = unk_1c;
    int lsz = g_080b336c;
    if (unk_29)
        return 0;
    if (in->unk_19 == 2) {
        func_0804ad38("canMoveInputLock - aa input\n");
        func_0804aeb0();
    } else {
    cls0805bcc4 *par = in->getParent();
    bool cat = false;
    int s, e, end;
    if (lsz && par->unk_2d == 2) {
        cls0805bcc4 *x = unk_0c[0];
        if (x->unk_2c) {
            if (in->unk_24 == x->unk_24)
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
    if (lsz && p - s >= lsz)
        return 0x40000;
    int ws = s, we = p;
    wrapRange(ws, we);
    for (; lk; lk = lk->unk_08) {
        lk->print();
        if (lk == par)
            continue;
        if (lk->unk_24 == in->unk_24)
            continue;
        if (cat && lk->unk_2d == 2)
            continue;
        int ls = lk->unk_34;
        int le = lk->unk_4c;
        wrapRange(ls, le);
        if (lk->unk_22) {
            if (ws < le && we >= ls)
                return 0x10;
        } else if (func_0805db74(ws, we, ls, le))
            return 0x20;
    }
    }
    return 0;
}

int cls0805c244::canMoveOutputLock(cls0805bcc4 *out, short *pos, int *minPos)
{
    int ret;
    int lsz = g_080b336c;
    int p = *pos;
    int sz = pos[2];
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

bool instStream::func_0805e4bc()
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
        if (!logged) {
            func_0804ad38("failed to log\n");
            return false;
        }
    }

    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *r = (op->isReg() && op->unk_2b != 2) ? op->unk_18 : 0;
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
                                if (l > maxLat)
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
            RegPoolEntry *r = (op->isReg() && op->unk_2b != 2) ? op->unk_18 : 0;
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
            RegPoolEntry *r = (op->isReg() && op->unk_2b != 2) ? op->unk_18 : 0;
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
            RegPoolEntry *r = (op->isReg() && op->unk_2b != 2) ? op->unk_18 : 0;
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

    for (in = lst->unk_04; in; in = in->unk_1c.unk_04) {
        for (Instr *m = in->unk_1c.unk_04; m; m = m->unk_1c.unk_04) {
            if (func_080543bc(m, in)) {
                m->unk_13f0.unk_08++;
                m->unk_13f0.unk_0c += in->unk_13f0.unk_0c + 1;
                m->unk_13f0.unk_10 = in->unk_13f0.unk_10 + 1;
            }
        }
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
            RegPoolEntry *r = op->getReg2();
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
                    int d = op->unk_0a + hi;
                    if (d > maxIssue)
                        maxIssue = d;
                }
            }
        }
    }

    for (; both; both--) {
        if (upper < lower)
            upper++;
        else
            lower++;
    }
    if (upper > best) {
        unk_2ba = 3;
        best = upper;
    }
    if (lower > best) {
        unk_2ba = 4;
        best = lower;
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
    workTable *wt = unk_218;
    if (!wt)
        wt = getWorkTable();
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
    workTable *wt = unk_218;
    if (!wt)
        wt = getWorkTable();
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
        if (s - e > b->unk_38)
            mark = "****";
        int ht = b->unk_04 - in->unk_13f0.unk_00;
        func_0804acb0("[%3d]:%3d [%5d %5d %5d %5d ]%s dep=%2d ht=%2d  %s\n",
                      n, b->unk_38, e, s, in->unk_140c, in->unk_140e, mark,
                      in->unk_13f0.unk_04, ht, in->unk_34->unk_04);
        n++;
    }
    func_0804acb0("dyDuumpStats - end\n");
    errorOccurred = saved;
}
