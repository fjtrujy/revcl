// olList / ol_s operand-location lists, CIE group 5.
#pragma implementation "ol_s.h"
#include "Instr.h"
#include "ol_s.h"
#include "os.h"

inline void ol_s::reset()
{
    unk_13 = 0;
    unk_04 = 0;
    unk_10 = -1;
    unk_12 = 0;
    unk_00 = 0;
    unk_11 = 0;
    unk_0c = 0;
    unk_08 = 0;
}

inline bool ol_s::isExt()
{
    return unk_13 & 1;
}

inline ol_s::ol_s()
{
    reset();
}

void olList::func_08057e9c()
{
    int n = 0;
    ol_s *p = unk_00;
    if (unk_04 != 0) {
        for (; p; p = p->unk_00) {
            if (p->unk_04) {
                p->unk_04->func_08054168();
                p->unk_04->func_08054008(false);
            }
            n++;
        }
    } else if (p) {
        func_0804ad38("opLocList - had zero count, but non empty head\n");
        func_0804aeb0();
    }
    if (n != unk_04) {
        func_0804ad38("oploc list - bad list not nent\n");
        func_0804aeb0();
    }
}

void olList::func_08057f2c()
{
    int n = 0;
    ol_s *p = unk_00;
    if (unk_04 != 0) {
        for (; p; p = p->unk_00) {
            if (p->unk_04) {
                p->unk_04->func_08054168();
                p->unk_04->leDump(p->unk_10, 1 << p->unk_11);
            }
            n++;
        }
    } else if (p) {
        func_0804ae34("opLocList - had zero count, but non empty head\n");
    }
    if (n != unk_04)
        func_0804ae34("oploc list - bad list not nent\n");
}

void ol_s::func_08057fbc()
{
    if (unk_04) {
        unk_04->func_08054168();
        unk_04->func_08054008(false);
    }
}

void ol_s::func_08057fec()
{
    if (unk_04) {
        unk_04->func_08054168();
        unk_04->leDump(unk_10, 1 << unk_11);
    }
}

Operand *func_0805802c(ol_s *ol)
{
    if (ol->unk_04)
        return &ol->unk_04->unk_38[ol->unk_10];
    return 0;
}

static inline ol_s *allocOL()
{
    ol_s *p = olFreeList;
    if (!p) {
        p = new ol_s;
    } else {
        olFreeList = p->unk_00;
        p->reset();
    }
    return p;
}

ol_s *olList::func_080580fc(Instr *inst, int op, int slot, bool ext)
{
    ol_s *prev = 0;
    ol_s *p;
    for (p = unk_00; p; p = p->unk_00) {
        if (inst == p->unk_04 && op == p->unk_10 && slot == p->unk_11) {
            if (ext != p->isExt()) {
                func_0804ad38("addOL: inst external state mismatch bExt=%d\n", ext);
                p->func_08057fbc();
                func_0804aeb0();
            }
            func_0804ae34("matched oploc\n");
            return p;
        }
        prev = p;
    }
    p = allocOL();
    p->init(inst, op, slot, ext);
    if (prev)
        prev->unk_00 = p;
    else
        unk_00 = p;
    unk_04++;
    return p;
}
