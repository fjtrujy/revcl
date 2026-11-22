#include "os.h"
#include "lock_shared.h"

static inline void resetInstrDyn(Instr *in)
{
    in->dynamicUpperBound = 30000;
    in->dynamicEarliestStart = -1;
    in->dynamicReverseStart = 0;
    in->dynamicReverseEnd = 0;
    for (int i = 0; i != in->operandCount; i++) {
        RegPoolEntry *r = in->operands[i].assignedRegister;
        if (r)
            r->resetRegisterPartTiming();
    }
}

void Instr::resetDynamicSchedulingStats()
{
    dynamicUpperBound = 30000;
    dynamicEarliestStart = -1;
    dynamicReverseStart = 0;
    dynamicReverseEnd = 0;
    for (int i = 0; i != operandCount; i++) {
        RegPoolEntry *r = operands[i].assignedRegister;
        if (r)
            r->resetRegisterPartTiming();
    }
}

bool resetBlockDynamicStats(Block *b)
{
    b->unk_04 = -1;
    b->unk_06 = -1;
    for (Instr *in = b->completedInstructions.head; in; in = in->pendingLink.nextInstruction)
        resetInstrDyn(in);
    for (Instr *in = b->pendingInstructions.head; in; in = in->pendingLink.nextInstruction)
        resetInstrDyn(in);
    return true;
}


static inline bool isAAInputLock(OperandLock *in)
{
    return in->unk_19 == 2;
}

static inline int lockLatency(OperandLock *input, OperandLock *parent)
{
    return input->issueLatency + parent->issueLatency;
}

bool dynCalcStats(workTable *wt)
{
    Block *b = (Block *)wt;
    int i;
    Instr *in;
    resetBlockDynamicStats(b);

    for (in = b->completedInstructions.head; in; in = in->pendingLink.nextInstruction) {
        int v = in->position.linearRow;
        in->dynamicReverseStart = v;
        in->dynamicEarliestStart = v;
        if (v > b->unk_04)
            b->unk_04 = v;
    }

    for (in = b->pendingInstructions.head; in; in = in->pendingLink.nextInstruction) {
        Operand *op;
        int h = 0;
        for (i = 0; i != in->operandCount; i++) {
            op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *s = &op->parts[p];
                    bool bit = (op->componentMask >> p) & 1;
                    if (bit && !isAAInputLock(s)) {
                        OperandLock *par = s->getParent();
                        int lat = lockLatency(s, par);
                        int t = 0;
                        if (!isRegLock(par))
                            t = par->ownerInstr()->dynamicEarliestStart + lat;
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
                            v = v - op->outputLatency;
                        if (h <= v)
                            h = v + 1;
                    }
                }
            }
        }
        in->dynamicEarliestStart = h;
        if (h > b->unk_04)
            b->unk_04 = h;
        for (i = 0; i != in->operandCount; i++) {
            op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    bool bit = (op->componentMask >> p) & 1;
                    if (bit) {
                        OpSlot *s = &op->parts[p];
                        rPart *rp = reg->getRP(p);
                        if (!isAAInputLock(s)) {
                            if (rp->unk_42 == -1) {
                                rp->unk_42 = in->dynamicEarliestStart;
                                rp->unk_44 = in->dynamicEarliestStart;
                            } else {
                                if (rp->unk_42 > in->dynamicEarliestStart)
                                    rp->unk_42 = in->dynamicEarliestStart;
                                if (rp->unk_44 < in->dynamicEarliestStart)
                                    rp->unk_44 = in->dynamicEarliestStart;
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
                    bool bit = (op->componentMask >> p) & 1;
                    if (bit) {
                        rPart *rp = reg->getRP(p);
                        rp->unk_42 = -1;
                        rp->unk_44 = -1;
                    }
                }
            }
        }
    }

    for (in = b->pendingInstructions.head; in; in = in->pendingLink.nextInstruction)
        in->dynamicReverseStart = b->unk_04;

    for (in = b->pendingInstructions.tail; in; in = in->pendingLink.previousInstruction) {
        for (i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *s = &op->parts[p];
                    bool bit = (op->componentMask >> p) & 1;
                    if (bit && !isAAInputLock(s)) {
                        OperandLock *par = s->getParent();
                        if (!isRegLock(par)) {
                            Instr *pi = par->ownerInstr();
                            int lat = lockLatency(s, par);
                            int v = in->dynamicEarliestStart - lat;
                            if (v < 0) {
                                reportError("-ve ip?\n");
                                in->dumpOperandDiagnostics(false);
                                wt->dump();
                                in->dumpOperandDiagnostics(false);
                                exitAfterCleanup(-1);
                            }
                            if (pi->dynamicReverseStart > v)
                                pi->dynamicReverseStart = v;
                        }
                    }
                }
            }
        }
    }

    for (in = b->pendingInstructions.head; in; in = in->pendingLink.nextInstruction) {
        for (i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = registerOperand(op, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *s = &op->parts[p];
                    bool bit = (op->componentMask >> p) & 1;
                    if (bit && !isAAInputLock(s)) {
                        OperandLock *par = s->getParent();
                        if (!isRegLock(par)) {
                            Instr *pi = par->ownerInstr();
                            int lat;
                            if (s->unk_15)
                                lat = 1;
                            else
                                lat = par->issueLatency;
                            int v = pi->dynamicReverseStart + lat;
                            if (v < in->dynamicUpperBound)
                                in->dynamicUpperBound = v;
                            v = in->dynamicEarliestStart - lat;
                            if (v > pi->dynamicReverseEnd)
                                pi->dynamicReverseEnd = v;
                        }
                    }
                }
            }
        }
    }

    for (in = b->pendingInstructions.head; in; in = in->pendingLink.nextInstruction) {
        in->schedulingStatistics.forwardLatencyDepth = in->dynamicEarliestStart;
        in->schedulingStatistics.reverseLatencyDepth = b->unk_04 - in->dynamicReverseStart;
    }
    return true;
}

void dynDumpStats(Block *b)
{
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    reportInfo("dynDumpStats - start\n");
    int n = 0;
    for (Instr *in = b->pendingInstructions.head; in; in = in->pendingLink.nextInstruction) {
        const char *mark = "    ";
        int s = in->dynamicEarliestStart;
        int e = in->dynamicUpperBound;
        int dep = b->loopSize;
        if (s - e > dep)
            mark = "****";
        int ht = b->unk_04 - in->schedulingStatistics.reverseLatencyDepth;
        dep = in->schedulingStatistics.forwardLatencyDepth;
        reportInfo("[%3d]:%3d [%5d %5d %5d %5d ]%s dep=%2d ht=%2d  %s\n",
                      n, b->loopSize, e, s, in->dynamicReverseStart, in->dynamicReverseEnd, mark,
                      dep, ht, in->opcodeInfo->mnemonic);
        n++;
    }
    reportInfo("dyDuumpStats - end\n");
    errorOccurred = saved;
}
