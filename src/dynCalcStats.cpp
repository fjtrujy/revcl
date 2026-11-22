#include "os.h"
#include "lock_shared.h"

static inline void resetInstrDyn(Instr *instruction)
{
    instruction->dynamicUpperBound = 30000;
    instruction->dynamicEarliestStart = -1;
    instruction->dynamicReverseStart = 0;
    instruction->dynamicReverseEnd = 0;
    for (int i = 0; i != instruction->operandCount; i++) {
        RegPoolEntry *registerEntry = instruction->operands[i].assignedRegister;
        if (registerEntry)
            registerEntry->resetRegisterPartTiming();
    }
}

void Instr::resetDynamicSchedulingStats()
{
    dynamicUpperBound = 30000;
    dynamicEarliestStart = -1;
    dynamicReverseStart = 0;
    dynamicReverseEnd = 0;
    for (int i = 0; i != operandCount; i++) {
        RegPoolEntry *registerEntry = operands[i].assignedRegister;
        if (registerEntry)
            registerEntry->resetRegisterPartTiming();
    }
}

bool resetBlockDynamicStats(Block *block)
{
    block->maximumDynamicStartRow = -1;
    block->unk_06 = -1;
    for (Instr *instruction = block->completedInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction)
        resetInstrDyn(instruction);
    for (Instr *instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction)
        resetInstrDyn(instruction);
    return true;
}


static inline bool isAAInputLock(OperandLock *operandLock)
{
    return operandLock->operandStatusCode == 2;
}

static inline int lockLatency(OperandLock *input, OperandLock *parent)
{
    return input->issueLatency + parent->issueLatency;
}

