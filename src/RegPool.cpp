#pragma implementation "regUseSlot.h"
#include "RegPool.h"
#include "ol_s_inline.h"
#include "os.h"
#include "VUStr.h"
#include "workTable.h"

struct iInfo;
iInfo* func_0806bf8c(const char* name, bool);
bool func_0804bfcc(const char* a, const char* b);

RegPool::RegPool()
{
    unk_28 = 0;
    unk_2c = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_18 = 0;
    unk_20 = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_10 = 0;
    unk_14 = 0;
    unk_0e = 0;
}

RegPool::~RegPool()
{
    RegPoolEntry* p = unk_1c;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    unk_1c = unk_20 = 0;
    unk_24 = 0;
    delete[] unk_04;
}

bool RegPool::Init(int a, const char* name, int cls, int b)
{
    unk_28 = a;
    unk_18 = b;
    unk_2c = name;
    unk_04 = 0;
    unk_08 = 0;
    unk_20 = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_0c = cls;

    int n = 0;
    int idx;
    RegPoolEntry* e;
    RegDesc* d;
    for (d = regDescTable; d->names[0]; d++)
        if (d->unk_11 == cls)
            n++;
    if (!n)
        return true;

    RegPoolEntry* tab = new RegPoolEntry[n];
    unk_04 = tab;
    if (!tab) {
        func_0804ad38("RegPool::Init - couldn't allocate table\n");
        return false;
    }
    e = tab;
    unk_08 = n;

    d = regDescTable;
    idx = 0;
    int state = 0;
    for (; d->names[0]; d++, idx++) {
        if (d->unk_11 != cls) {
            if (state)
                state = 2;
            continue;
        }
        if (state == 2) {
            func_0804ad38("misordered uid\n");
            return false;
        }
        state = 1;
        for (int i = 0; i != 4; i++) {
            if (d->names[i] && !e->func_0805a8a4(d->names[i])) {
                func_0804ad38("failed to add register alias!\n");
                return false;
            }
        }
        e->unk_15 = d->unk_10 != 0;
        e->unk_24 = d->unk_11;
        e->unk_04 = this;
        e->unk_28 = d->unk_12;
        e->unk_2c = d->unk_14 != 0;
        e->unk_1c = d->unk_13;
        if (e->unk_1c && e->unk_28 != 1) {
            func_0804ad38("nonblockid is set, but nparts!=1.. register is %s\n", e->func_0805a8e0());
            return false;
        }
        e->unk_30 = idx;
        e++;
    }
    return true;
}

void RegPool::Dump()
{
    if (!unk_2c) {
        func_0804ad38("RegPool::Dump() POOL_WITH_NO_NAME\n");
        return;
    }
    if (unk_04) {
        for (int i = 0; i != unk_08; i++) {
            RegPoolEntry* e = &unk_04[i];
            for (RegPoolEntry* p = e; p; p = 0)
                ;
        }
        if (unk_18) {
            for (RegPoolEntry* p = unk_1c; p; p = p->next)
                ;
        }
    }
}

RegPoolEntry* RegPool::func_080593e0(int index)
{
    if (!unk_04)
        return 0;
    int i = index - unk_04->unk_30;
    if (i < 0 || i >= unk_08)
        return 0;
    return &unk_04[i];
}

RegPoolEntry* RegPool::func_08059418(const char* name)
{
    RegPoolEntry* e = unk_04;
    for (int i = 0; i != unk_08; i++, e++)
        if (e->unk_2b8.func_0804b284(name))
            return e;
    return 0;
}

bool RegPool::func_08059464()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->func_0805a390();
        for (RegPoolEntry* p = unk_1c; p; p = p->next)
            p->func_0805a390();
    }
    return true;
}

bool RegPool::func_080594c0()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->func_0805a348();
        for (RegPoolEntry* p = unk_1c; p; p = p->next)
            p->func_0805a348();
    }
    return true;
}

bool RegPool::func_0805951c()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->func_0805a300();
    }
    for (RegPoolEntry* p = unk_1c; p; p = p->next)
        p->func_0805a300();
    return true;
}

bool RegPool::func_08059578()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->func_0805a4b4();
        for (RegPoolEntry* p = unk_1c; p; p = p->next)
            p->func_0805a4b4();
    }
    return true;
}

