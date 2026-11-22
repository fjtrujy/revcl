#include "RegPool.h"
#include "ol_s_inline.h"
#include "os.h"
#include "VUStr.h"
#include "workTable.h"

struct iInfo;
iInfo* findOpcodeInfoByMnemonic(const char* name, bool);
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
    registerPrefix = 0;
    poolName = 0;
    registerEntries = 0;
    entryCount = 0;
    registerClass = 0;
    virtualPartCount = 0;
    virtualTail = 0;
    virtualHead = 0;
    virtualCount = 0;
    zeroRegister = 0;
    externalRegister = 0;
    unk_0e = 0;
}

RegPool::~RegPool()
{
    RegPoolEntry* p = virtualHead;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    delete[] registerEntries;
}

bool RegPool::Init(VclPtrWord registerPrefix, const char* name, int cls, int virtualRegisterParts)
{
    this->registerPrefix = registerPrefix;
    virtualPartCount = virtualRegisterParts;
    poolName = name;
    registerEntries = 0;
    entryCount = 0;
    virtualTail = 0;
    virtualHead = 0;
    virtualCount = 0;
    registerClass = cls;

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
    registerEntries = tab;
    if (!tab) {
        reportError("RegPool::Init - couldn't allocate table\n");
        return false;
    }
    entry = tab;
    entryCount = registerCount;

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
            if (descriptor->names[i] && !entry->addRegisterNameAlias(descriptor->names[i])) {
                reportError("failed to add register alias!\n");
                return false;
            }
        }
        entry->allocatable = descriptor->allocatable != 0;
        entry->registerClass = descriptor->registerClass;
        entry->ownerPool = this;
        entry->partCount = descriptor->partCount;
        entry->unk_2c = descriptor->unk_14 != 0;
        entry->nonBlockId = descriptor->nonBlockId;
        if (entry->nonBlockId && entry->partCount != 1) {
            reportError("nonblockid is set, but nparts!=1.. register is %s\n", entry->registerName());
            return false;
        }
        entry->registerIndex = descriptorIndex;
        entry++;
    }
    return true;
}

void RegPool::Dump()
{
    if (!poolName) {
        reportError("RegPool::Dump() POOL_WITH_NO_NAME\n");
        return;
    }
    int current = 0;
    if (registerEntries) {
        int i = 0;
        for (; current != entryCount; current = ++i) {
            RegPoolEntry* e = &registerEntries[i];
            int offset = (char*)e - (char*)registerEntries;
            for (int p = offset; p; p = 0)
                ;
        }
        if (virtualPartCount) {
            for (RegPoolEntry* p = virtualHead; p; p = p->next)
                ;
        }
    }
}

RegPoolEntry* RegPool::findByRegisterIndex(int index)
{
    if (!registerEntries)
        return 0;
    int i = index - registerEntries->registerIndex;
    if (i < 0 || i >= entryCount)
        return 0;
    return &registerEntries[i];
}

RegPoolEntry* RegPool::findPhysicalByName(const char* name)
{
    RegPoolEntry* e = registerEntries;
    for (int i = 0; i != entryCount; i++, e++)
        if (e->nameAliases.findByNameIgnoreCase(name))
            return e;
    return 0;
}

bool RegPool::resetPoolThroughputState()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->resetRegisterThroughputState();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->resetRegisterThroughputState();
    }
    return true;
}

bool RegPool::resetPoolPartTiming()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->resetRegisterPartTiming();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->resetRegisterPartTiming();
    }
    return true;
}

bool RegPool::resetPoolFlowState()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->resetRegisterFlowState();
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        p->resetRegisterFlowState();
    return true;
}

bool RegPool::clearPoolPartPayloads()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->clearRegisterPartPayloads();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->clearRegisterPartPayloads();
    }
    return true;
}

bool RegPool::clearVirtualRegistersForParse()
{
    RegPoolEntry* p = virtualHead;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    RegPoolEntry* e = registerEntries;
    for (int i = 0; i != entryCount; i++, e++) {
        e->unk_16 = false;
        e->isVirtual = false;
    }
    return true;
}

