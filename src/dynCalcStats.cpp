#include "os.h"
#include "lock_shared.h"

static inline void resetInstrDyn(Instr *in)
{
    in->unk_1408 = 30000;
    in->unk_140a = -1;
    in->unk_140c = 0;
    in->unk_140e = 0;
    for (int i = 0; i != in->operandCount; i++) {
        RegPoolEntry *r = in->operands[i].unk_18;
        if (r)
            r->func_0805a348();
    }
}

void Instr::func_0805f4e0()
{
    unk_1408 = 30000;
    unk_140a = -1;
    unk_140c = 0;
    unk_140e = 0;
    for (int i = 0; i != operandCount; i++) {
        RegPoolEntry *r = operands[i].unk_18;
        if (r)
            r->func_0805a348();
    }
}

bool func_0805f550(Block *b)
{
    b->unk_04 = -1;
    b->unk_06 = -1;
    for (Instr *in = b->unk_a0.head; in; in = in->unk_24.unk_00)
        resetInstrDyn(in);
    for (Instr *in = b->unk_94.head; in; in = in->unk_24.unk_00)
        resetInstrDyn(in);
    return true;
}


static inline bool isAAInputLock(OperandLock *in)
{
    return in->unk_19 == 2;
}

static inline int lockLatency(OperandLock *input, OperandLock *parent)
{
    return input->unk_18 + parent->unk_18;
}

bool dynCalcStats(workTable *wt)
{
    Block *b = (Block *)wt;
    int i;
    Instr *in;
    func_0805f550(b);

    for (in = b->unk_a0.head; in; in = in->unk_24.unk_00) {
        int v = in->unk_13e8.unk_00;
        in->unk_140c = v;
        in->unk_140a = v;
        if (v > b->unk_04)
            b->unk_04 = v;
    }

    for (in = b->unk_94.head; in; in = in->unk_24.unk_00) {
        Operand *op;
        int h = 0;
        for (i = 0; i != in->operandCount; i++) {
            op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit && !isAAInputLock(s)) {
                        OperandLock *par = s->getParent();
                        int lat = lockLatency(s, par);
                        int t = 0;
                        if (!isRegLock(par))
                            t = par->ownerInstr()->unk_140a + lat;
                        if (t > h)
                            h = t;
                    }
                }
            }
        }
        for (i = 0; i != in->operandCount; i++) {
            op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 2);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    rPart *rp = reg->getRP(p);
                    if (in->inputMaskForRegister(reg, 1 << p) && rp->unk_44 != -1) {
                        int v = rp->unk_44;
                        if (op->unk_0b)
                            v = v - 1;
                        else
                            v = v - op->unk_0a;
                        if (h <= v)
                            h = v + 1;
                    }
                }
            }
        }
        in->unk_140a = h;
        if (h > b->unk_04)
            b->unk_04 = h;
        for (i = 0; i != in->operandCount; i++) {
            op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit) {
                        OpSlot *s = &op->unk_2c[p];
                        rPart *rp = reg->getRP(p);
                        if (!isAAInputLock(s)) {
                            if (rp->unk_42 == -1) {
                                rp->unk_42 = in->unk_140a;
                                rp->unk_44 = in->unk_140a;
                            } else {
                                if (rp->unk_42 > in->unk_140a)
                                    rp->unk_42 = in->unk_140a;
                                if (rp->unk_44 < in->unk_140a)
                                    rp->unk_44 = in->unk_140a;
                            }
                        }
                    }
                }
            }
        }
        for (i = 0; i != in->operandCount; i++) {
            op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 2);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit) {
                        rPart *rp = reg->getRP(p);
                        rp->unk_42 = -1;
                        rp->unk_44 = -1;
                    }
                }
            }
        }
    }

    for (in = b->unk_94.head; in; in = in->unk_24.unk_00)
        in->unk_140c = b->unk_04;

    for (in = b->unk_94.tail; in; in = in->unk_24.unk_04) {
        for (i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit && !isAAInputLock(s)) {
                        OperandLock *par = s->getParent();
                        if (!isRegLock(par)) {
                            Instr *pi = par->ownerInstr();
                            int lat = lockLatency(s, par);
                            int v = in->unk_140a - lat;
                            if (v < 0) {
                                reportError("-ve ip?\n");
                                in->func_08054008(false);
                                wt->dump();
                                in->func_08054008(false);
                                exitAfterCleanup(-1);
                            }
                            if (pi->unk_140c > v)
                                pi->unk_140c = v;
                        }
                    }
                }
            }
        }
    }

    for (in = b->unk_94.head; in; in = in->unk_24.unk_00) {
        for (i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *s = &op->unk_2c[p];
                    bool bit = (op->unk_07 >> p) & 1;
                    if (bit && !isAAInputLock(s)) {
                        OperandLock *par = s->getParent();
                        if (!isRegLock(par)) {
                            Instr *pi = par->ownerInstr();
                            int lat;
                            if (s->unk_15)
                                lat = 1;
                            else
                                lat = par->unk_18;
                            int v = pi->unk_140c + lat;
                            if (v < in->unk_1408)
                                in->unk_1408 = v;
                            v = in->unk_140a - lat;
                            if (v > pi->unk_140e)
                                pi->unk_140e = v;
                        }
                    }
                }
            }
        }
    }

    for (in = b->unk_94.head; in; in = in->unk_24.unk_00) {
        in->unk_13f0.unk_04 = in->unk_140a;
        in->unk_13f0.unk_00 = b->unk_04 - in->unk_140c;
    }
    return true;
}

void dynDumpStats(Block *b)
{
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    reportInfo("dynDumpStats - start\n");
    int n = 0;
    for (Instr *in = b->unk_94.head; in; in = in->unk_24.unk_00) {
        const char *mark = "    ";
        int s = in->unk_140a;
        int e = in->unk_1408;
        int dep = b->unk_38;
        if (s - e > dep)
            mark = "****";
        int ht = b->unk_04 - in->unk_13f0.unk_00;
        dep = in->unk_13f0.unk_04;
        reportInfo("[%3d]:%3d [%5d %5d %5d %5d ]%s dep=%2d ht=%2d  %s\n",
                      n, b->unk_38, e, s, in->unk_140c, in->unk_140e, mark,
                      dep, ht, in->opcodeInfo->mnemonic);
        n++;
    }
    reportInfo("dyDuumpStats - end\n");
    errorOccurred = saved;
}
