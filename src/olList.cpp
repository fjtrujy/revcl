// olList / ol_s operand-location lists, CIE group 5.
#pragma implementation "ol_s.h"
#include "Instr.h"
#include "ol_s.h"
#include "ol_s_inline.h"
#include "os.h"

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

static inline void ol_s_copy_inl(ol_s *dst, ol_s *src)
{
    dst->reset();
    if (src) {
        dst->unk_13 = src->unk_13;
        dst->unk_04 = src->unk_04;
        dst->unk_10 = src->unk_10;
        dst->unk_11 = src->unk_11;
        dst->unk_12 = src->unk_12;
    }
}

void ol_s::func_08058054(ol_s *src)
{
    reset();
    if (src) {
        unk_13 = src->unk_13;
        unk_04 = src->unk_04;
        unk_10 = src->unk_10;
        unk_11 = src->unk_11;
        unk_12 = src->unk_12;
    }
}

static inline void ol_s_init_inl(ol_s *o, Instr *inst, int op, int slot, bool ext)
{
    o->reset();
    if (ext)
        o->unk_13 = 1;
    o->unk_04 = inst;
    o->unk_10 = op;
    o->unk_11 = slot;
}

void ol_s::init(Instr *inst, int op, int slot, bool ext)
{
    reset();
    if (ext)
        unk_13 = 1;
    unk_04 = inst;
    unk_10 = op;
    unk_11 = slot;
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
    ol_s_init_inl(p, inst, op, slot, ext);
    if (prev)
        prev->unk_00 = p;
    else
        unk_00 = p;
    unk_04++;
    return p;
}

ol_s *olList::func_0805826c(Instr *inst)
{
    ol_s *prev = 0;
    ol_s *p;
    for (p = unk_00; p; p = p->unk_00) {
        if (inst == p->unk_04)
            return p;
        prev = p;
    }
    p = allocOL();
    ol_s_init_inl(p, inst, 0, 0, false);
    if (prev)
        prev->unk_00 = p;
    else
        unk_00 = p;
    unk_04++;
    return p;
}

ol_s *olList::func_08058348(ol_s *ol, bool ext)
{
    return func_080580fc(ol->unk_04, ol->unk_10, ol->unk_11, ext);
}

ol_s *olList::func_08058370()
{
    ol_s *p = unk_00;
    if (p && p->isExt()) {
        p = p->unk_00;
        while (p && p->isExt())
            p = p->unk_00;
    }
    return p;
}

ol_s *ol_s::func_080583ac()
{
    ol_s *p = unk_00;
    while (p && p->isExt())
        p = p->unk_00;
    return p;
}

ol_s *olList::func_080583d8()
{
    ol_s *p = unk_00;
    if (p && !p->isExt()) {
        p = p->unk_00;
        while (p && !p->isExt())
            p = p->unk_00;
    }
    return p;
}

ol_s *ol_s::func_08058414()
{
    ol_s *p = unk_00;
    while (p && !p->isExt())
        p = p->unk_00;
    return p;
}

RegPoolEntry *func_08058440(ol_s *ol)
{
    Operand *op = ol->unk_04 ? &ol->unk_04->unk_38[ol->unk_10] : 0;
    if (op)
        return op->unk_18;
    func_0804ae34("no register!\n");
    return 0;
}

ol_s *olList::func_08058484(ol_s *src)
{
    ol_s *prev = 0;
    ol_s *p;
    for (p = unk_00; p; p = p->unk_00) {
        if (src->unk_12 < p->unk_12)
            break;
        prev = p;
    }
    ol_s *n = allocOL();
    ol_s_copy_inl(n, src);
    n->unk_13 |= 1;
    n->unk_00 = p;
    if (prev)
        prev->unk_00 = n;
    else
        unk_00 = n;
    unk_04++;
    return n;
}

void func_0805859c(ol_s *ol)
{
    ol->unk_13 |= 2;
}

int func_080585a8(ol_s *ol)
{
    return ol->unk_04->func_08055548();
}

int func_080585c0(ol_s *ol)
{
    return ol->unk_04->unk_13e0;
}

bool func_080585d4(ol_s *a, ol_s *b)
{
    if (a && b)
        return a->unk_04 == b->unk_04 && a->unk_10_11 == b->unk_10_11;
    return a == b;
}

instStream *func_08058618(ol_s *ol)
{
    return ol->unk_04->func_08054168();
}

int func_0805862c(ol_s *ol)
{
    return ol->unk_04->unk_13e8.unk_00;
}

ol_s *func_08058640()
{
    return allocOL();
}

int func_080586c4()
{
    int n = 0;
    for (ol_s *p = olFreeList; p; p = p->unk_00)
        n++;
    return n;
}

ol_s *olFreeList = 0;
