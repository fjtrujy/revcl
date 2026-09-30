#pragma implementation "wtEntry.h"
#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"

bool loadStalls(Program *p)
{
    infoList *st = &p->proc.unk_74;
    for (stallNode *n = (stallNode *)st->unk_04; n; n = (stallNode *)n->unk_04) {
        Instr *in = n->unk_18.unk_00;
        if (!in) {
            func_0804ad38("no instruction in stall\n");
            return 0;
        }
        cls_unk30 *owner = in->unk_30;
        if (!owner) {
            func_0804ad38("instruction has no owner!\n");
            return 0;
        }
        if (owner->unk_09 != 3) {
            func_0804ad38("bad code mode for stall\n");
            return 0;
        }
        cls08058718 *loc = &in->unk_13e8;
        wtRow *ie = (wtRow *)owner->getIE_ck(loc->unk_04);
        if (!ie) {
            func_0804ad38("couldn't get ie for instruction\n");
            return 0;
        }
        if (ie->unk_00[loc->unk_02].unk_00 != in) {
            func_0804ad38("instruction wasn't where I thought it was\n");
            return 0;
        }
        for (int i = 0; i != 3; i++) {
            if (n->unk_24[i]) {
                if (!ie->addStall(i, n->unk_24[i])) {
                    func_0804ad38("failed to load a stall\n");
                    return 0;
                }
            }
        }
    }
    return 1;
}

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

bool wtRow::func_080721f8()
{
    for (int i = 0; i != 2; i++)
        if (!unk_00[i].func_08072218())
            return 0;
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
