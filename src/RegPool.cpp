#include "RegPool.h"
#include "ol_s_inline.h"
#include "os.h"
#include "VUStr.h"
#include "workTable.h"

struct iInfo;
iInfo* func_0806bf8c(const char* name, bool);
bool asciiCaseEqual(const char* a, const char* b);

const char *regNameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]_$";

const char *regTypeName(int type)
{
    const char *s = "UNKNOWN_TYPE";
    switch (type) {
    case 8:  s = "VPU_I"; break;
    case 1:  s = "CPU_GPR"; break;
    case 2:  s = "FPR"; break;
    case 6:  s = "CPU_HW"; break;
    case 9:  s = "VPU_VF"; break;
    case 10: s = "VPU_HW"; break;
    }
    return s;
}

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

bool RegPool::Init(VclPtrWord registerPrefix, const char* name, int cls, int virtualRegisterParts)
{
    unk_28 = registerPrefix;
    unk_18 = virtualRegisterParts;
    unk_2c = name;
    unk_04 = 0;
    unk_08 = 0;
    unk_20 = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_0c = cls;

    int registerCount = 0;
    int descriptorIndex;
    RegPoolEntry* entry;
    RegDesc* descriptor;
    for (descriptor = regDescTable; descriptor->names[0]; descriptor++)
        if (descriptor->registerClass == cls)
            registerCount++;
    if (!registerCount)
        return true;

    RegPoolEntry* tab = new RegPoolEntry[registerCount];
    unk_04 = tab;
    if (!tab) {
        reportError("RegPool::Init - couldn't allocate table\n");
        return false;
    }
    entry = tab;
    unk_08 = registerCount;

    descriptor = regDescTable;
    descriptorIndex = 0;
    int classRunState = 0;
    for (; descriptor->names[0]; descriptor++, descriptorIndex++) {
        if (descriptor->registerClass != cls) {
            if (classRunState)
                classRunState = 2;
            continue;
        }
        if (classRunState == 2) {
            reportError("misordered uid\n");
            return false;
        }
        classRunState = 1;
        for (int i = 0; i != 4; i++) {
            if (descriptor->names[i] && !entry->func_0805a8a4(descriptor->names[i])) {
                reportError("failed to add register alias!\n");
                return false;
            }
        }
        entry->unk_15 = descriptor->allocatable != 0;
        entry->registerClass = descriptor->registerClass;
        entry->unk_04 = this;
        entry->partCount = descriptor->partCount;
        entry->unk_2c = descriptor->unk_14 != 0;
        entry->unk_1c = descriptor->nonBlockId;
        if (entry->unk_1c && entry->partCount != 1) {
            reportError("nonblockid is set, but nparts!=1.. register is %s\n", entry->registerName());
            return false;
        }
        entry->unk_30 = descriptorIndex;
        entry++;
    }
    return true;
}

