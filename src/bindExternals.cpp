#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"

bool bindExternals(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("no worktable!\n");
            func_0804adf0(-1);
        }
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            for (int i = 0; i != in->unk_138b; i++) {
                Operand *op = &in->unk_38[i];
                RegPoolEntry *reg = op->unk_18;
                if (!reg)
                    continue;
                regUse *ru = wt->unk_14.func_0805b5dc(reg);
                if (!ru) {
                    func_0804ad38("failed to find regref in bind externals\n");
                    return false;
                }
                if (ru->isExternal() || wt->unk_08 == 3) {
                    if (reg->unk_14) {
                        func_0804ad38("found a virtual register on an x operand\n");
                        return false;
                    }
                    if (op->unk_10 && op->unk_10 != reg) {
                        func_0804ad38("operand is forced to base, but not this one\n");
                        return false;
                    }
                    op->unk_10 = reg;
                }
            }
        }
    }
    return true;
}

int addXinFor(instStream *s, RegPoolEntry *r);
int addXoutFor(instStream *s, RegPoolEntry *r);
bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, char kind, int dir, bool flag);
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
                    func_0804ad38("failed to add external input to %s\n", s->func_0806d674());
                    return false;
                }
                if (xout && !addXoutFor(s, ext)) {
                    func_0804ad38("failed to add external output to %s\n", s->func_0806d674());
                    return false;
                }
                for (RegPoolEntry *e = rp->unk_1c; e; e = e->next) {
                    if (xin && !addXinFor(s, e)) {
                        func_0804ad38("failed to add external input to %s\n", s->func_0806d674());
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
