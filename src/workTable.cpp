extern const char thruSumOobError[] = "thruSum was set - but - oBits wasn't set for a reg part..register %s, part %d\n";
#define VCL_REGUSESLOT_IMPL
#include "RegPool.h"
#include "ol_s.h"
#include "os.h"
#include "workTable.h"

RegPool *GetRP(unsigned t);

regUseSlot::~regUseSlot()
{
}

void regUseList::func_0805b44c()
{
    if (func_0805b0cc()) {
        for (int i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp)
                func_0805b494(i);
        }
    }
}

extern const char regUseNewFailedError[] = "AddNewRegRef() - new failed\n";

int regUseList::func_0805b494(int regClass)
{
    int n = 0;
    regUse *head = unk_04;
    regUse *ru;
    for (ru = head; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (!reg) {
            func_0804ad38("found reg ref with no reg!\n");
            func_0804adf0(-1);
            return 0;
        }
        if (reg->unk_24 == regClass)
            n++;
    }
    if (n == 0)
        return 0;
    for (ru = head; ru; ru = ru->unk_04) {
        if (ru->unk_08->isClass(regClass))
            ru->func_0805b51c();
    }
    return n;
}

void regUse::func_0805b51c()
{
    RegPoolEntry *reg = unk_08;
    int i;
    int j = 0;
    if (reg->unk_28 != 0)
        do
            j++;
        while (j != reg->unk_28);
    for (i = 0; i != reg->unk_28; i++) {
        if (unk_20[i].unk_34[0] && !((unk_0f >> i) & 1)) {
            func_0804ad38(thruSumOobError,
                          reg->func_0805a8e0(), i);
            func_0804adf0(-1);
        }
    }
    for (i = 0; i != reg->unk_28; i++) {
        if (unk_20[i].unk_10.count)
            unk_20[i].unk_10.dump();
    }
}

regUse *regUseList::func_0805b5dc(RegPoolEntry *reg)
{
    return find(reg);
}

regUse *regUseList::firstReal()
{
    regUse *p;
    for (p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        if (!reg) {
            func_0804ad38("regref has no register!\n");
            func_0804adf0(-1);
        }
        if (!reg->unk_14)
            break;
    }
    return p;
}

regUse *regUse::nextReal()
{
    regUse *p;
    for (p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        if (!reg) {
            func_0804ad38("regref has no register!\n");
            func_0804adf0(-1);
        }
        if (!reg->unk_14)
            break;
    }
    return p;
}

regUse *regUseList::firstVirtual()
{
    regUse *p;
    for (p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        if (!reg) {
            func_0804ad38("regref has no register!\n");
            func_0804adf0(-1);
        }
        if (reg->unk_14)
            break;
    }
    return p;
}

regUse *regUse::nextVirtual()
{
    regUse *p;
    for (p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        if (!reg) {
            func_0804ad38("regref has no register!\n");
            func_0804adf0(-1);
        }
        if (reg->unk_14)
            break;
    }
    return p;
}

bool regUseList::optReset()
{
    for (regUse *p = unk_04; p; p = p->unk_04) {
        for (int i = 0; i != 4; i++) {
            p->unk_20[i].unk_58 = 0;
            p->unk_20[i].unk_5c.func_0805bcc4();
        }
    }
    return true;
}

bool regUse::optReset()
{
    for (int i = 0; i != 4; i++) {
        unk_20[i].unk_58 = 0;
        unk_20[i].unk_5c.func_0805bcc4();
    }
    return true;
}

bool regUseSlot::optReset()
{
    unk_58 = 0;
    unk_5c.func_0805bcc4();
    return true;
}

bool regUseSlot::addOutput(ol_s *ol)
{
    if (!unk_08.func_08058484(ol)) {
        func_0804ad38("failed to add oploc to output list sort\n");
        return false;
    }
    return true;
}

void regUseSlot::func_0805b814()
{
    unk_08.func_08057e9c();
}

void regUseSlot::func_0805b824()
{
    unk_00.func_08057e9c();
}

cls0805bcc4 *regUseSlot::func_0805b830()
{
    return &unk_5c;
}