void RegPool::Dump()
{
    if (!unk_2c) {
        reportError("RegPool::Dump() POOL_WITH_NO_NAME\n");
        return;
    }
    int current = 0;
    if (unk_04) {
        int i = 0;
        for (; current != unk_08; current = ++i) {
            RegPoolEntry* e = &unk_04[i];
            int offset = (char*)e - (char*)unk_04;
            for (int p = offset; p; p = 0)
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

static inline bool isRegNameChar(char c)
{
    for (char* q = (char*)regNameChars; *q != '\0'; q++)
        if (*q == c)
            return true;
    return false;
}

bool RegPool::isResvd(const char* name, int cls)
{
    if (!name) {
        reportError("isResvd. passed a null string as name\n");
        exitAfterCleanup(-1);
    }
    const char* p;
    for (p = name; *p; p++) {
        char c = *p;
        if (!isRegNameChar(c)) {
            reportUserMessage("can't use '%c' in the name of a register : '%s'\n", c, name);
            return true;
        }
    }
    unsigned int len = p - name;
    if (len > 62) {
        reportUserMessage("register name '%s' is too long\n", name);
        return true;
    }
    if (len > 1 && func_0806bf8c(name, true)) {
        reportUserMessage("invalid name for register '%s'\n", name);
        return true;
    }
    if (cls == 8) {
        if (asciiCaseEqual(name, "i") || asciiCaseEqual(name, "p")
            || asciiCaseEqual(name, "q") || asciiCaseEqual(name, "r"))
            return false;
    }
    for (RegDesc* d = regDescTable; d->names[0]; d++) {
        if (d->registerClass == cls)
            continue;
        for (int i = 0; i != 3; i++)
            if (d->names[i] && asciiCaseEqual(name, d->names[i]))
                return true;
    }
    return false;
}

bool RegPool::func_08059828(bool flag)
{
    RegPoolEntry* e = unk_04;
    for (int i = 0; i != unk_08; i++, e++)
        if (!e->func_0805a4f4(flag)) {
            reportError("cycle: fail for %s\n", e->registerName());
            return false;
        }
    for (RegPoolEntry* p = unk_1c; p; p = p->next)
        if (!p->func_0805a4f4(flag)) {
            reportError("cycle: fail for %s\n", p->registerName());
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
        if (asciiCaseEqual(p->registerName(), name))
            break;
    return p;
}

RegPoolEntry* RegPool::func_08059944(const char* name)
{
    if (!unk_18) {
        RegPoolEntry* e = func_08059418(name);
        if (!e)
            reportError("failed to find reg, and don't allow virtuals - pool %s regname=%s\n", unk_2c, name);
        return e;
    }
    RegPoolEntry* e = func_08059418(name);
    if (e)
        return e;
    if (isResvd(name, unk_0c)) {
        reportUserMessage("can't use %s as a name for one of the %s\n", name, unk_2c);
        return 0;
    }
    e = findVirtual(unk_1c, name);
    if (!e) {
        e = func_08059a5c(name);
        if (!e) {
            reportError("failed to create a virtual reg\n");
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
        reportWarning(" name '%s' was reserved\n", name);
    RegPoolEntry* e;
    for (e = unk_1c; e; e = e->next)
        if (asciiCaseEqual(e->registerName(), name))
            break;
    if (e) {
        reportWarning("WARNING: AddVirtual : name already found '%s'!\n", name);
        return 0;
    }
    e = new RegPoolEntry;
    if (!e) {
        reportError("failed to get a new virtual register\n");
        return 0;
    }
    e->unk_04 = this;
    e->registerClass = unk_0c;
    e->unk_0c = 0;
    e->next = 0;
    e->partCount = unk_18;
    e->unk_30 = unk_0c * 2000 + unk_24;
    e->unk_14 = true;
    e->unk_1c = 0;
    if (!e->func_0805a8a4(name)) {
        reportError("failed to christen name to register %s \n", name);
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
        reportError("regPool: add all to use - but had no table\n");
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
    reportError("failed to resetlalloc\n");
    return false;
}

int maxParts_080bce20;
int nRegs_080bce24;
int tStart_080bce28;
int tEnd_080bce2c;
char aTab_080bce40[0x20000];
char tTab_080dce40[0x1000];
char regMask_080dde40[32];
int moveRegCount;

RegPoolEntry* RegPool::getMoveReg(Instr* move)
{
    char buf[128];
    formatIntoBuffer(buf, "move__m%d", moveRegCount);
    moveRegCount++;
    if (!move) {
        reportError("getMoveReg -  but no move\n");
        exitAfterCleanup(-1);
    }
    RegPoolEntry* e = func_08059a5c(buf);
    if (!e) {
        reportError("failed to add a new virtual register!\n");
        exitAfterCleanup(-1);
    }
    if (e->unk_20) {
        reportError("move inst already set in getMoveReg\n");
        exitAfterCleanup(-1);
    }
    e->unk_20 = move;
    return e;
}

bool RegPool::func_08059e20(RegPoolEntry* e)
{
    return e && e->registerClass == unk_0c;
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
    buf += formatIntoBuffer(buf, "; %3d %s\n", countUsed(), unk_2c);
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
    for (i = 0; i != partCount; i++) {
        rPart* s = getRP(i);
        ol_s* c = &s->unk_24;
        if (c->unk_12)
            out[n++] = c;
        int j;
        ol_s* p;
        for (j = 0, p = s->unk_4c; j != 4; j++, p++)
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
