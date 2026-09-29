// iHolder and infoList, CIE group 3.
#pragma implementation
#include "infoList.h"
#include "Instr.h"
#include "instStream.h"
#include "RegPool.h"
#include "workTable.h"
#include "ubsList.h"
#include "os.h"

extern char *stallNames_080b3244[3];

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

inline void iHolder::dump()
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

void stallNode::dump()
{
    char buf[512];
    char *p = buf;
    p += func_0804adbc(p, "STALL");
    int max = 0;
    for (int i = 0; i != 3; i++) {
        unsigned short v = unk_24[i];
        if (v > max)
            max = v;
        if (v)
            p += func_0804adbc(p, "_%s(%d)", stallNames_080b3244[i], v);
    }
    p += func_0804adbc(p, " maxVal %d ", max);
    if (unk_08)
        p += func_0804adbc(p, ": local");
    else
        p += func_0804adbc(p, " inter-stream");
    *p = 0;
    func_0804acb0("%s\n", buf);
    func_0804acb0("    setter is : ");
    unk_0c.dump();
    func_0804acb0("    user is   : ");
    unk_18.dump();
}

bool stallNode::catStall(ol_s *a, ol_s *b, int type, int val)
{
    if (!a || !b) {
        func_0804ac7c("stallNode - init called with a null input\n");
        return false;
    }
    if (!unk_0c.catInit(a)) {
        func_0804ad38("stallInit - failed to add in a source loc\n");
        return false;
    }
    Instr *ia = a->unk_04;
    Instr *ib = b->unk_04;
    if (!ia || !ib) {
        func_0804ad38("can't determine - at least one of instructions in stall pair\n");
        return false;
    }
    instStream *sa = ia->func_08054168();
    instStream *sb = ib->func_08054168();
    sa->unk_260 = 1;
    sb->unk_260 = 1;
    if (!sa || !sb) {
        func_0804ad38("can't determine owner ist for one of inputs to stall\n");
        return false;
    }
    unk_08 = sa->unk_2c0 == sb->unk_2c0;
    if (!unk_18.catInit(b)) {
        func_0804ad38("stallInit - failed to add in a user loc\n");
        return false;
    }
    if (type < 0 || type > 2) {
        func_0804ad38("bad stall type\n");
        return false;
    }
    if (val > unk_24[type])
        unk_24[type] = val;
    return true;
}

ubsNode::ubsNode()
{
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
    unk_14 = 0;
}

void ubsNode::dump()
{
    struct ubsEntry {
        Instr *inst;
        int opnd;
        int mask;
    } list[64];
    int n, opnd;
    func_0804acb0("Used before set : ");
    n = 0;
    workTable *wt = unk_14->getWorkTable();
    regUse *ru;
    if (!wt || !(ru = wt->unk_14.func_0805b5dc(unk_08))) {
        func_0804acb0("register %s is used before set at \n",
                      unk_08->func_0805a8e0(), opnd, unk_14->func_0806d674());
        return;
    }
    func_0804acb0("register %s is used before set at instruction(s) :\n",
                  unk_08->func_0805a8e0());
    for (int i = 0; i != unk_08->unk_28; i++) {
        if ((unk_0c >> i) & 1) {
            for (ol_s *p = ru->unk_20[i].unk_00; p; p = p->unk_00) {
                Instr *inst = p->unk_04;
                int bit = 1 << p->unk_11;
                opnd = p->unk_10;
                int j = 0;
                ubsEntry *e = list;
                for (; j < n; j++, e++)
                    if (e->inst == inst && e->opnd == opnd)
                        break;
                if (j == n) {
                    e = 0;
                    if (n < 64) {
                        e = &list[n];
                        e->mask = 0;
                        n++;
                    }
                }
                e->inst = inst;
                e->mask |= bit;
                e->opnd = opnd;
            }
        }
    }
    for (int k = 0; k != n; k++) {
        func_0804acb0("   ");
        list[k].inst->leDump(list[k].opnd, list[k].mask);
    }
}

bool ubsNode::catInit(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits)
{
    if (!entry) {
        func_0804ad38("catInit - entry point not set\n");
        return false;
    }
    if (!use) {
        func_0804ad38("catInit - use point not set\n");
        return false;
    }
    if (!reg) {
        func_0804ad38("ubs::catInit - register not set\n");
        return false;
    }
    if (!bits) {
        func_0804ad38("ubs::catInit - bits was null in cat init\n");
        return false;
    }
    unk_10 = entry;
    unk_14 = use;
    unk_08 = reg;
    unk_0c |= bits;
    return true;
}

