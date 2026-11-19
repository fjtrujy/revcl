#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "flowBlock.h"

Instr *addXinFor(instStream *s, RegPoolEntry *r);
Instr *addXoutFor(instStream *s, RegPoolEntry *r);
bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag);
RegPoolEntry *getTagRegister(RegPool *pool, char *name);
int getPoolsByClassID(int cls, RegPool **tab, int max);
RegPool *GetRP(unsigned t);

static inline bool addNewInputInl(Instr *in, RegPoolEntry *r, int bits)
{
    if (!addNewOperand(in, r, bits, 0x37, 1, false)) {
        func_0804ad38("add new input failed\n");
        return false;
    }
    return true;
}

static inline bool addNewOutputInl(Instr *in, RegPoolEntry *r, int bits, bool f)
{
    if (!addNewOperand(in, r, bits, 0x37, 2, f)) {
        func_0804ad38("add new output failed\n");
        return false;
    }
    return true;
}


bool bindDependencies(Program *p)
{
    RegPool *pools[8];
    instStream *s;
    int n = getPoolsByClassID(3, pools, 8);
    if (n < 0) {
        func_0804ad38("bindDependencies() - Processor failed to enumerate hardware register classes\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            int bits = in->getRamBits();
            if (!in->unk_139c)
                continue;
            if (!in->unk_13ce) {
                func_0804ad84("tagString found for non memory instruction\n");
                return false;
            }
            if (bits) {
                char *tag = in->unk_139c;
                RegPool *hw = GetRP(10);
                if (!hw) {
                    func_0804ad38("failed to get a hw pool\n");
                    return false;
                }
                if (hw->unk_0e != 3) {
                    func_0804ad38("umm - not a hardware register\n");
                    return false;
                }
                while (*tag) {
                    while (*tag == ':')
                        tag++;
                    char *end = tag;
                    while (*end && *end != ':')
                        end++;
                    if (*end)
                        *end++ = 0;
                    RegPoolEntry *r = getTagRegister(hw, tag);
                    if (!r) {
                        func_0804ad38("failed to create a tag register\n");
                        return false;
                    }
                    switch ((unsigned char)in->unk_13ce) {
                    case 1:
                        if (!addNewInputInl(in, r, bits)) {
                            func_0804ad38("failed to add input operand for load\n");
                            return false;
                        }
                        break;
                    case 2:
                        if (!addNewOutputInl(in, r, bits, true)) {
                            func_0804ad38("failed to add output operand for store\n");
                            return false;
                        }
                        break;
                    default:
                        func_0804ad38("unknown ramdir!\n");
                        return false;
                    }
                    tag = end;
                }
            }
            delete in->unk_139c;
            in->unk_139c = 0;
        }
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        bool xin = s->unk_2cc == 2;
        bool xout = s->unk_2cd == 3;
        if (xin) {
            if (!addXinFor(s, func_0806c224(0x39))) {
                func_0804ad38("failed to add the fpu sticky flags as xin\n");
                return false;
            }
            if (!addXinFor(s, func_0806c224(0x3a))) {
                func_0804ad38("failed to add the div sticky flags as xin\n");
                return false;
            }
        }
        if (xin || xout) {
            for (int i = 0; i != n; i++) {
                RegPool *rp = pools[i];
                RegPoolEntry *ext = rp->unk_14;
                if (!ext) {
                    func_0804ad38("failed to get EXT for pool %s\n", rp->unk_2c);
                    return false;
                }
                if (xin && !addXinFor(s, ext)) {
                    const char *ownerName = s->func_0806d674();
                    func_0804ad38("failed to add external input to %s\n", ownerName);
                    return false;
                }
                if (xout && !addXoutFor(s, ext)) {
                    func_0804ad38("failed to add external output to %s\n", s->func_0806d674());
                    return false;
                }
                for (RegPoolEntry *e = rp->unk_1c; e; e = e->next) {
                    if (xin && !addXinFor(s, e)) {
                        const char *ownerName = s->func_0806d674();
                        func_0804ad38("failed to add external input to %s\n", ownerName);
                        return false;
                    }
                    // the original calls addXinFor here too
                    if (xout && !addXinFor(s, e)) {
                        func_0804ad38("failed to add external output to %s\n", s->func_0806d674());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

Instr *addXinFor(instStream *s, RegPoolEntry *r)
{
    iInfo *info = func_0806bf74(0xbe);
    if (!info) {
        func_0804ad38("addXinFor - cant get xin_hw\n");
        return 0;
    }
    Instr *in = new Instr(info);
    if (!in) {
        func_0804ad38("failed to make xin instruction\n");
        return 0;
    }
    if (in->unk_138b != 1) {
        func_0804ad38("xin doesn't have just one operand\n");
        delete in;
        return 0;
    }
    int serial = g_080b33a8++;
    in->unk_13e0 = serial;
    Operand *op = &in->unk_38[0];
    if (op->unk_04 != 0x37) {
        func_0804ad38("xin wasn't external\n");
        delete in;
        return 0;
    }
    op->unk_18 = r;
    op->unk_07 = (1 << r->unk_28) - 1;
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("can't get work table\n");
        delete in;
        return 0;
    }
    if (!wt->logAddLinear_Tail(in)) {
        func_0804ad38("failed to an external input to the tail of list\n");
        return 0;
    }
    return in;
}

Instr *addXoutFor(instStream *s, RegPoolEntry *r)
{
    iInfo *info = func_0806bf74(0xbf);
    if (!info) {
        func_0804ad38("addxoutFor - can't get xout_hw\n");
        return 0;
    }
    Instr *in = new Instr(info);
    if (!in) {
        func_0804ad38("failed to make xout instruction\n");
        return 0;
    }
    if (in->unk_138b != 1) {
        func_0804ad38("xout doesn't have just one operand\n");
        delete in;
        return 0;
    }
    int serial = g_080b33a8++;
    in->unk_13e0 = serial;
    Operand *op = &in->unk_38[0];
    if (op->unk_04 != 0x37) {
        func_0804ad38("xout wasn't external\n");
        delete in;
        return 0;
    }
    op->unk_18 = r;
    op->unk_07 = (1 << r->unk_28) - 1;
    workTable *wt = s->getTable();
    if (!wt) {
        func_0804ad38("can't get work table\n");
        delete in;
        return 0;
    }
    if (!wt->logAddLinear_Head(in)) {
        func_0804ad38("failed to an external output to the head of list\n");
        return 0;
    }
    return in;
}

bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag)
{
    if (!r) {
        func_0804ad38("add new output - passed a null rg pointer\n");
        return false;
    }
    if (!bits) {
        func_0804ad38("passed null mod bits for new output register\n");
        return false;
    }
    if (bits & (-1 << r->unk_28)) {
        func_0804ad38("add new output - out of bounds mod bits requested\n");
        return false;
    }
    iInfo *info = in->unk_34;
    if (!info) {
        func_0804ad38("instruction has no m_Inst\n");
        return false;
    }
    unsigned char n = in->unk_138b;
    int i;
    for (i = 0; i < n; i++) {
        Operand *op = &in->unk_38[i];
        if (op->unk_08 && op->unk_09 == dir && r == op->unk_18) {
            op->unk_07 |= bits;
            return true;
        }
    }
    if (i == 12) {
        func_0804ad38("ran out of operand space for add new output\n");
        return false;
    }
    Operand *op = &in->unk_38[n];
    op->unk_26 = n;
    op->unk_0b = 1;
    op->unk_09 = dir;
    op->unk_08 = 1;
    op->unk_05 = 0;
    op->unk_0d = 0;
    op->unk_0a = flag;
    op->unk_29 = 0;
    op->unk_18 = r;
    op->unk_07 = bits;
    op->unk_04 = kind;
    in->unk_138b++;
    return in->func_08053fac();
}

bool addNewInput(Instr *in, RegPoolEntry *r, int bits)
{
    if (!addNewOperand(in, r, bits, 0x37, 1, false)) {
        func_0804ad38("add new input failed\n");
        return false;
    }
    return true;
}

bool addNewOutput(Instr *in, RegPoolEntry *r, int bits, int flag)
{
    if (!addNewOperand(in, r, bits, 0x37, 2, flag)) {
        func_0804ad38("add new output failed\n");
        return false;
    }
    return true;
}

RegPoolEntry *getTagRegister(RegPool *pool, char *name)
{
    if (RegPool::isResvd(name, pool->unk_0c)) {
        func_0804ad84("can't use %s as a name for one of the %s\n", name, pool->unk_2c);
        return 0;
    }
    RegPoolEntry *r = pool->func_08059a1c(name);
    if (!r) {
        r = pool->func_08059a5c(name);
        if (!r) {
            func_0804ad38("failed to create a virtual reg\n");
            return 0;
        }
    }
    r->unk_14 = false;
    r->unk_28 = 4;
    return r;
}
