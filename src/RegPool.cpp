#include "RegPool.h"
#include "os.h"

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
        int n = unk_08;
        for (int i = 0; i != n; i++)
            ;
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
