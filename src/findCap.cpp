#include "sred_shared.h"

#include "os.h"
#include "workTable.h"
#include "Instr.h"

bool setRange(int start, int end, int lsz);
bool moveLocks(Instr *i, InstructionPosition *pos);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int latestInsert(workTable *wt, Instr *in, bool *found);
int func_0807e130(workTable *wt, Instr *in, InstructionPosition *pos, bool upwards);
int countSetBits(unsigned int x);
RegPoolEntry *getZeroReg(unsigned int type);


static inline bool isOut(Operand *o) { return o->direction == 2; }
static inline RegPoolEntry *outR(Operand *o)
{
    if (isOut(o))
        return o->assignedRegister;
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

    RegPoolEntry *reg = isRegisterOperand(op, 2) ? op->assignedRegister : 0;
    switch (reg->registerIndex) {
    case 52:
        cap = 0x73;
        g_080de744 = findRegisterByGlobalIndex(51);
        g_080de748 = 1;
        break;
    case 50:
        g_080de744 = reg;
        g_080de748 = 1;
        cap = 0x74;
        break;
    case 55: case 57: case 59: {
        if (in->position.getPipe())
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
                mask[i] = (e->unk_16 && e->allocatable) ? in->operands[0].componentMask : 0;
            }
            mask[0] = 0;
            for (regUse *ru = wt->registerUses.head; ru; ru = ru->next) {
                RegPoolEntry *reg = ru->registerEntry;
                if (reg->registerClass != 9)
                    continue;
                if (reg->isVirtual == 1) {
                    reportError("register isn't base!\n");
                    return -1;
                }
                unsigned char m = ru->inputMask | ru->outputMask | ru->throughputMask;
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
                int m = in->operands[0].componentMask & mask[i];
                int n = countSetBits(m);
                if (n > best) {
                    RegPoolEntry *e = pool->findByRegisterIndex(i);
                    g_080de748 = m;
                    g_080de744 = e;
                    best = n;
                }
            }
        } else {
            g_080de748 = in->operands[0].componentMask;
            g_080de744 = r0;
        }
        if (g_080de744)
            cap = 0x21;
        break;
    }
    }

    if (cap < 0)
        return 0;
    iInfo *ii = getOpcodeInfoById(cap);
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
    ci->destinationMask = mask;
    for (int i = 0; i != ci->operandCount; i++)
        ci->operands[i].componentMask = mask;

    if (g_080de744->registerClass == 9) {
        RegPoolEntry *z = getZeroReg(9);
        ci->operands[0].assignedRegister = z;
        ci->operands[0].originalRegister = z;
        ci->operands[0].forcedBaseRegister = 0;
        ci->operands[1].assignedRegister = g_080de744;
        ci->operands[1].originalRegister = g_080de744;
        ci->operands[1].forcedBaseRegister = 0;
        g_080de740->operands[0].originalRegister = g_080de744;
        g_080de740->operands[0].assignedRegister = g_080de744;
        g_080de740->operands[0].forcedBaseRegister = 0;
        regUse *ru = g_080de73c->registerUses.findByRegister(g_080de744);
        if (!ru) {
            ru = g_080de73c->registerUses.AddNewRegRef(g_080de744);
            if (!ru) {
                reportError("failed to add new regref\n");
                return true;
            }
        }
        ru->inputMask |= mask;
        ru->outputMask |= mask;
    } else {
        ci->operands[0].assignedRegister = g_080de744;
        ci->operands[0].originalRegister = g_080de744;
    }
    ci->position.setFromLinearRow(line);
    ci->position.setInstructionPipe(pipe);
    if (!g_080de73c->putIn(ci)) {
        reportError("failed to put instruction into itable\n");
        return false;
    }
    g_080de738 = 0;
    return true;
}