bool RegPool::func_080595d4()
{
    RegPoolEntry* p = unk_1c;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    unk_1c = unk_20 = 0;
    unk_24 = 0;
    RegPoolEntry* e = unk_04;
    for (int i = 0; i != unk_08; i++, e++) {
        e->unk_16 = false;
        e->unk_14 = false;
    }
    return true;
}

int func_0805968c(const char* name)
{
    return RegPool::isResvd(name, -123);
}

static inline bool isRegNameChar(const char* c)
{
    for (const char* q = regNameChars; *q; q++)
        if (*q == *c)
            return true;
    return false;
}

bool RegPool::isResvd(const char* name, int cls)
{
    if (!name) {
        func_0804ad38("isResvd. passed a null string as name\n");
        func_0804adf0(-1);
    }
    const char* p;
    for (p = name; *p; p++) {
        if (!isRegNameChar(p)) {
            func_0804ad84("can't use '%c' in the name of a register : '%s'\n", *p, name);
            return true;
        }
    }
    unsigned int len = p - name;
    if (len > 62) {
        func_0804ad84("register name '%s' is too long\n", name);
        return true;
    }
    if (len > 1 && func_0806bf8c(name, true)) {
        func_0804ad84("invalid name for register '%s'\n", name);
        return true;
    }
    if (cls == 8) {
        if (func_0804bfcc(name, "i") || func_0804bfcc(name, "p")
            || func_0804bfcc(name, "q") || func_0804bfcc(name, "r"))
            return false;
    }
    for (RegDesc* d = regDescTable; d->names[0]; d++) {
        if (d->unk_11 == cls)
            continue;
        for (int i = 0; i != 3; i++)
            if (d->names[i] && func_0804bfcc(name, d->names[i]))
                return true;
    }
    return false;
}

bool RegPool::func_08059828(bool flag)
{
    RegPoolEntry* e = unk_04;
    for (int i = 0; i != unk_08; i++, e++)
        if (!e->func_0805a4f4(flag)) {
            func_0804ad38("cycle: fail for %s\n", e->func_0805a8e0());
            return false;
        }
    for (RegPoolEntry* p = unk_1c; p; p = p->next)
        if (!p->func_0805a4f4(flag)) {
            func_0804ad38("cycle: fail for %s\n", p->func_0805a8e0());
            return false;
        }
    return true;
}

bool RegPool::func_080598b0()
{
    RegPoolEntry* p = unk_1c;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    unk_1c = unk_20 = 0;
    unk_24 = 0;
    return true;
}

static inline RegPoolEntry* findVirtual(RegPoolEntry* p, const char* name)
{
    for (; p; p = p->next)
        if (func_0804bfcc(p->func_0805a8e0(), name))
            break;
    return p;
}

RegPoolEntry* RegPool::func_08059944(const char* name)
{
    if (!unk_18) {
        RegPoolEntry* e = func_08059418(name);
        if (!e)
            func_0804ad38("failed to find reg, and don't allow virtuals - pool %s regname=%s\n", unk_2c, name);
        return e;
    }
    RegPoolEntry* e = func_08059418(name);
    if (e)
        return e;
    if (isResvd(name, unk_0c)) {
        func_0804ad84("can't use %s as a name for one of the %s\n", name, unk_2c);
        return 0;
    }
    e = findVirtual(unk_1c, name);
    if (!e) {
        e = func_08059a5c(name);
        if (!e) {
            func_0804ad38("failed to create a virtual reg\n");
            return 0;
        }
    }
    return e;
}

RegPoolEntry* RegPool::func_08059a1c(const char* name)
{
    return findVirtual(unk_1c, name);
}

RegPoolEntry* RegPool::func_08059a5c(const char* name)
{
    if (isResvd(name, unk_0c))
        func_0804ac7c(" name '%s' was reserved\n", name);
    RegPoolEntry* e;
    for (e = unk_1c; e; e = e->next)
        if (func_0804bfcc(e->func_0805a8e0(), name))
            break;
    if (e) {
        func_0804ac7c("WARNING: AddVirtual : name already found '%s'!\n", name);
        return 0;
    }
    e = new RegPoolEntry;
    if (!e) {
        func_0804ad38("failed to get a new virtual register\n");
        return 0;
    }
    e->unk_04 = this;
    e->unk_24 = unk_0c;
    e->unk_0c = 0;
    e->next = 0;
    e->unk_28 = unk_18;
    e->unk_30 = unk_0c * 2000 + unk_24;
    e->unk_14 = true;
    e->unk_1c = 0;
    if (!e->func_0805a8a4(name)) {
        func_0804ad38("failed to christen name to register %s \n", name);
        delete e;
        return 0;
    }
    if (unk_1c) {
        unk_20->next = e;
        e->unk_0c = unk_20;
        unk_20 = e;
    } else {
        unk_20 = e;
        unk_1c = e;
    }
    unk_24++;
    return e;
}

