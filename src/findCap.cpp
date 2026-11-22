#include "sred_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int func_0804bf50(unsigned int x);
RegPoolEntry *getZeroReg(unsigned int type);


static inline bool isOut(Operand *o) { return o->unk_09 == 2; }
static inline RegPoolEntry *outR(Operand *o)
{
    if (isOut(o))
        return o->unk_18;
    return 0;
}
static inline bool isPoolZero(RegPool *p, RegPoolEntry *r) { return r == p->zeroRegister; }

int findCap(workTable *wt, Instr *in, int opIdx, RegPoolEntry **capReg, int *capMask)
{
    g_080de73c = wt;
    g_080de744 = 0;
    *capReg = 0;
    Operand *op = &in->operands[opIdx];
    int cap = -1;
    g_080de734 = 0;
    g_080de748 = 0;
    *capMask = 0;
    g_080de738 = 0;
    g_080de740 = 0;

    RegPoolEntry *reg = isRegisterOperand(op, 2) ? op->unk_18 : 0;
    switch (reg->registerIndex) {
    case 52:
        cap = 0x73;
        g_080de744 = func_0806c224(51);
        g_080de748 = 1;
        break;
    case 50:
        g_080de744 = reg;
        g_080de748 = 1;
        cap = 0x74;
        break;
    case 55: case 57: case 59: {
        if (in->unk_13e8.getPipe())
            break;
        RegPoolEntry *r0 = outR(&in->operands[0]);
        if (!r0) {
            const char *name = in->opcodeInfo ? in->opcodeInfo->mnemonic : "NO_BASE_INST";
            reportError("couldn't get r0 for %s\n", name);
            return -1;
        }
        if (r0->registerClass != 9)
            break;
        RegPool *pool = r0->ownerPool;
        if (isPoolZero(pool, r0)) {
            unsigned char mask[32];
            int i;
            for (i = 0; i != 32; i++) {
                RegPoolEntry *e = pool->findByRegisterIndex(i);
                mask[i] = (e->unk_16 && e->allocatable) ? in->operands[0].unk_07 : 0;
            }
            mask[0] = 0;
            for (regUse *ru = wt->unk_14.head; ru; ru = ru->unk_04) {
                RegPoolEntry *reg = ru->unk_08;
                if (reg->registerClass != 9)
                    continue;
                if (reg->isVirtual == 1) {
                    reportError("register isn't base!\n");
                    return -1;
                }
                unsigned char m = ru->unk_0e | ru->unk_0f | ru->unk_10;
                int idx = pool->regIndex(reg->registerIndex);
                if (idx < 0) {
                    reportError("failed to get local id\n");
                    return -1;
                }
                if (m & mask[idx])
                    mask[idx] = 0;
            }
            int best = 0;
            for (i = 1; i != 32; i++) {
                int m = in->operands[0].unk_07 & mask[i];
                int n = func_0804bf50(m);
                if (n > best) {
                    RegPoolEntry *e = pool->findByRegisterIndex(i);
                    g_080de748 = m;
                    g_080de744 = e;
                    best = n;
                }
            }
        } else {
            g_080de748 = in->operands[0].unk_07;
            g_080de744 = r0;
        }
        if (g_080de744)
            cap = 0x21;
        break;
    }
    }

    if (cap < 0)
        return 0;
    iInfo *ii = func_0806bf74(cap);
    g_080de738 = ii;
    if (!ii && cap != 0) {
        reportError("failed to get II for cap \n");
        return -1;
    }
    if (!ii) {
        reportError("failed to get II for cap \n");
        return -1;
    }
    *capReg = g_080de744;
    g_080de740 = in;
    *capMask = g_080de748;
    g_080de734 = ii->unk_0e_w;
    return g_080de734;
}

bool doCap(int line, int pipe, int mask)
{
    if (!g_080de738) {
        reportError("cap_ii not set\n");
        return false;
    }
    if (!mask) {
        reportError("zero mod bits for doCap\n");
        return false;
    }
    Instr *ci = new Instr(g_080de738);
    if (!ci) {
        reportError("failed to make cap instruction\n");
        return false;
    }
    ci->unk_138f = mask;
    for (int i = 0; i != ci->operandCount; i++)
        ci->operands[i].unk_07 = mask;

    if (g_080de744->registerClass == 9) {
        RegPoolEntry *z = getZeroReg(9);
        ci->operands[0].unk_18 = z;
        ci->operands[0].unk_14 = z;
        ci->operands[0].unk_10 = 0;
        ci->operands[1].unk_18 = g_080de744;
        ci->operands[1].unk_14 = g_080de744;
        ci->operands[1].unk_10 = 0;
        g_080de740->operands[0].unk_14 = g_080de744;
        g_080de740->operands[0].unk_18 = g_080de744;
        g_080de740->operands[0].unk_10 = 0;
        regUse *ru = g_080de73c->unk_14.findByRegister(g_080de744);
        if (!ru) {
            ru = g_080de73c->unk_14.AddNewRegRef(g_080de744);
            if (!ru) {
                reportError("failed to add new regref\n");
                return true;
            }
        }
        ru->unk_0e |= mask;
        ru->unk_0f |= mask;
    } else {
        ci->operands[0].unk_18 = g_080de744;
        ci->operands[0].unk_14 = g_080de744;
    }
    ci->unk_13e8.func_0805874c(line);
    ci->unk_13e8.func_0805878c(pipe);
    if (!g_080de73c->putIn(ci)) {
        reportError("failed to put instruction into itable\n");
        return false;
    }
    g_080de738 = 0;
    return true;
}
