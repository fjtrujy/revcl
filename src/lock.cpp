#include "Instr.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"
#include "Block.h"
#include "statCounts.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
bool g_080de284;

bool cls0805bcc4::func_0805bcc4()
{
    unk_22 = 0;
    unk_20 = 0;
    unk_0c = 0;
    unk_08 = 0;
    unk_16 = 0;
    unk_17 = 0;
    unk_18 = 0;
    unk_15 = 0;
    unk_1c = 0;
    unk_2c = 0;
    unk_19 = 0;
    unk_10 = 0;
    unk_2e = 0;
    unk_48 = 0;
    unk_4a = 0;
    unk_46 = 0;
    unk_02 = 0;
    unk_01 = 0;
    unk_00 = 0;
    unk_24 = 0;
    unk_2d = 0;
    unk_14 = 0;
    unk_04 = 0;
    unk_28 = -1;
    unk_29 = -1;
    unk_32 = -1;
    unk_30 = -1;
    unk_34 = -1;
    unk_36 = -1;
    unk_38 = -1;
    unk_3a = -1;
    unk_3c = -1;
    unk_3e = -1;
    unk_40 = -1;
    unk_42 = -1;
    unk_4e = -1;
    unk_4c = -1;
    return true;
}

bool cls0805bcc4::setOwner(int part, Instr *instr, int opnd)
{
    if (!instr) {
        reportError("setowner - passed null\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_24) {
        reportError("owner already set for rlock set owner\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_2d) {
        reportError("owner: type wasn't null when try to set\n");
        return false;
    }
    if (unk_04) {
        reportError("setowner - lock was already on list\n");
        exitAfterCleanup(-1);
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
        reportError("setowner - passed null rf\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_24) {
        reportError("owner already set for rlock set owner\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_2d) {
        reportError("owner: type wasn't null when try to set\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_04) {
        reportError("setowner - lock was already on list\n");
        exitAfterCleanup(-1);
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
        return &instr->operands[opnd];
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
    reportError("failed to get part - lock incorrect oType\n");
    exitAfterCleanup(-1);
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
            reportError("type inst..  can't get it though!\n");
            exitAfterCleanup(-1);
        }
        instr->func_08054008(false);
        break;
    }
    case 2: {
        regUse *rf = unk_2d == 2 ? (regUse *)unk_24 : 0;
        if (!rf) {
            reportError("type regref..  can't get it though!\n");
            exitAfterCleanup(-1);
        }
        break;
    }
    default:
        reportError("??????????? unkown lock\n");
        exitAfterCleanup(-1);
    }
}

cls0805bcc4 *cls0805bcc4::getParent()
{
    if (unk_14 != 1) {
        reportError("getParent - called on non input lock\n");
        print();
        return 0;
    }
    if (!unk_0c) {
        reportWarning("getParent: not set!\n");
        print();
        return 0;
    }
    if (unk_0c->unk_14 != 2) {
        reportError("parent was not output!\n");
        reportError("we are:\n");
        print();
        reportError("parent is\n");
        unk_0c->print();
        exitAfterCleanup(-1);
    }
    return unk_0c;
}

bool cls0805bcc4::setInOut(cls0805bcc4 *olk)
{
    if (!olk) {
        reportError("setinout - passed null\n");
        exitAfterCleanup(-1);
    }
    if (olk->unk_14 != 2) {
        reportError("setinout - olk wasn't output\n");
        exitAfterCleanup(-1);
    }
    if (!unk_24) {
        reportError("owner not set for inout\n");
        exitAfterCleanup(-1);
    }
    if (unk_24 != olk->unk_24) {
        reportError("setinout -- inst bad\n");
        exitAfterCleanup(-1);
    }
    if (unk_28 != olk->unk_28) {
        reportError("set in out - part mismatched\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_1c) {
        reportError("rlock - set inout - already set\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_14 != 1) {
        reportError("set inout on non input\n");
        print();
        exitAfterCleanup(-1);
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
        reportError("ERROR - nOutLocks != nToGo + nDone\n");
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
        reportError("Addoutputlock - null pointer\n");
        return false;
    }
    if (lk->unk_14 != 2) {
        reportError("Addoutputlock - bad type\n");
        return false;
    }
    if (lk->unk_2c) {
        reportError("Addoutputlock - already inserted\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (!isOwnedBy(lk, this)) {
        reportError("Addoutput lock - I don't own it\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lk->unk_0c) {
        reportError("AddOutputLock: m_Link already set!\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lk->unk_22 != lk->unk_20) {
        reportError("Addoutputlock - mRefCount!=m_RefCount\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lk->unk_08) {
        reportError("AddOutputLock:: m_Next already set\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (unk_26 <= 0) {
        reportError("add output lock - count was already down to zero or less\n");
        checkCounts();
        exitAfterCleanup(-1);
        return false;
    }
    unk_26--;
    lk->unk_08 = unk_1c;
    unk_1c = lk;
    if (!lk->calcStats(0)) {
        reportError("AddOutputLock failed to calcStats\n");
        checkCounts();
        exitAfterCleanup(-1);
    }
    lk->unk_2c = 1;
    lk->unk_10 = 0;
    cls0805bcc4 *prev = 0;
    for (cls0805bcc4 *cur = unk_20; cur; cur = cur->unk_10) {
        int rl = lk->unk_30;
        int crl = cur->unk_30;
        if ((short)rl == (short)crl && (short)rl >= 0) {
            reportError("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            exitAfterCleanup(-1);
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
        reportError("AddInputLock failed to calcStats\n");
        exitAfterCleanup(-1);
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
        reportError("UndoInputLock failed to calcStats\n");
        exitAfterCleanup(-1);
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
        reportError("UndoOutputLock failed to calcStats\n");
        checkCounts();
        exitAfterCleanup(-1);
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
        reportError("couldn't find lock on the sorted list\n");
        exitAfterCleanup(-1);
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

static inline Operand *lockOperand(cls0805bcc4 *lk)
{
    if (lk->unk_2d == 1) {
        int opnd = lk->unk_29;
        Instr *instr = (Instr *)lk->unk_24;
        return &instr->operands[opnd];
    }
    return 0;
}

static inline bool isInserted(cls0805bcc4 *lk)
{
    return lk->unk_2c != 0;
}

bool cls0805bcc4::isOk(int type, bool inserted)
{
    int loopSize = g_080b336c;
    bool ok = true;
    regUse *rf = unk_2d == 2 ? (regUse *)unk_24 : 0;
    Operand *op = lockOperand(this);

    if (rf) {
        if (op) {
            reportError("rLock::isOk() - both reg ref and op ok\n");
            ok = false;
        }
    } else if (!op) {
        reportError("rLock::isOk() - neither op nor regref set\n");
        ok = false;
    }
    if (type >= 0 && unk_14 != type) {
        reportError("isOk - unexpected type for is ok\n");
        ok = false;
    }
    if (isInserted(this) != inserted) {
        reportError("isOK - insert state was unexpected!\n");
        ok = false;
    }
    switch (unk_14) {
    case 1:
        if (!getParent()) {
            reportError("input lock has no parent\n");
            ok = false;
        }
        break;
    case 2: {
        if (!inserted && unk_20 != unk_22) {
            reportError("output lock - ref count mismatch\n");
            return false;
        }
        regUse *orf = unk_2d == 2 ? (regUse *)unk_24 : 0;
        if (orf && loopSize) {
            if (unk_18 > 1) {
                reportError("external input had latency for loop\n");
                ok = false;
            }
            if (unk_17 > 1) {
                reportError("external input had thruput for loop\n");
                ok = false;
            }
        }
        if (loopSize) {
            if (unk_18 > loopSize && unk_15 && unk_20) {
                reportError("latency bigger than loop!\n");
                ok = false;
            }
            if (unk_17 > loopSize) {
                reportError("thruput bigger than loop!\n");
                ok = false;
            }
        }
        break;
    }
    default:
        reportError("rLock::isOk() - bad lock type\n");
        ok = false;
        break;
    }
    if (!ok) {
        reportError("rLock::isOk - failed\n");
        print();
        exitAfterCleanup(-1);
    }
    return ok;
}

static inline bool ownsLock(cls0805c244 *list, cls0805bcc4 *lk)
{
    if (lk->unk_04 == list)
        return true;
    return false;
}

bool cls0805c244::isOk(cls0805bcc4 *lk, int type, bool inserted)
{
    if (!lk) {
        reportError("rLockList::isOk  -passed a null lock\n");
        exitAfterCleanup(-1);
    }
    bool ok = lk->isOk(type, inserted);
    if (lk->unk_14 == 0) {
        cls0805bcc4 *parent = lk->getParent();
        cls0805bcc4 *p;
        for (p = unk_1c; p; p = p->unk_08)
            if (p == parent)
                break;
        if (!p) {
            reportError("rLockList::isOk - parent is not on insert list\n");
            ok = false;
        }
    }
    if (!ownsLock(this, lk)) {
        reportError("isok - I don't own it\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (lk->unk_14 == 2) {
        if (unk_24 <= 0) {
            reportError("isOk - output, but no outputs! - <=0 output locks!\n");
            ok = false;
        }
        int lim = (inserted == false) - 1;
        if (unk_26 <= lim) {
            reportError("isOk - output, but no outputs! - <=0 output locks to go!\n");
            ok = false;
        }
    }
    if (!ok)
        exitAfterCleanup(-1);
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
                    unk_36 = unk_32 + unk_18;
                unk_3a = unk_32 + unk_18;
            }
        } else if (isRegLock(this)) {
            if (unk_18 == 0) {
                unk_36 = 0;
                unk_3a = 0;
            } else {
                if (unk_15)
                    unk_36 = 0;
                else
                    unk_36 = unk_32 + unk_18;
                unk_3a = unk_32 + unk_18;
            }
        } else {
            if (unk_15)
                unk_36 = unk_32 + 1;
            else
                unk_36 = unk_32 + unk_18;
            unk_3a = unk_32 + unk_18;
        }
        if (unk_3e < unk_3a) {
            if (g_080de284) {
                reportError("rLock::calcStats - rfUsed before result ready..rfu=%d rr=%d!\n", unk_3e, unk_3a);
                print();
                return false;
            }
            if (minUse < unk_36)
                reportWarning("rLock::calcStats - stalls non fatal, but.. rfUsed_nolh before result write..rfu=%d rw=%d..this maybe important\n", minUse, unk_36);
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
                    unk_36 = unk_32 + unk_18;
                unk_3a = unk_32 + unk_18;
            }
        } else {
            if (unk_15)
                unk_36 = unk_32 + 1;
            else
                unk_36 = unk_32 + unk_18;
            unk_3a = unk_32 + unk_18;
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
            int hi, lo;
            int a = unk_48, b = unk_4a;
            int len = unk_46;
            hi = a; lo = b;
            if (a < b) { hi = b; lo = a; }
            int d = hi - lo;
            len -= d;
            if (len > 0)
                hi += (len + 1) >> 1;
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

void cls0805bcc4::setFrom(InstructionPosition *r)
{
    int rline = r->unk_00;
    unk_32 = rline;
    int loop = r->unk_04;
    unk_30 = loop;
    int lsz = g_080b336c;
    if (lsz) {
        if (unk_32 % lsz == unk_30)
            return;
        reportError("rLock::setFrom: rline/loop mismatch %d vs %d (lsz%d)\n", unk_32, unk_30, lsz);
    } else {
        if (unk_32 == unk_30)
            return;
        reportError("setFrom rline/non-loop mismatch\n");
    }
    exitAfterCleanup(-1);
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
        reportError("moveoutputlock - null pointer\n");
        return false;
    }
    if (lk->unk_14 != 2) {
        reportError("moveoutputlock - bad type\n");
        return false;
    }
    if (!lk->unk_2c) {
        reportError("moveoutputlock - not already inserted\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (!lk->calcStats(0)) {
        reportError("moveoutputlock failed to set lUsed\n");
        checkCounts();
        exitAfterCleanup(-1);
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
            reportError("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            exitAfterCleanup(-1);
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
        reportError("moveinputlock failed to calcStats\n");
        exitAfterCleanup(-1);
    }
    return true;
}

static inline bool singleExtOutputMode()
{
    if (g_080de284 == 1)
        return true;
    return false;
}

bool cls0805c244::addExtOutput(cls0805bcc4 *lk, bool flag)
{
    if (lk->unk_16) {
        reportError("lock is already external output\n");
        return false;
    }
    if (unk_2a > 3) {
        reportError("too many extern outputs\n");
        return false;
    }
    if (unk_2a && singleExtOutputMode()) {
        if (flag)
            return true;
        reportError("more than one xo for rlock uta..count=%d\n", unk_2a);
        checkCounts();
        return false;
    }
    unk_2b = flag;
    unk_0c[unk_2a] = lk;
    lk->unk_16 = ++unk_2a;
    return true;
}

static inline bool hasRegUseOwner(cls0805bcc4 *lk)
{
    if (lk->unk_2d == 2)
        return true;
    return false;
}

Instr *cls0805c244::earliestExt()
{
    Instr *res = 0;
    cls0805bcc4 *lk = unk_20;
    if (lk) {
        if (hasRegUseOwner(lk)) {
            int best = 30000;
            for (cls0805bcc4 *ch = lk->unk_0c; ch; ch = ch->unk_08) {
                if (ch->unk_19 != 2) {
                    Instr *in = ch->unk_2d == 1 ? (Instr *)ch->unk_24 : 0;
                    if (!in) {
                        reportError("not get in for earliest ext\n");
                        exitAfterCleanup(-1);
                    }
                    if (in->position.unk_00 < best) {
                        best = in->insertionSerial;
                        res = in;
                    }
                }
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
            if (reg->getPartPayload(part) && reg->getPartPayload(part) != (VclPtrWord)p) {
                reportError("rg points to rll, but rll list not it\n");
                exitAfterCleanup(-1);
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
    cls0805c244 *ll = unk_04;
    if (!ll)
        return "NOT_ON_A_LOCKLIST";
    RegPoolEntry *rg = ll->unk_04;
    if (!rg)
        return "RLL_HAS_NO_REG";
    return rg->registerName();
}

bool cls0805c244::setRepReg(RegPoolEntry *rg)
{
    if (unk_04 && rg != unk_04) {
        reportError("can't set representative reg - it's already set\n");
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
