#include "os.h"
#include "lock_shared.h"

static inline void resetInstrDyn(Instr *instruction)
{
    instruction->dynamicUpperBound = 30000;
    instruction->dynamicEarliestStart = -1;
    instruction->dynamicReverseStart = 0;
    instruction->dynamicReverseEnd = 0;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        RegPoolEntry *registerEntry = instruction->operands[operandIndex].assignedRegister;
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
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        RegPoolEntry *registerEntry = operands[operandIndex].assignedRegister;
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
    int operandIndex;
    Instr *instruction;
    resetBlockDynamicStats(block);

    for (instruction = block->completedInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        int scheduledRow = instruction->position.linearRow;
        instruction->dynamicReverseStart = scheduledRow;
        instruction->dynamicEarliestStart = scheduledRow;
        if (scheduledRow > block->maximumDynamicStartRow)
            block->maximumDynamicStartRow = scheduledRow;
    }

    for (instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        Operand *operand;
        int maximumReadyCycle = 0;
        for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            operand = &instruction->operands[operandIndex];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                    OpSlot *slot = &operand->parts[partIndex];
                    bool bit = (operand->componentMask >> partIndex) & 1;
                    if (bit && !isAAInputLock(slot)) {
                        OperandLock *par = slot->getParent();
                        int lat = lockLatency(slot, par);
                        int readyCycle = 0;
                        if (!isRegLock(par))
                            readyCycle = par->ownerInstr()->dynamicEarliestStart + lat;
                        if (readyCycle > maximumReadyCycle)
                            maximumReadyCycle = readyCycle;
                    }
                }
            }
        }
        for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            operand = &instruction->operands[operandIndex];
            RegPoolEntry *reg = registerOperand(operand, 2);
            if (reg) {
                for (int registerPartIndex = 0; registerPartIndex != reg->partCount; registerPartIndex++) {
                    rPart *registerPart = reg->getRP(registerPartIndex);
                    if (instruction->inputMaskForRegister(reg, 1 << registerPartIndex) && registerPart->latestPendingReadCycle != -1) {
                        int pendingReadCycle = registerPart->latestPendingReadCycle;
                        if (operand->externalBlockModeFlag)
                            pendingReadCycle = pendingReadCycle - 1;
                        else
                            pendingReadCycle = pendingReadCycle - operand->outputLatency;
                        if (maximumReadyCycle <= pendingReadCycle)
                            maximumReadyCycle = pendingReadCycle + 1;
                    }
                }
            }
        }
        instruction->dynamicEarliestStart = maximumReadyCycle;
        if (maximumReadyCycle > block->maximumDynamicStartRow)
            block->maximumDynamicStartRow = maximumReadyCycle;
        for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            operand = &instruction->operands[operandIndex];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int registerPartIndex = 0; registerPartIndex != reg->partCount; registerPartIndex++) {
                    bool bit = (operand->componentMask >> registerPartIndex) & 1;
                    if (bit) {
                        OpSlot *slot = &operand->parts[registerPartIndex];
                        rPart *registerPart = reg->getRP(registerPartIndex);
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
        for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            operand = &instruction->operands[operandIndex];
            RegPoolEntry *reg = registerOperand(operand, 2);
            if (reg) {
                for (int registerPartIndex = 0; registerPartIndex != reg->partCount; registerPartIndex++) {
                    bool bit = (operand->componentMask >> registerPartIndex) & 1;
                    if (bit) {
                        rPart *registerPart = reg->getRP(registerPartIndex);
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
        for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int registerPartIndex = 0; registerPartIndex != reg->partCount; registerPartIndex++) {
                    OpSlot *slot = &operand->parts[registerPartIndex];
                    bool bit = (operand->componentMask >> registerPartIndex) & 1;
                    if (bit && !isAAInputLock(slot)) {
                        OperandLock *par = slot->getParent();
                        if (!isRegLock(par)) {
                            Instr *predecessorInstruction = par->ownerInstr();
                            int lat = lockLatency(slot, par);
                            int earliestStartCandidate = instruction->dynamicEarliestStart - lat;
                            if (earliestStartCandidate < 0) {
                                reportError("-ve ip?\n");
                                instruction->dumpOperandDiagnostics(false);
                                table->dump();
                                instruction->dumpOperandDiagnostics(false);
                                exitAfterCleanup(-1);
                            }
                            if (predecessorInstruction->dynamicReverseStart > earliestStartCandidate)
                                predecessorInstruction->dynamicReverseStart = earliestStartCandidate;
                        }
                    }
                }
            }
        }
    }

    for (instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        for (operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (reg) {
                for (int registerPartIndex = 0; registerPartIndex != reg->partCount; registerPartIndex++) {
                    OpSlot *slot = &operand->parts[registerPartIndex];
                    bool bit = (operand->componentMask >> registerPartIndex) & 1;
                    if (bit && !isAAInputLock(slot)) {
                        OperandLock *par = slot->getParent();
                        if (!isRegLock(par)) {
                            Instr *lockOwnerInstruction = par->ownerInstr();
                            int lat;
                            if (slot->latencyModeFlag)
                                lat = 1;
                            else
                                lat = par->issueLatency;
                            int reverseStartCandidate = lockOwnerInstruction->dynamicReverseStart + lat;
                            if (reverseStartCandidate < instruction->dynamicUpperBound)
                                instruction->dynamicUpperBound = reverseStartCandidate;
                            reverseStartCandidate = instruction->dynamicEarliestStart - lat;
                            if (reverseStartCandidate > lockOwnerInstruction->dynamicReverseEnd)
                                lockOwnerInstruction->dynamicReverseEnd = reverseStartCandidate;
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
    int instructionIndex = 0;
    for (Instr *instruction = block->pendingInstructions.head; instruction; instruction = instruction->pendingLink.nextInstruction) {
        const char *mark = "    ";
        int earliestStart = instruction->dynamicEarliestStart;
        int upperBound = instruction->dynamicUpperBound;
        int dep = block->loopSize;
        if (earliestStart - upperBound > dep)
            mark = "****";
        int heightToTarget = block->maximumDynamicStartRow - instruction->schedulingStatistics.reverseLatencyDepth;
        dep = instruction->schedulingStatistics.forwardLatencyDepth;
        reportInfo("[%3d]:%3d [%5d %5d %5d %5d ]%s dep=%2d ht=%2d  %s\n",
                      instructionIndex, block->loopSize, upperBound, earliestStart, instruction->dynamicReverseStart, instruction->dynamicReverseEnd, mark,
                      dep, heightToTarget, instruction->opcodeInfo->mnemonic);
        instructionIndex++;
    }
    reportInfo("dyDuumpStats - end\n");
    errorOccurred = saved;
}
