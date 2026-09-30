// iTable: work table construction and management (CIE group 24).
#include "workTable.h"
#include "instStream.h"

workTable::workTable()
{
    unk_2c = 0;
    unk_4c = 0;
    unk_24 = 0;
    unk_20 = 0;
    unk_0b = 0;
    unk_10 = 0;
    dispose();
}

workTable::~workTable()
{
    dispose();
}

bool workTable::init(unsigned char mode, int size)
{
    dispose();
    switch (mode) {
    case 2:
    case 3:
    case 5:
        size += 4;
        if (size < 4)
            size = 4;
        unk_2c = new wtRow[size];
        if (!unk_2c)
            return false;
        unk_28 = size;
        break;
    case 1:
    case 4:
        if (size) {
            func_0804ad38("can't ask for a table based itable (%d) for a synth/uta table...like.. perleaze make me an abstract base class and leave me alone!\n", size);
            return false;
        }
        break;
    default:
        func_0804ad38("iTable::Init() -%s for Init\n", codeModeStr(mode));
        return false;
    }
    unk_30 = 0;
    unk_34 = 0;
    unk_38 = 0;
    unk_ac = 0;
    unk_40 = 0;
    unk_ad = 0;
    unk_0c = 0;
    unk_7a = 0;
    unk_09 = mode;
    return true;
}

bool workTable::dispose()
{
    Instr *p = unk_94.unk_00;
    while (p) {
        Instr *next = p->unk_24.unk_00;
        if (p->unk_13e8.unk_04 < 0)
            delete p;
        p = next;
    }
    unk_0b = 0;
    if (!freeLockLists(unk_24)) {
        func_0804ad38("failed to dispose rll list\n");
        func_0804aeb0();
    }
    unk_24 = 0;
    unk_20 = 0;
    unk_14.clear();
    delete[] unk_2c;
    unk_2c = 0;
    unk_28 = 0;
    unk_38 = 0;
    unk_30 = 0;
    unk_34 = 0;
    unk_3c = 0;
    unk_00 = -1;
    unk_09 = 0;
    unk_08 = 0;
    unk_0a = 0;
    unk_c8 = 0;
    unk_bc = 0;
    unk_b0 = 0;
    unk_b4 = 0;
    unk_c0 = 0;
    unk_b8 = 0;
    unk_c4 = 0;
    unk_4c = 0;
    unk_88.reset();
    unk_94.reset();
    unk_a0.reset();
    unk_50 = 0;
    unk_40 = 0;
    unk_0c = -1;
    unk_10 = 0;
    return true;
}

void workTable::dump()
{
    switch (unk_09) {
    case 0:
        break;
    case 1:
    case 4:
        for (Instr *p = unk_88.unk_00; p; p = p->unk_1c.unk_00)
            p->func_08054008(false);
        break;
    case 3:
    case 5:
        for (int i = unk_30; i < unk_34; i++) {
            wtRow *r = rowAt(i);
            if (!r)
                func_0804aeb0();
            r->func_0807222c(i);
        }
        break;
    case 2: {
        int lsz = unk_38;
        if (!unk_2c) {
            func_0804ad38("code mode mid - but no table!\n");
            func_0804aeb0();
        }
        if (!lsz && unk_34 < 0) {
            wtRow *r = unk_2c;
            for (int n = unk_28 - 1; n >= 0; n--, r++)
                if (r->unk_00[0].unk_00 || r->unk_00[1].unk_00)
                    break;
        }
        for (int i = unk_30; i < unk_34; i++) {
            wtRow *r = rowAt(i);
            if (!r)
                func_0804aeb0();
            r->func_0807222c(i);
        }
        for (Instr *p = unk_94.unk_00; p; p = p->unk_24.unk_00)
            p->func_08054008(false);
        break;
    }
    default:
        func_0804ad38("unknown code mode for Dump()\n");
        func_0804aeb0();
    }
    unk_14.func_0805b44c();
}

const char *workTable::ownerName()
{
    if (!unk_10)
        return "UNOWNED_TABLE";
    return unk_10->func_0806d674();
}

bool workTable::reserveEntry(int key, int row, int slot)
{
    bool even = !(key & 1);
    if (even) {
        func_0804ad38("can't reserve an entry with even key!\n");
        func_0804aeb0();
        return false;
    }
    Instr **ie = ((cls_unk30 *)this)->getIE_ck(row);
    if (!ie) {
        func_0804ad38("failed to reserve instruction!\n");
        return false;
    }
    if (ie[slot]) {
        func_0804ad38("can't reserve instruction - something already there!\n");
        func_0804aeb0();
    }
    ie[slot] = (Instr *)key;
    return true;
}

int workTable::func_08072eec()
{
    int len = -1;
    switch (unk_09) {
    case 3:
        len = unk_0c;
        break;
    case 1:
        func_0804ad38("called getlogical len on uta\n");
        func_0804aeb0();
        break;
    case 4:
        func_0804ac7c("called getlogical on synth stream\n");
        len = 0;
        break;
    default:
        func_0804ad38("unknown code mode for logical len\n");
        func_0804aeb0();
    }
    return len;
}

