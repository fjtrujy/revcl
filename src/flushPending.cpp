#include "os.h"
#include "RegPool_shared.h"

bool rPart::flushPending()
{
    if (unk_3e) {
        reportError("rPart::flushPending - issueLock was set (%d)\n", unk_3e);
        return false;
    }
    for (int i = 0; i != 4; i++)
        unk_4c[i].reset();
    unk_38 = 0;
    unk_24.reset();
    return true;
}

ol_s* rPart::func_0805a164()
{
    for (int i = 0; i != 4; i++) {
        if (!unk_4c[i].unk_12) {
            unk_4c[i].reset();
            return &unk_4c[i];
        }
    }
    return 0;
}

bool rPart::cycle(bool flag)
{
    if (flag) {
        unk_3e = 0;
        if (unk_38 && unk_38 != &unk_24) {
            unk_24.func_08058054(unk_38);
            unk_38 = &unk_24;
        }
        for (int i = 0; i != 4; i++)
            unk_4c[i].reset();
        ol_s* cur = &unk_24;
        if (cur->unk_12)
            cur->unk_12 = 0;
    } else {
        int n = 0;
        ol_s* cur = &unk_24;
        int c = cur->unk_12;
        if (c) {
            int v = c - 1;
            cur->unk_12 = v > 255 ? 255 : v;
            n = 1;
        }
        for (int i = 0; i != 4; i++) {
            ol_s* p = &unk_4c[i];
            int c = p->unk_12;
            if (!c)
                continue;
            int v = c - 1;
            p->unk_12 = v > 255 ? 255 : v;
            if (v == 0) {
                if (++n > 1) {
                    reportError("WARNING !! CYCLE wrote thru more than once!\n");
                    return false;
                }
                cur->func_08058054(p);
                unk_3c = 0;
            }
        }
    }
    if (++unk_3c == 0x7fff)
        unk_3c = 0x7fff;
    if (unk_3e)
        unk_3e--;
    return true;
}

void RegPoolEntry::func_0805a300()
{
    usedSlotMask = 0;
    throughputWriter = 0;
    unk_36 = 0;
    for (int i = 0; i != 4; i++)
        unk_44[i].func_0805a3cc();
}

void RegPoolEntry::func_0805a340()
{
}

void RegPoolEntry::func_0805a348()
{
    for (int i = 0; i != 4; i++) {
        rPart* p = &unk_44[i];
        p->unk_40 = -1;
        p->unk_42 = -1;
        p->unk_44 = -1;
    }
}

void rPart::func_0805a374()
{
    unk_40 = -1;
    unk_42 = -1;
    unk_44 = -1;
}

void RegPoolEntry::func_0805a390()
{
    throughputWriter = 0;
    unk_36 = 0;
    for (int i = 0; i != 4; i++)
        unk_44[i].func_0805a3cc();
}

void rPart::func_0805a3cc()
{
    unk_00[3] = 0;
    unk_00[2] = 0;
    unk_00[0] = 0;
    unk_00[1] = 0;
    unk_3e = 0;
    unk_24.reset();
    unk_38 = 0;
    unk_3c = 0;
    unk_00[7] = -1;
    unk_00[4] = -1;
    unk_00[8] = -1;
    unk_00[5] = -1;
    unk_00[6] = -1;
    unk_00[12] = -1;
    unk_00[9] = -1;
    unk_00[13] = -1;
    unk_00[10] = -1;
    unk_00[14] = -1;
    unk_00[11] = -1;
    unk_00[16] = -1;
    unk_00[15] = -1;
    for (int i = 0; i != 4; i++)
        unk_4c[i].reset();
}

void RegPoolEntry::func_0805a4b4()
{
    for (int i = 0; i != 4; i++)
        unk_44[i].unk_48 = 0;
}

void rPart::func_0805a4d4()
{
    unk_48 = 0;
}

void rPart::func_0805a4e4()
{
    unk_48 = 0;
}

bool RegPoolEntry::func_0805a4f4(bool flag)
{
    for (int i = 0; i != partCount; i++)
        if (!unk_44[i].cycle(flag))
            return false;
    return true;
}

RegPoolEntry::RegPoolEntry()
{
    unk_0c = 0;
    next = 0;
    registerClass = 0;
    partCount = 0;
    allocatable = false;
    unk_20 = 0;
    registerIndex = 0;
    isVirtual = false;
    ownerPool = 0;
    registerWeb = 0;
    unk_10 = 0;
    unk_16 = false;
    usedSlotMask = 0;
    unk_1c = 0;
    unk_2c = false;
    assignedPhysicalRegister = 0;
}

rPart::rPart()
{
    func_0805a3cc();
    unk_48 = 0;
}

mNode* RegPoolEntry::func_0805a8a4(const char* name)
{
    mNode* n = unk_2b8.findByExactName(name);
    if (n)
        return n;
    return unk_2b8.appendNamedNode(name);
}

const char* RegPoolEntry::registerName()
{
    const char* name = unk_2b8.firstNodeName();
    if (!name)
        name = "UNNAMED_REGISTER!";
    return name;
}

VclPtrWord RegPoolEntry::func_0805a908(int i)
{
    return unk_44[i].unk_48;
}

bool RegPoolEntry::func_0805a924(int i, VclPtrWord v)
{
    unk_44[i].unk_48 = v;
    return true;
}

VclPtrWord rPart::func_0805a948()
{
    return unk_48;
}

bool rPart::func_0805a954(VclPtrWord v)
{
    unk_48 = v;
    return true;
}

ol_s* rPart::func_0805a968(int i)
{
    return &unk_4c[i];
}

void RegPoolEntry::moveReset()
{
    if (!unk_20) {
        reportError("rg..movereset called - but not move\n");
        exitAfterCleanup(-1);
    }
    unk_3c = -1;
    allocatable = false;
    registerWeb = 0;
    unk_10 = 0;
    for (int i = 0; i != partCount; i++)
        unk_44[i].unk_48 = 0;
}

bool RegPoolEntry::optReset()
{
    registerWeb = 0;
    return true;
}

bool RegPoolEntry::func_0805aa00()
{
    for (int i = 0; i != partCount; i++) {
        if (unk_44[i].unk_48)
            ((char*)unk_44[i].unk_48)[3] = 0;
    }
    return true;
}

char regNameBuf[1024];
int regNameBufPos;

char* RegPoolEntry::maskName(int mask)
{
    if (regNameBufPos > 512)
        regNameBufPos = 0;
    char* buf = &regNameBuf[regNameBufPos];
    const char* name = unk_2b8.firstNodeName();
    if (!name)
        name = "UNNAMED_REGISTER!";
    char* p = buf + formatIntoBuffer(buf, "%s", name);
    if (partCount > 1)
        p += formatIntoBuffer(p, "%s", func_080520d4(mask));
    *p = 0;
    regNameBufPos = p - regNameBuf + 1;
    return buf;
}

int RegPoolEntry::regIndex()
{
    if (!ownerPool)
        reportError("no reg pool set for register\n");
    return ownerPool->regIndex(registerIndex);
}