bool ubsList::addUBS(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits)
{
    ubsNode *p = (ubsNode *)unk_04;
    if (!entry || !use || !reg || !bits) {
        func_0804ad38("ubsList: addUBS - passed bad args\n");
        return false;
    }
    for (; p; p = (ubsNode *)p->unk_04)
        if (reg == p->unk_08 && entry == p->unk_10 && use == p->unk_14)
            break;
    if (!p) {
        p = new ubsNode;
        if (!p) {
            func_0804ad38("infoList::addStall - failed to get new ubs\n");
            return false;
        }
        unk_08++;
        p->unk_04 = unk_04;
        unk_04 = p;
    }
    if (!p->catInit(entry, use, reg, bits)) {
        func_0804ad38("failed to cat init a used-before-set operand\n");
        return false;
    }
    return true;
}

bool Instr::func_08052f84()
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (!op->getReg())
            continue;
        op->unk_29 = 0;
        if (!(unk_1388 & 4))
            continue;
        for (ol_s *ol = op->unk_2c[0].unk_54.unk_00; ol; ol = ol->unk_00) {
            Instr *in = ol->unk_04;
            bool found = false;
            if (in->unk_13ce && !ol->unk_10)
                continue;
            for (int j = 0; j != in->unk_138b; j++) {
                RegPoolEntry *r = in->unk_38[j].getReg();
                if (r && (unsigned)(r->unk_30 - 54) <= 5) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                op->unk_29 = 1;
                break;
            }
        }
    }
    return true;
}

int Instr::func_08053104()
{
    int target;
    if (!unk_34)
        return 0;
    int id = func_0806c72c(unk_34);
    target = -1;
    if (id < 0) {
        func_0804ad38("vpu: fixup branch - iUID<0\n");
        return -1;
    }
    if (!(unk_1388 & 4))
        return 1;
    if (unk_1388 & 8)
        return 1;
    if (unk_138b == 2) {
        RegPoolEntry *r = unk_38[0].unk_18;
        if (!r)
            return -1;
        if (r->unk_30 == 0x20) {
            if (id == 0xa8)
                return 0;
            if (id == 0xa9)
                target = 1;
            if (id == 0xaa)
                return 0;
            if (id == 0xab)
                target = 1;
        }
    }
    if (unk_138b == 3 && unk_38[0].unk_18 == unk_38[1].unk_18) {
        if (id == 0xac)
            target = 2;
        if (id == 0xad)
            return 0;
    }
    if (target <= 0)
        return 1;
    for (int i = 0; i != unk_138b; i++) {
        unk_38[i].unk_18 = 0;
        unk_38[i].unk_04 = 0;
    }
    unk_138b = 1;
    unk_38[0].unk_04 = 0x1b;
    if (!unk_38[0].unk_1c.set(&unk_38[target].unk_1c)) {
        func_0804ad38("fixup branch -set new ximm\n");
        return -1;
    }
    unk_38[target].unk_1c.clear();
    unk_34 = func_0806bf8c("b", true);
    if (!func_08053fac()) {
        func_0804ad38("failed to convert a branch\n");
        return -1;
    }
    unk_1388 = 1;
    return 2;
}

int Instr::func_080532d8()
{
    int r = 0;
    switch (func_08055ac4()) {
    case -1:
        r = -1;
        break;
    case 0x22:
    case 0x6c:
        if (unk_38[1].isZero() && !unk_38[2].isZero()) {
            RegPoolEntry *t = unk_38[1].unk_18;
            unk_38[1].unk_18 = unk_38[2].unk_18;
            unk_38[2].unk_18 = t;
            unk_38[1].unk_10 = unk_38[2].unk_10;
            unk_38[2].unk_10 = 0;
        }
        break;
    case 0x30:
    case 0x6f: {
        Operand *op = &unk_38[1];
        if (op->unk_18 == unk_38[2].unk_18) {
            RegPoolEntry *z = func_0806c25c(op->unk_04);
            if (!z) {
                func_0804ad38("failed to get a zero register");
                return 0;
            }
            for (int i = 1; i != 3; i++) {
                unk_38[i].unk_18 = z;
                r = 1;
            }
        }
        break;
    }
    }
    return r;
}
