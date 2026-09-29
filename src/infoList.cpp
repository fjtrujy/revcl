// iHolder and infoList, CIE group 3.
#pragma implementation
#include "infoList.h"
#include "Instr.h"
#include "os.h"

inline iHolder::iHolder()
{
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
}

void iHolder::func_080523b8()
{
    if (unk_00)
        unk_00->leDump(unk_04, unk_08);
    else
        func_0804ac50(" UNSET_INST\n");
}

bool iHolder::catInit(ol_s *ol)
{
    if (!ol) {
        func_0804ad38("iHolder - passed a null ol\n");
        return false;
    }
    if (!unk_00) {
        if (ol->unk_04) {
            unk_00 = ol->unk_04;
            unk_04 = ol->unk_10;
            unk_08 = 0;
        } else {
            func_0804ad38("iHolder:: init called, but ol_s not inst\n");
            return false;
        }
    } else if (unk_00 != ol->unk_04 || unk_04 != ol->unk_10) {
        func_0804ad38("iHolder::catInit - mismatch adding to an existing object\n");
        return false;
    }
    unk_08 |= 1 << ol->unk_11;
    return true;
}

infoList::infoList()
{
    unk_04 = 0;
    unk_08 = 0;
}

infoList::~infoList()
{
    infoNode *p = unk_04;
    while (p) {
        infoNode *n = p->unk_04;
        delete p;
        p = n;
    }
    unk_04 = 0;
    unk_08 = 0;
}

bool infoList::clear()
{
    infoNode *p = unk_04;
    while (p) {
        infoNode *n = p->unk_04;
        delete p;
        p = n;
    }
    unk_04 = 0;
    unk_08 = 0;
    return true;
}

void infoList::func_080525c8()
{
    for (infoNode *p = unk_04; p; p = p->unk_04)
        p->dump();
}

bool infoList::addStall(ol_s *a, ol_s *b, int x, int val)
{
    stallNode *p = (stallNode *)unk_04;
    if (!a || !b) {
        func_0804ad38("infoList:: addStall - null input ptr\n");
        return false;
    }
    if (val <= 0) {
        func_0804ad38("infoList:: add stall - val <=0\n");
        return false;
    }
    for (; p; p = (stallNode *)p->unk_04)
        if ((char)p->unk_0c.func_08052380(a) && (char)p->unk_18.func_08052380(b))
            break;
    if (!p) {
        p = new stallNode;
        if (!p) {
            func_0804ad38("infoList::addStall - failed to get new stall\n");
            return false;
        }
        p->unk_04 = unk_04;
        unk_08++;
        unk_04 = p;
    }
    if (!p->catStall(a, b, x, val)) {
        func_0804ad38("infoList::addStall - failed to cat a stall\n");
        return false;
    }
    return true;
}