int isReservedRegisterName(const char* name)
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
    if (len > 1 && findOpcodeInfoByMnemonic(name, true)) {
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

bool RegPool::cycleRegisters(bool flag)
{
    RegPoolEntry* e = registerEntries;
    for (int i = 0; i != entryCount; i++, e++)
        if (!e->cycleRegisterParts(flag)) {
            reportError("cycle: fail for %s\n", e->registerName());
            return false;
        }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        if (!p->cycleRegisterParts(flag)) {
            reportError("cycle: fail for %s\n", p->registerName());
            return false;
        }
    return true;
}

bool RegPool::clearVirtualRegisters()
{
    RegPoolEntry* p = virtualHead;
    while (p) {
        RegPoolEntry* n = p->next;
        delete p;
        p = n;
    }
    virtualHead = virtualTail = 0;
    virtualCount = 0;
    return true;
}

static inline RegPoolEntry* findVirtual(RegPoolEntry* p, const char* name)
{
    for (; p; p = p->next)
        if (asciiCaseEqual(p->registerName(), name))
            break;
    return p;
}

RegPoolEntry* RegPool::findOrCreateRegister(const char* name)
{
    if (!virtualPartCount) {
        RegPoolEntry* e = findPhysicalByName(name);
        if (!e)
            reportError("failed to find reg, and don't allow virtuals - pool %s regname=%s\n", poolName, name);
        return e;
    }
    RegPoolEntry* e = findPhysicalByName(name);
    if (e)
        return e;
    if (isResvd(name, registerClass)) {
        reportUserMessage("can't use %s as a name for one of the %s\n", name, poolName);
        return 0;
    }
    e = findVirtual(virtualHead, name);
    if (!e) {
        e = createVirtualRegister(name);
        if (!e) {
            reportError("failed to create a virtual reg\n");
            return 0;
        }
    }
    return e;
}

RegPoolEntry* RegPool::findVirtualByName(const char* name)
{
    return findVirtual(virtualHead, name);
}

RegPoolEntry* RegPool::createVirtualRegister(const char* name)
{
    if (isResvd(name, registerClass))
        reportWarning(" name '%s' was reserved\n", name);
    RegPoolEntry* e;
    for (e = virtualHead; e; e = e->next)
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
    e->ownerPool = this;
    e->registerClass = registerClass;
    e->previous = 0;
    e->next = 0;
    e->partCount = virtualPartCount;
    e->registerIndex = registerClass * 2000 + virtualCount;
    e->isVirtual = true;
    e->nonBlockId = 0;
    if (!e->addRegisterNameAlias(name)) {
        reportError("failed to christen name to register %s \n", name);
        delete e;
        return 0;
    }
    if (virtualHead) {
        virtualTail->next = e;
        e->previous = virtualTail;
        virtualTail = e;
    } else {
        virtualTail = e;
        virtualHead = e;
    }
    virtualCount++;
    return e;
}

bool RegPool::markAllocatablePhysicalRegistersUsed()
{
    if (!registerEntries) {
        reportError("regPool: add all to use - but had no table\n");
        return false;
    }
    RegPoolEntry* e = registerEntries;
    for (int i = 0; i != entryCount; i++, e++)
        if (e->allocatable)
            e->unk_16 = true;
    return true;
}

bool RegPool::clearRegisterAssignmentsFirstPass()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->assignedPhysicalRegister = 0;
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        p->assignedPhysicalRegister = 0;
    return true;
}

bool RegPool::clearRegisterAssignmentsSecondPass()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->assignedPhysicalRegister = 0;
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next)
        p->assignedPhysicalRegister = 0;
    return true;
}

bool RegPool::resetPoolLocalAllocationState()
{
    for (RegPoolEntry* p = virtualHead; p; p = p->next) {
        if (!p->clearPartPayloadMarkers())
            goto fail;
        p->assignedPhysicalRegister = 0;
        p->assignmentCostTableSize = -1;
    }
    {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++) {
            if (!e->clearPartPayloadMarkers())
                goto fail;
            e->assignedPhysicalRegister = 0;
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
    RegPoolEntry* e = createVirtualRegister(buf);
    if (!e) {
        reportError("failed to add a new virtual register!\n");
        exitAfterCleanup(-1);
    }
    if (e->moveInstruction) {
        reportError("move inst already set in getMoveReg\n");
        exitAfterCleanup(-1);
    }
    e->moveInstruction = move;
    return e;
}

bool RegPool::ownsRegisterClass(RegPoolEntry* e)
{
    return e && e->registerClass == registerClass;
}

bool RegPool::optReset()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->optReset();
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->optReset();
    }
    return true;
}

bool RegPool::emitReset()
{
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            e->usedSlotMask = 0;
        for (RegPoolEntry* p = virtualHead; p; p = p->next)
            p->usedSlotMask = 0;
    }
    return true;
}

char* RegPool::emitStats(char* buf)
{
    buf += formatIntoBuffer(buf, "; %3d %s\n", countUsed(), poolName);
    return buf;
}

int RegPool::regIndex(int reg)
{
    int r = -1;
    if (registerEntries) {
        reg -= registerEntries->registerIndex;
        if (reg >= 0 && reg < entryCount)
            r = reg;
    }
    return r;
}

int RegPool::countUsed()
{
    int n = 0;
    if (registerEntries) {
        RegPoolEntry* e = registerEntries;
        for (int i = 0; i != entryCount; i++, e++)
            if (e->usedSlotMask)
                n++;
    }
    return n;
}

int RegPool::countMarkedPhysicalRegisters()
{
    int n = 0;
    if (registerEntries) {
        for (int i = 0; i != entryCount; i++)
            if (registerEntries[i].unk_16)
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
        ol_s* c = &s->currentLocation;
        if (c->unk_12)
            out[n++] = c;
        int j;
        ol_s* p;
        for (j = 0, p = s->pendingLocations; j != 4; j++, p++)
            if (p->unk_12)
                out[n++] = p;
    }
    return n;
}

int RegPool::gatherInflight(ol_s** out)
{
    int n = 0;
    for (int i = 0; i != entryCount; i++) {
        RegPoolEntry* e = &registerEntries[i];
        int r = e->gatherInflight(out + n);
        if (r < 0)
            return -1;
        n += r;
    }
    for (RegPoolEntry* p = virtualHead; p; p = p->next) {
        int r = p->gatherInflight(out + n);
        if (r < 0)
            return -1;
        n += r;
    }
    return n;
}