bool dynCalcStats(workTable *table)
{
    Block *block = (Block *)table;
    int i;
    Instr *instruction;
    resetBlockDynamicStats(block);

    for (instruction = block->completedInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        int v = instruction->position.linearRow;
        instruction->dynamicReverseStart = v;
        instruction->dynamicEarliestStart = v;
        if (v > block->maximumDynamicStartRow)
            block->maximumDynamicStartRow = v;
    }

    for (instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        Operand *operand;
        int h = 0;
        for (i = 0; i != instruction->operandCount; i++) {
            operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *slot = &operand->parts[p];
                    bool bit = (operand->componentMask >> p) & 1;
                    if (bit && !isAAInputLock(slot)) {
                        OperandLock *par = slot->getParent();
                        int lat = lockLatency(slot, par);
                        int t = 0;
                        if (!isRegLock(par))
                            t = par->ownerInstr()->dynamicEarliestStart + lat;
                        if (t > h)
                            h = t;
                    }
                }
            }
        }
        for (i = 0; i != instruction->operandCount; i++) {
            operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 2);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    rPart *registerPart = reg->getRP(p);
                    if (instruction->inputMaskForRegister(reg, 1 << p) && registerPart->latestPendingReadCycle != -1) {
                        int v = registerPart->latestPendingReadCycle;
                        if (operand->externalBlockModeFlag)
                            v = v - 1;
                        else
                            v = v - operand->outputLatency;
                        if (h <= v)
                            h = v + 1;
                    }
                }
            }
        }
        instruction->dynamicEarliestStart = h;
        if (h > block->maximumDynamicStartRow)
            block->maximumDynamicStartRow = h;
        for (i = 0; i != instruction->operandCount; i++) {
            operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    bool bit = (operand->componentMask >> p) & 1;
                    if (bit) {
                        OpSlot *slot = &operand->parts[p];
                        rPart *registerPart = reg->getRP(p);
                        if (!isAAInputLock(slot)) {
                            if (registerPart->earliestPendingReadCycle == -1) {
                                registerPart->earliestPendingReadCycle = instruction->dynamicEarliestStart;
                                registerPart->latestPendingReadCycle = instruction->dynamicEarliestStart;
                            } else {
                                if (registerPart->earliestPendingReadCycle > instruction->dynamicEarliestStart)
                                    registerPart->earliestPendingReadCycle = instruction->dynamicEarliestStart;
                                if (registerPart->latestPendingReadCycle < instruction->dynamicEarliestStart)
                                    registerPart->latestPendingReadCycle = instruction->dynamicEarliestStart;
                            }
                        }
                    }
                }
            }
        }
        for (i = 0; i != instruction->operandCount; i++) {
            operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 2);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    bool bit = (operand->componentMask >> p) & 1;
                    if (bit) {
                        rPart *registerPart = reg->getRP(p);
                        registerPart->earliestPendingReadCycle = -1;
                        registerPart->latestPendingReadCycle = -1;
                    }
                }
            }
        }
    }

    for (instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction)
        instruction->dynamicReverseStart = block->maximumDynamicStartRow;

    for (instruction = block->pendingInstructions.tail; instruction; instruction = instruction->pendingLink.previousInstruction) {
        for (i = 0; i != instruction->operandCount; i++) {
            Operand *operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *slot = &operand->parts[p];
                    bool bit = (operand->componentMask >> p) & 1;
                    if (bit && !isAAInputLock(slot)) {
                        OperandLock *par = slot->getParent();
                        if (!isRegLock(par)) {
                            Instr *pi = par->ownerInstr();
                            int lat = lockLatency(slot, par);
                            int v = instruction->dynamicEarliestStart - lat;
                            if (v < 0) {
                                reportError("-ve ip?\n");
                                instruction->dumpOperandDiagnostics(false);
                                table->dump();
                                instruction->dumpOperandDiagnostics(false);
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

    for (instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        for (i = 0; i != instruction->operandCount; i++) {
            Operand *operand = &instruction->operands[i];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int p = 0; p != reg->partCount; p++) {
                    OpSlot *slot = &operand->parts[p];
                    bool bit = (operand->componentMask >> p) & 1;
                    if (bit && !isAAInputLock(slot)) {
                        OperandLock *par = slot->getParent();
                        if (!isRegLock(par)) {
                            Instr *pi = par->ownerInstr();
                            int lat;
                            if (slot->latencyModeFlag)
                                lat = 1;
                            else
                                lat = par->issueLatency;
                            int v = pi->dynamicReverseStart + lat;
                            if (v < instruction->dynamicUpperBound)
                                instruction->dynamicUpperBound = v;
                            v = instruction->dynamicEarliestStart - lat;
                            if (v > pi->dynamicReverseEnd)
                                pi->dynamicReverseEnd = v;
                        }
                    }
                }
            }
        }
    }

    for (instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        instruction->schedulingStatistics.forwardLatencyDepth = instruction->dynamicEarliestStart;
        instruction->schedulingStatistics.reverseLatencyDepth = block->maximumDynamicStartRow - instruction->dynamicReverseStart;
    }
    return true;
}

void dynDumpStats(Block *block)
{
    int saved = errorOccurred;
    errorOccurred = !quietDiagnostics;
    reportInfo("dynDumpStats - start\n");
    int n = 0;
    for (Instr *instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        const char *mark = "    ";
        int s = instruction->dynamicEarliestStart;
        int e = instruction->dynamicUpperBound;
        int dep = block->loopSize;
        if (s - e > dep)
            mark = "****";
        int ht = block->maximumDynamicStartRow - instruction->schedulingStatistics.reverseLatencyDepth;
        dep = instruction->schedulingStatistics.forwardLatencyDepth;
        reportInfo("[%3d]:%3d [%5d %5d %5d %5d ]%s dep=%2d ht=%2d  %s\n",
                      n, block->loopSize, e, s, instruction->dynamicReverseStart, instruction->dynamicReverseEnd, mark,
                      dep, ht, instruction->opcodeInfo->mnemonic);
        n++;
    }
    reportInfo("dyDuumpStats - end\n");
    errorOccurred = saved;
}
