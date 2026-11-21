#pragma implementation "wtEntry.h"
#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
// wtRow methods are compiled in this same translation unit to reproduce
// the original constant-pool alignment and linked data layout.
#include "loadStalls_shared.h"

wtRow::wtRow()
{
    unk_0c = 0;
    unk_08 = 0;
    unk_0a = 0;
    unk_09 = 0;
}

wtRow::~wtRow()
{
}

static inline bool clearWtEntry(wtEntry *e) { e->unk_00 = 0; return 1; }

bool wtRow::func_080721f8()
{
    for (int i = 0; i != 2; i++)
        if (!clearWtEntry(&unk_00[i]))
            return 0;
    return 1;
}

bool wtEntry::func_08072218()
{
    unk_00 = 0;
    return 1;
}

void wtRow::func_0807222c(int)
{
    for (int i = 0; i != 2; i++) {
        Instr *in = unk_00[i].unk_00;
        if (!in)
            continue;
        if ((int)in & 1)
            continue;
        in->func_08054008(false);
    }
}

Instr *wtRow::getEntry(int p)
{
    if (p < 0) {
        func_0804ad38("iEntry::GetI() - p<0\n");
        func_0804adf0(-1);
        return 0;
    }
    if (p >= 2) {
        func_0804ad38("iEntry::GetI() - p>=MAX_IPIPES\n");
        func_0804adf0(-1);
        return 0;
    }
    return unk_00[p].unk_00;
}

Instr *GetI(Instr **ie, int p)
{
    if (p < 0) {
        func_0804ad38("iEntry::GetI() - p<0\n");
        func_0804adf0(-1);
        return 0;
    }
    if (p >= 2) {
        func_0804ad38("iEntry::GetI() - p>=MAX_IPIPES\n");
        func_0804adf0(-1);
        return 0;
    }
    Instr *in = ie[p];
    if ((int)in & 1)
        return 0;
    return in;
}

int wtRow::func_08072304()
{
    int n = 0;
    for (int i = 0; i != 2; i++)
        if (unk_00[i].unk_00)
            n++;
    return n;
}

Instr *wtRow::onlyEntry()
{
    Instr *r = 0;
    for (int i = 0; i != 2; i++) {
        if (unk_00[i].unk_00) {
            if (r)
                return 0;
            r = unk_00[i].unk_00;
        }
    }
    return r;
}

bool wtRow::clearStalls()
{
    unk_0a = 0;
    unk_09 = 0;
    return 1;
}

void wtRow::func_08072364()
{
    unk_0a = 0;
    unk_09 = 0;
}

bool wtRow::addStall(int type, int len)
{
    if (len <= 0) {
        func_0804ad38("can't add a zero or -ve length stall\n");
        return 0;
    }
    if (len > 255)
        func_0804ac7c("truncating stall length..\n");
    if (type < 0 || type > 2) {
        func_0804ad38("bad stall type\n");
        return 0;
    }
    unk_0a |= 1 << type;
    if (unk_09 < len)
        unk_09 = len;
    return 1;
}

int wtRow::func_080723ec(RegPoolEntry *reg, unsigned char mask)
{
    int r = 0;
    if (!unk_08) {
        for (int i = 0; i != 2; i++) {
            Instr *in = GetI(&unk_00[0].unk_00, i);
            if (in)
                r |= in->func_080553b4(reg, mask);
        }
    }
    return r;
}

int wtRow::func_08072440(RegPoolEntry *reg, unsigned char mask)
{
    int r = 0;
    if (!unk_08) {
        for (int i = 0; i != 2; i++) {
            Instr *in = GetI(&unk_00[0].unk_00, i);
            if (in)
                r |= in->func_0805544c(reg, mask);
        }
    }
    return r;
}

unsigned int wtRow::func_08072494()
{
    Instr *in = GetI(&unk_00[0].unk_00, 1);
    unsigned int m = 0;
    if (in) {
        for (int i = 0; i != in->unk_138b; i++) {
            if (in->unk_38[i].unk_29) {
                unsigned int a = (unsigned short)in->unk_38[i].unk_18->unk_44[0].unk_3c;
                if (!m || a < m)
                    m = a;
            }
        }
    }
    return m;
}
