#include "RegPool.h"
#include "os.h"
#include "workTable.h"

RegPool *GetRP(unsigned t);

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
    if (n <= 0)
        return 0;
    for (ru = head; ru; ru = ru->unk_04) {
        if (ru->unk_08->unk_24 == regClass)
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
            func_0804ad38("thruSum was set - but - oBits wasn't set for a reg part..register %s, part %d\n",
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