int workTable::getLogicalLen()
{
    int len = 0;
    switch (unk_09) {
    case 1:
    case 3:
        len = unk_78;
        break;
    case 4:
        break;
    default:
        func_0804ad38("unknown code mode for logical len\n");
        func_0804aeb0();
    }
    return len;
}

int workTable::getPhysicalLen()
{
    int len = -1;
    switch (unk_09) {
    case 3:
        if (!unk_2c) {
            func_0804ad38("no table!\n");
            func_0804aeb0();
        }
        if (unk_34 < unk_30) {
            func_0804ad38("end before start\n");
            func_0804aeb0();
        }
        len = unk_34 - unk_30;
        break;
    case 1:
        func_0804ad38("called getphysical len on uta\n");
        func_0804aeb0();
        break;
    case 4:
        func_0804ac7c("called getlogical on synth stream\n");
        len = 0;
        break;
    default:
        func_0804ad38("unknown code mode for logical len\n");
        func_0804aeb0();
    }
    if (len < 0) {
        func_0804ad38("getPhysicalLen() - would return 0 - halting\n");
        func_0804aeb0();
    }
    return len;
}

Instr *workTable::func_08073060()
{
    Instr *b = getBranch();
    if (b && ((b->unk_1388 >> 2) & 1))
        return b;
    return 0;
}

Instr *workTable::getBranch()
{
    Instr *b = 0;
    switch (unk_09) {
    case 3:
        if (!unk_2c) {
            func_0804ac7c("called getBranch - but no table\n");
            return 0;
        }
        if (unk_34 > unk_30) {
            int r = unk_34 - 2;
            if (r < unk_30)
                return 0;
            b = rowAt(r)->getEntry(1);
        }
        break;
    case 1:
        b = unk_88.unk_04;
        break;
    case 4:
        break;
    default: {
        const char *owner = unk_10 ? unk_10->func_0806d674() : "UNOWNED_TABLE";
        func_0804ad38("bad code mode for getBranch %s for %s\n", codeModeStr(unk_09), owner);
        func_0804aeb0();
    }
    }
    if (!unk_10) {
        func_0804ac7c("no owner set for getBranch\n");
        func_0804aeb0();
    } else if (b) {
            if (b->unk_1388) {
                if (b->unk_1392)
                    return 0;
                else if (!unk_10->unk_2ce) {
                    func_0804ad38("iTable::getBranch() for %s table has a branch at the end, but owning stream doesn't think it branches\n",
                                  unk_10->func_0806d674());
                    dump();
                    func_0804aeb0();
                }
            } else if (unk_10->unk_2ce) {
                func_0804ad38("iTable::getBranch() - owner thinks it has a branch, but the instruction doesn't seem to be one\n");
                func_0804aeb0();
            }
        } else if (unk_10->unk_2ce) {
            func_0804ad38("iTable::getBranch - owning stream thinks it has a branch - but there's no instruction here!\n");
            func_0804aeb0();
        }
    return (b && b->unk_1388) ? b : 0;
}

class Block;
bool UnlinkFromLinear(Block *blk, Instr *n);
bool func_08073bb0(Block *blk, Instr *in);

inline bool workTable::logUnlinkFromLinear(Instr *in)
{
    if (unk_94.unk_08) {
        func_0804ad38("iTable::logUnlinkFromLinear - hmm - we might be being optimised? todo=%d\n", unk_94.unk_08);
        return 0;
    }
    if (!UnlinkFromLinear((Block *)this, in)) {
        func_0804ad38("logUnlinkFromLinear: failed to unlink instruction from linear\n");
        func_0804aeb0();
    }
    in->unk_30 = 0;
    if (!func_08073bb0((Block *)this, in)) {
        func_0804ad38("failed to unlog instruction\n");
        return 0;
    }
    return 1;
}

bool workTable::removeBranch()
{
    Instr *b = getBranch();
    if (!b) {
        func_0804ad38("removeBranch - couldn't find a branch instruction\n");
        return 0;
    }
    if (!logUnlinkFromLinear(b)) {
        func_0804ad38("failed to take out the branch instruction\n");
        return 0;
    }
    delete b;
    return 1;
}

int workTable::getCondLid()
{
    int lid = -1;
    Instr *b = getBranch();
    Instr *c = (b && ((b->unk_1388 >> 2) & 1)) ? b : 0;
    if (c)
        lid = c->unk_13e8.unk_03;
    if (unk_38) {
        if (lid < 0)
            func_0804ad38("getCondLid : no terminating conditional!\n");
    } else if (lid > 0) {
        func_0804ad38("getCondLid : terminating conditional had lid>0\n");
    }
    return lid;
}

bool workTable::takeOut(Instr *i)
{
    cls08058718 *p = &i->unk_13e8;
    wtRow *row = &unk_2c[p->unk_04];
    row->unk_00[p->unk_02].unk_00 = (Instr *)i->unk_1398;
    i->clearSlot();
    return 1;
}

bool workTable::putIn(Instr *i)
{
    cls08058718 *p = &i->unk_13e8;
    wtRow *row = &unk_2c[p->unk_04];
    row->unk_00[p->unk_02].unk_00 = i;
    return 1;
}
