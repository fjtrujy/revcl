#include "Instr.h"
#include "os.h"

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
    if (lk->unk_04 != this) {
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
        if (rl == crl && rl >= 0) {
            func_0804ad38("during a lock sort for output locks, two had the same rline..\n");
            checkCounts();
            func_0804adf0(-1);
        }
        int line = lk->unk_32;
        int cline = cur->unk_32;
        if (line < cline)
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
