// iHolder and infoList, CIE group 3.
#include "infoList.h"
#include "Instr.h"
#include "instStream.h"
#include "RegPool.h"
#include "workTable.h"
#include "ubsList.h"
#include "os.h"

char *stallNames_080b3244[3] = { "LATENCY", "THRUPUT", "RFWRITE" };

inline iHolder::iHolder()
{
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
}

static inline int iHolderMatches(iHolder *h, ol_s *ol)
{
    if (!ol || !h->unk_00)
        return 0;
    int r = 0;
    Instr *i = ol->unk_04;
    if (h->unk_00 == i) {
        int idx = ol->unk_10;
        if (h->unk_04 == idx)
            r = 1;
    }
    return r;
}

int iHolder::func_08052380(ol_s *ol)
{
    return iHolderMatches(this, ol);
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
        Instr *in = ol->unk_04;
        if (!in) {
            func_0804ad38("iHolder:: init called, but ol_s not inst\n");
            return false;
        }
        unk_00 = in;
        unk_04 = ol->unk_10;
        unk_08 = 0;
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

inline infoNode::infoNode(InitTag)
{
    unk_04 = 0;
}

inline stallNode::stallNode(InitTag) : infoNode(infoNode::INIT)
{
    for (int i = 0; i != 3; i++)
        unk_24[i] = 0;
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
        if ((char)iHolderMatches(&p->unk_0c, a) && (char)iHolderMatches(&p->unk_18, b))
            break;
    if (!p) {
        p = new stallNode(stallNode::INIT);
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

infoNode::infoNode()
{
    unk_04 = 0;
}

infoNode::~infoNode()
{
}

stallNode::stallNode() : infoNode(infoNode::INIT)
{
    for (int i = 0; i != 3; i++)
        unk_24[i] = 0;
}

stallNode::~stallNode()
{
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

ubsNode::ubsNode() : infoNode(infoNode::INIT)
{
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
    unk_14 = 0;
}

ubsNode::~ubsNode()
{
}

void ubsNode::dump()
{
    struct ubsEntry {
        Instr *inst;
        int opnd;
        int mask;
    } list[64];
    int n;
    func_0804acb0("Used before set : ");
    n = 0;
    instStream *use = unk_14;
    workTable *wt = use->unk_218;
    bool haveTable = wt != 0;
    if (!haveTable) {
        func_0804ad38("work table not set in %s\n", use->func_0806d674());
        func_0804adf0(-1);
    }
    if (!haveTable)
        wt = use->unk_218;
    regUse *ru;
    if (wt && (ru = wt->unk_14.func_0805b5dc(unk_08))) {
        func_0804acb0("register %s is used before set at instruction(s) :\n",
                      unk_08->func_0805a8e0());
        for (int i = 0; i != unk_08->unk_28; i++) {
            if ((unk_0c >> i) & 1) {
                for (ol_s *p = ru->unk_20[i].unk_00.unk_00; p; p = p->unk_00) {
                    Instr *inst = p->unk_04;
                    int bit = p->unk_11;
                    int opnd = p->unk_10;
                    bit = 1 << bit;
                    int j;
                    for (j = 0; j < n &&
                         (list[j].inst != inst || list[j].opnd != opnd); j++)
                        ;
                    ubsEntry *e = 0;
                    if (j == n) {
                        if (n < 64) {
                            e = &list[n];
                            e->mask = 0;
                            n++;
                        }
                    } else
                        e = &list[j];
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
    } else {
        func_0804acb0("register %s is used before set at \n",
                      unk_08->func_0805a8e0(), unk_14->func_0806d674());
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