bool RegPool::func_08059c00()
{
    if (!unk_04) {
        func_0804ad38("regPool: add all to use - but had no table\n");
        return false;
    }
    RegPoolEntry* e = unk_04;
    for (int i = 0; i != unk_08; i++, e++)
        if (e->unk_15)
            e->unk_16 = true;
    return true;
}

bool RegPool::func_08059c4c()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->unk_18 = 0;
    }
    for (RegPoolEntry* p = unk_1c; p; p = p->next)
        p->unk_18 = 0;
    return true;
}

bool RegPool::func_08059c94()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->unk_18 = 0;
    }
    for (RegPoolEntry* p = unk_1c; p; p = p->next)
        p->unk_18 = 0;
    return true;
}

bool RegPool::func_08059cdc()
{
    for (RegPoolEntry* p = unk_1c; p; p = p->next) {
        if (!p->func_0805aa00())
            goto fail;
        p->unk_18 = 0;
        p->unk_3c = -1;
    }
    {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++) {
            if (!e->func_0805aa00())
                goto fail;
            e->unk_18 = 0;
        }
    }
    return true;
fail:
    func_0804ad38("failed to resetlalloc\n");
    return false;
}

int moveRegCount;

RegPoolEntry* RegPool::getMoveReg(Instr* move)
{
    char buf[128];
    func_0804adbc(buf, "move__m%d", moveRegCount);
    moveRegCount++;
    if (!move) {
        func_0804ad38("getMoveReg -  but no move\n");
        func_0804adf0(-1);
    }
    RegPoolEntry* e = func_08059a5c(buf);
    if (!e) {
        func_0804ad38("failed to add a new virtual register!\n");
        func_0804adf0(-1);
    }
    if (e->unk_20) {
        func_0804ad38("move inst already set in getMoveReg\n");
        func_0804adf0(-1);
    }
    e->unk_20 = move;
    return e;
}

bool RegPool::func_08059e20(RegPoolEntry* e)
{
    return e && e->unk_24 == unk_0c;
}

bool RegPool::optReset()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->optReset();
        for (RegPoolEntry* p = unk_1c; p; p = p->next)
            p->optReset();
    }
    return true;
}

bool RegPool::emitReset()
{
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            e->unk_35 = 0;
        for (RegPoolEntry* p = unk_1c; p; p = p->next)
            p->unk_35 = 0;
    }
    return true;
}

char* RegPool::emitStats(char* buf)
{
    buf += func_0804adbc(buf, "; %3d %s\n", countUsed(), unk_2c);
    return buf;
}

int RegPool::regIndex(int reg)
{
    int r = -1;
    if (unk_04) {
        reg -= unk_04->unk_30;
        if (reg >= 0 && reg < unk_08)
            r = reg;
    }
    return r;
}

int RegPool::countUsed()
{
    int n = 0;
    if (unk_04) {
        RegPoolEntry* e = unk_04;
        for (int i = 0; i != unk_08; i++, e++)
            if (e->unk_35)
                n++;
    }
    return n;
}

int RegPool::func_08059f70()
{
    int n = 0;
    if (unk_04) {
        for (int i = 0; i != unk_08; i++)
            if (unk_04[i].unk_16)
                n++;
    }
    return n;
}

int RegPoolEntry::gatherInflight(ol_s** out)
{
    int n = 0;
    int i;
    for (i = 0; i != unk_28; i++) {
        rPart* s = getRP(i);
        if (s->unk_24.unk_12)
            out[n++] = &s->unk_24;
        ol_s* p = s->unk_4c;
        for (int j = 0; j != 4; j++, p++)
            if (p->unk_12)
                out[n++] = p;
    }
    return n;
}

int RegPool::gatherInflight(ol_s** out)
{
    int n = 0;
    for (int i = 0; i != unk_08; i++) {
        RegPoolEntry* e = &unk_04[i];
        int r = e->gatherInflight(out + n);
        if (r < 0)
            return -1;
        n += r;
    }
    for (RegPoolEntry* p = unk_1c; p; p = p->next) {
        int r = p->gatherInflight(out + n);
        if (r < 0)
            return -1;
        n += r;
    }
    return n;
}