regUse *regUse::Clone()
{
    RegPoolEntry *reg = unk_08;
    if (!reg) {
        func_0804ad38("regref::Clone - input had no register\n");
        return 0;
    }
    regUse *n = new regUse(reg);
    if (!n) {
        func_0804ad38("failed to get new regref in clone\n");
        return 0;
    }
    n->unk_18 = unk_18;
    n->unk_12 = unk_12;
    n->unk_1c = unk_1c;
    n->unk_13 = unk_13;
    n->unk_14 = unk_14;
    n->unk_0c = unk_0c;
    n->unk_08 = unk_08;
    n->unk_0d = unk_0d;
    n->unk_0e = unk_0e;
    n->unk_0f = unk_0f;
    n->unk_10 = unk_10;
    n->unk_16 = unk_16;
    n->unk_11 = unk_11;
    n->unk_15 = unk_15;
    for (int i = 0; i != reg->unk_28; i++) {
        n->unk_20[i].unk_34[0] = unk_20[i].unk_34[0];
        n->unk_20[i].unk_34[1] = unk_20[i].unk_34[1];
        n->unk_20[i].unk_34[3] = unk_20[i].unk_34[3];
        n->unk_20[i].unk_34[2] = unk_20[i].unk_34[2];
        n->unk_20[i].unk_3c[0] = unk_20[i].unk_3c[0];
        n->unk_20[i].unk_3c[1] = unk_20[i].unk_3c[1];
        n->unk_20[i].unk_3c[2] = unk_20[i].unk_3c[2];
        n->unk_20[i].unk_3c[3] = unk_20[i].unk_3c[3];
        n->unk_20[i].unk_3c[4] = unk_20[i].unk_3c[4];
        n->unk_20[i].unk_3c[5] = unk_20[i].unk_3c[5];
        n->unk_20[i].unk_3c[6] = unk_20[i].unk_3c[6];
        n->unk_20[i].unk_3c[7] = unk_20[i].unk_3c[7];
        n->unk_20[i].unk_3c[8] = unk_20[i].unk_3c[8];
        n->unk_20[i].unk_3c[9] = unk_20[i].unk_3c[9];
        n->unk_20[i].unk_3c[10] = unk_20[i].unk_3c[10];
        n->unk_20[i].unk_3c[11] = unk_20[i].unk_3c[11];
        n->unk_20[i].unk_3c[12] = unk_20[i].unk_3c[12];
        n->unk_20[i].unk_30 = unk_20[i].unk_30;
        n->unk_20[i].unk_32 = unk_20[i].unk_32;
        n->unk_20[i].unk_58 = 0;
        n->unk_20[i].unk_5c.func_0805bcc4();
    }
    return n;
}

const char *regUse::regName()
{
    if (!unk_08)
        return "REGREG_NOREG";
    return unk_08->func_0805a8e0();
}

ol_s *regUseSlot::func_0805ba2c(int pos, bool all)
{
    if (all)
        pos = 0xff;
    ol_s *prev = 0;
    for (ol_s *p = unk_08.unk_00; p; p = p->unk_00) {
        if (pos < p->unk_12)
            break;
        prev = p;
    }
    return prev;
}

int regUseList::func_0805ba64(int regClass)
{
    int n = 0;
    for (regUse *p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        if (reg->unk_24 != regClass || reg == reg->unk_04->unk_10)
            continue;
        if (~p->unk_0e & p->unk_10)
            n++;
        else if (!p->unk_0f)
            n++;
    }
    return n;
}

void regUseList::func_0805bab0()
{
    if (func_0805b0cc()) {
        for (int i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp && rp->unk_18) {
                func_0804acb0("%s\n", rp->unk_2c);
                func_0805bb10(i);
            }
        }
    }
}

static inline bool isVirtualRegister(RegPoolEntry *reg)
{
    if (reg->unk_14 == true)
        return true;
    return false;
}

void regUseList::func_0805bb10(int regClass)
{
    regUse *p;
    for (p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        RegPoolEntry *phys = reg->unk_18;
        const char *alloc = "UNALLOCATED";
        if (phys)
            alloc = phys->func_0805a8e0();
        if (!isVirtualRegister(reg))
            alloc = reg->func_0805a8e0();
        int mask = p->unk_0e | p->unk_0f | p->unk_0c | p->unk_0d | p->unk_10;
        if (reg->unk_24 == regClass && reg != reg->unk_04->unk_10 && p->isExternal()) {
            const char *thr = "";
            if (p->unk_10 & ~(p->unk_0e | p->unk_0f | p->unk_0c | p->unk_0d))
                thr = "-THR";
            func_0804acb0(" EXTERNAL: %20s - %s %s\n", reg->maskName(mask), alloc, thr);
        }
    }
    for (p = unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        RegPoolEntry *phys = reg->unk_18;
        const char *alloc = "UNALLOCATED";
        if (phys)
            alloc = phys->func_0805a8e0();
        if (!isVirtualRegister(reg))
            alloc = reg->func_0805a8e0();
        int mask = p->unk_0e | p->unk_0f | p->unk_0c | p->unk_0d | p->unk_10;
        if (reg->unk_24 == regClass && reg != reg->unk_04->unk_10 && !p->isExternal())
            func_0804acb0(" LOCAL : %20s - %s\n", reg->maskName(mask), alloc);
    }
}
