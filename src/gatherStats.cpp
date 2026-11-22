#include "os.h"
#include "lock_shared.h"

SchedulingStatistics::SchedulingStatistics()
{
}

SchedulingStatistics::~SchedulingStatistics()
{
}

void SchedulingStatistics::noopStatisticsHook()
{
}

void SchedulingStatistics::invalidateAllCounters()
{
    reverseLatencyDepth = reverseDependencyHeight = -1;
    forwardLatencyDepth = forwardDependencyHeight = -1;
    moveCount = moveDistance = -1;
    moveChainLength = -1;
    independentLatencyDepth = -1;
}

void SchedulingStatistics::zeroAllCounters()
{
    reverseLatencyDepth = reverseDependencyHeight = 0;
    forwardLatencyDepth = forwardDependencyHeight = 0;
    moveCount = moveDistance = 0;
    moveChainLength = 0;
    independentLatencyDepth = 0;
}

void SchedulingStatistics::copyCountersFrom(SchedulingStatistics *src)
{
    reverseLatencyDepth = src->reverseLatencyDepth;
    reverseDependencyHeight = src->reverseDependencyHeight;
    forwardLatencyDepth = src->forwardLatencyDepth;
    forwardDependencyHeight = src->forwardDependencyHeight;
    moveCount = src->moveCount;
    moveDistance = src->moveDistance;
    moveChainLength = src->moveChainLength;
    independentLatencyDepth = src->independentLatencyDepth;
}

void SchedulingStatistics::mergeMaximumCounters(SchedulingStatistics *statistics)
{
    if (statistics->reverseLatencyDepth > reverseLatencyDepth) reverseLatencyDepth = statistics->reverseLatencyDepth;
    if (statistics->reverseDependencyHeight > reverseDependencyHeight) reverseDependencyHeight = statistics->reverseDependencyHeight;
    if (statistics->forwardLatencyDepth > forwardLatencyDepth) forwardLatencyDepth = statistics->forwardLatencyDepth;
    if (statistics->forwardDependencyHeight > forwardDependencyHeight) forwardDependencyHeight = statistics->forwardDependencyHeight;
    if (statistics->moveCount > moveCount) moveCount = statistics->moveCount;
    if (statistics->moveChainLength > moveChainLength) moveChainLength = statistics->moveChainLength;
    if (statistics->moveDistance > moveDistance) moveDistance = statistics->moveDistance;
    if (statistics->independentLatencyDepth > independentLatencyDepth) independentLatencyDepth = statistics->independentLatencyDepth;
}

char stallReasonTextBuffer[1024];  // stall reason text buffer

const char *stallReasonStr(short why)
{
    const char *reasonName = "dunno why";
    switch (why & 0xff) {
    case 1:
        reasonName = "instruction latency";
        break;
    case 2:
        reasonName = "instruction thruput";
        break;
    case 3:
        reasonName = "number of upper instructions";
        break;
    case 4:
        reasonName = "number of lower instructions";
        break;
    case 5:
        reasonName = "can't loop";
        break;
    case 6:
        reasonName = "dependency chain depth";
        break;
    case 7:
        reasonName = "dependency chain height";
        break;
    case 8:
        reasonName = "branch instruction dependency chain height";
        break;
    case 9:
        formatIntoBuffer(stallReasonTextBuffer, "issue pressure on %s", deviceNameByIndex((char)((why & 0xff00) >> 8)));
        reasonName = stallReasonTextBuffer;
        break;
    case 10:
        reasonName = "circular dependency chain";
        break;
    }
    return reasonName;
}

bool InstructionStream::gatherStats()
{
    SchedulingStatistics *streamStats = &schedulingStatistics;
    streamStats->reverseLatencyDepth = streamStats->reverseDependencyHeight = -1;
    streamStats->forwardLatencyDepth = streamStats->forwardDependencyHeight = -1;
    streamStats->moveCount = streamStats->moveDistance = -1;
    streamStats->moveChainLength = -1;
    streamStats->independentLatencyDepth = -1;
    int physSum = 0;
    int maxLat = 0;
    branchForwardLatencyDepth = -1;
    resetOnlyStreamBranchTimingSentinel = -1;
    statCounts *statistics = &stats;
    for (int resourceIndex = 0; resourceIndex != 9; resourceIndex++)
        statistics->resourceInstructionCounts[resourceIndex] = 0;
    savedStallReason = 0;
    currentStallReason = 0;
    ScheduleTable *table = getTable();
    if (!table) {
        reportError("failed to gather stats - no get work table\n");
        return false;
    }

    Instr *instruction;
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        SchedulingStatistics *instructionStatistics = &instruction->schedulingStatistics;
        instructionStatistics->reverseLatencyDepth = instructionStatistics->reverseDependencyHeight = 0;
        instructionStatistics->forwardLatencyDepth = instructionStatistics->forwardDependencyHeight = 0;
        instructionStatistics->moveCount = instructionStatistics->moveDistance = 0;
        instructionStatistics->moveChainLength = 0;
        instructionStatistics->independentLatencyDepth = 0;
        int logged = 0;
        if (instruction) {
            for (int resourceUsageIndex = 0; resourceUsageIndex != 9; resourceUsageIndex++) {
                bool resourceEnabled = (instruction->opcodeResourceMask >> resourceUsageIndex) & 1;
                if (resourceEnabled)
                    statistics->resourceInstructionCounts[resourceUsageIndex] += 2;
            }
            logged = 1;
        }
        if ((unsigned char)logged == 0) {
            reportError("failed to log\n");
            return false;
        }
    }

    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            Operand *operand = &instruction->operands[operandIndex];
            RegisterEntry *registerEntry = (isRegisterOperand(operand, 1) && operand->operandStatus != 2) ? operand->assignedRegister : 0;
            if (registerEntry) {
                for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                    if ((operand->componentMask >> partIndex) & 1) {
                        for (OperandLocation *operandLocation = operand->parts[partIndex].operandLocations.head; operandLocation; operandLocation = operandLocation->next) {
                            if (!operandLocation->instruction) {
                                reportError("failed to get setting instruction!\n");
                                return false;
                            }
                            if (iOwn(operandLocation->instruction)) {
                                Operand *sourceOperand = operandAtLocation(operandLocation);
                                if (!sourceOperand) {
                                    reportError("failed to get setting operand for instruction\n");
                                    return false;
                                }
                                int totalLatency = sourceOperand->outputLatency + operand->additionalLatency;
                                if (maxLat < totalLatency)
                                    maxLat = totalLatency;
                            }
                        }
                    }
                }
            }
        }
    }

    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        for (int statsOperandIndex = 0; statsOperandIndex != instruction->operandCount; statsOperandIndex++) {
            Operand *operand = &instruction->operands[statsOperandIndex];
            RegisterEntry *registerEntry;
            if (isRegisterOperand(operand, 1) && operand->operandStatus != 2)
                registerEntry = operand->assignedRegister;
            else
                registerEntry = 0;
            if (registerEntry) {
                for (int statsPartIndex = 0; statsPartIndex != registerEntry->partCount; statsPartIndex++) {
                    if (((operand->componentMask & ~operand->externalPartMask) >> statsPartIndex) & 1) {
                        if (operand->parts[statsPartIndex].operandLocations.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *sourceInstruction = operand->parts[statsPartIndex].operandLocations.head->instruction;
                        Operand *sourceOperand = operandAtLocation(operand->parts[statsPartIndex].operandLocations.head);
                        int forwardLatencyDepth = sourceInstruction->schedulingStatistics.forwardLatencyDepth + sourceOperand->outputLatency + operand->additionalLatency;
                        int forwardDependencyHeight = sourceInstruction->schedulingStatistics.forwardDependencyHeight + 1;
                        if (forwardLatencyDepth > instruction->schedulingStatistics.forwardLatencyDepth)
                            instruction->schedulingStatistics.forwardLatencyDepth = forwardLatencyDepth;
                        if (forwardDependencyHeight > instruction->schedulingStatistics.forwardDependencyHeight)
                            instruction->schedulingStatistics.forwardDependencyHeight = forwardDependencyHeight;
                    }
                }
            }
        }
    }

    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        for (int statsOperandIndex = 0; statsOperandIndex != instruction->operandCount; statsOperandIndex++) {
            Operand *operand = &instruction->operands[statsOperandIndex];
            RegisterEntry *registerEntry = (isRegisterOperand(operand, 1) && operand->operandStatus != 2) ? operand->assignedRegister : 0;
            if (registerEntry) {
                for (int statsPartIndex = 0; statsPartIndex != registerEntry->partCount; statsPartIndex++) {
                    if (((operand->componentMask & ~operand->externalPartMask) >> statsPartIndex) & 1) {
                        if (operand->parts[statsPartIndex].operandLocations.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *sourceInstruction = operand->parts[statsPartIndex].operandLocations.head->instruction;
                        Operand *sourceOperand = operandAtLocation(operand->parts[statsPartIndex].operandLocations.head);
                        int independentLatencyDepth = sourceInstruction->schedulingStatistics.independentLatencyDepth + sourceOperand->outputLatency + operand->additionalLatency;
                        if (independentLatencyDepth > instruction->schedulingStatistics.independentLatencyDepth)
                            instruction->schedulingStatistics.independentLatencyDepth = independentLatencyDepth;
                    }
                }
            }
        }
    }

    InstructionList *lst = &table->linearInstructions;
    for (instruction = lst->tail; instruction; instruction = instruction->linearLink.previousInstruction) {
        for (int statsOperandIndex = 0; statsOperandIndex != instruction->operandCount; statsOperandIndex++) {
            Operand *operand = &instruction->operands[statsOperandIndex];
            RegisterEntry *registerEntry;
            if (isRegisterOperand(operand, 1) && operand->operandStatus != 2)
                registerEntry = operand->assignedRegister;
            else
                registerEntry = 0;
            if (registerEntry) {
                for (int statsPartIndex = 0; statsPartIndex != registerEntry->partCount; statsPartIndex++) {
                    if (((operand->componentMask & ~operand->externalPartMask) >> statsPartIndex) & 1) {
                        if (operand->parts[statsPartIndex].operandLocations.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *sourceInstruction = operand->parts[statsPartIndex].operandLocations.head->instruction;
                        Operand *sourceOperand = operandAtLocation(operand->parts[statsPartIndex].operandLocations.head);
                        int reverseLatencyDepth = instruction->schedulingStatistics.reverseLatencyDepth + sourceOperand->outputLatency + operand->additionalLatency;
                        int reverseDependencyHeight = instruction->schedulingStatistics.reverseDependencyHeight + 1;
                        if (reverseLatencyDepth > sourceInstruction->schedulingStatistics.reverseLatencyDepth)
                            sourceInstruction->schedulingStatistics.reverseLatencyDepth = reverseLatencyDepth;
                        if (reverseDependencyHeight > sourceInstruction->schedulingStatistics.reverseDependencyHeight)
                            sourceInstruction->schedulingStatistics.reverseDependencyHeight = reverseDependencyHeight;
                    }
                }
            }
        }
    }

    for (instruction = lst->tail; instruction; ) {
        iLink *link = &instruction->linearLink;
        Instr *moveInstruction = link->previousInstruction;
        for (; moveInstruction; moveInstruction = moveInstruction->linearLink.previousInstruction) {
            if (hasDirectOperandDependency(moveInstruction, instruction)) {
                moveInstruction->schedulingStatistics.moveCount++;
                moveInstruction->schedulingStatistics.moveDistance += instruction->schedulingStatistics.moveDistance + 1;
                moveInstruction->schedulingStatistics.moveChainLength = instruction->schedulingStatistics.moveChainLength + 1;
            }
        }
        // Keep the original pointer-word load while reading a complete
        // address on either 32- or 64-bit targets.
        moveInstruction = (Instr *)*(VclPtrWord *)&link->previousInstruction;
        instruction = moveInstruction;
    }

    int upper = 0;
    int lower = 0;
    int thru = 0;
    int best = 0;
    int both = 0;
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        if (instruction->branchFlags)
            branchForwardLatencyDepth = instruction->schedulingStatistics.forwardLatencyDepth;
        char pipelineMask = instruction->permittedPipelineMask;
        if (pipelineMask == 3)
            both++;
        else if (pipelineMask & 2)
            lower++;
        else
            upper++;
        schedulingStatistics.mergeMaximumCounters(&instruction->schedulingStatistics);
    }

    int maxIssue = 0;
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        for (int statsOperandIndex = 0; statsOperandIndex != instruction->operandCount; statsOperandIndex++) {
            Operand *operand = &instruction->operands[statsOperandIndex];
            RegisterEntry *registerEntry = registerOperand(operand, 2);
            if (registerEntry) {
                for (int statsPartIndex = 0; statsPartIndex != registerEntry->partCount; statsPartIndex++) {
                    int nBoth = 0;
                    int n1 = 0;
                    int nOther = 0;
                    if ((operand->componentMask >> statsPartIndex) & 1) {
                        for (OperandLocation *operandLocation = operand->parts[statsPartIndex].operandLocations.firstInternalLocation(); operandLocation; operandLocation = operandLocation->nextInternalLocation()) {
                            char instructionPipelineMask = operandLocation->instruction->permittedPipelineMask;
                            if (instructionPipelineMask == 3)
                                nBoth++;
                            else if (instructionPipelineMask == 1)
                                n1++;
                            else
                                nOther++;
                        }
                    }
                    int higherPipeCount = n1;
                    int lowerPipeCount = nOther;
                    if (n1 < nOther) {
                        higherPipeCount = nOther;
                        lowerPipeCount = n1;
                    }
                    int rem = nBoth - (higherPipeCount - lowerPipeCount);
                    if (rem > 0)
                        higherPipeCount += (rem + 1) >> 1;
                    if (higherPipeCount)
                        higherPipeCount--;
                    higherPipeCount += operand->outputLatency;
                    if (higherPipeCount > maxIssue)
                        maxIssue = higherPipeCount;
                }
            }
        }
    }

    {
        int upperPipeCount = upper;
        int initialLowerPipeCount = lower;
        for (; both; both--) {
            if (upperPipeCount < initialLowerPipeCount)
                upperPipeCount++;
            else
                initialLowerPipeCount++;
        }
        if (upperPipeCount > best) {
            currentStallReason = 3;
            best = upperPipeCount;
        }
        if (initialLowerPipeCount > best) {
            currentStallReason = 4;
            best = initialLowerPipeCount;
        }
    }
    if (maxIssue > best) {
        best = maxIssue;
        currentStallReason = 1;
    }

    int depth = schedulingStatistics.independentLatencyDepth;
    if (depth < 0)
        depth = 0;
    for (RegisterUse *registerUse = table->registerUses.head; registerUse; registerUse = registerUse->next) {
        RegisterEntry *registerEntry = registerUse->registerEntry;
        if (!registerEntry) {
            reportError("regref had no reg\n");
            return false;
        }
        for (int statsPartIndex = 0; statsPartIndex != registerEntry->partCount; statsPartIndex++) {
            if ((registerUse->outputMask >> statsPartIndex) & 1) {
                regUseSlot *useSlot = &registerUse->slots[statsPartIndex];
                int totalThroughput = useSlot->throughputSums[0];
                if (totalThroughput > thru)
                    thru = totalThroughput;
                int throughputDepth = totalThroughput - useSlot->throughputSums[3];
                if (throughputDepth > depth)
                    depth = throughputDepth;
                physSum += useSlot->throughputSums[1];
            }
        }
    }
    if (thru > best) {
        currentStallReason = 2;
        best = thru;
    }
    if (maxLat > best) {
        currentStallReason = 1;
        best = maxLat;
    }
    for (int resourceSlotIndex = 0; resourceSlotIndex != 9; resourceSlotIndex++) {
        if (resourceSlotIndex < 0 || resourceSlotIndex > 8) {
            reportError("bad stat\n");
            exitAfterCleanup(-1);
        }
        int resourceInstructionCount = (statistics->resourceInstructionCounts[resourceSlotIndex] + 1) >> 1;
        if (resourceInstructionCount > best) {
            best = resourceInstructionCount;
            currentStallReason = (resourceSlotIndex << 8) | 9;
        }
    }
    if (physSum < best * 4)
        physSum = best * 4;
    if (physSum < 64)
        physSum = 64;
    if (physSum > 0x4000)
        physSum = 0x4000;
    if (table->codeModeValue == 3 && physSum < table->getPhysicalLen())
        physSum = table->getPhysicalLen();
    physicalInstructionBudget = physSum;
    if (canLoop()) {
        savedStallReason = currentStallReason;
        logicalLength = best;
        if (best <= 1)
            logicalLength = 2;
    } else {
        savedStallReason = 5;
        logicalLength = 0;
    }
    if (schedulingStatistics.forwardLatencyDepth >= best) {
        currentStallReason = 6;
        best = schedulingStatistics.forwardLatencyDepth + 1;
    }
    if (schedulingStatistics.reverseLatencyDepth >= best) {
        currentStallReason = 7;
        best = schedulingStatistics.reverseLatencyDepth + 1;
    }
    Instr *branchInstruction = table->getBranch();
    if (branchInstruction) {
        int branchLatencyDepth = branchInstruction->schedulingStatistics.forwardLatencyDepth + 2;
        if (best <= branchLatencyDepth) {
            currentStallReason = 8;
            best = branchLatencyDepth;
        }
    }
    dependencyLength = best;
    table->streamDependencyLength = dependencyLength;
    table->streamLogicalLength = logicalLength;
    table->logicalInstructionLength = depth;
    return true;
}

static inline int clampDepLen(int len)
{
    if (len <= 0)
        len = 0;
    return len;
}

int InstructionStream::getRecordedDependencyLength()
{
    int len = dependencyLength;
    if (len <= 0)
        len = 0;
    return len;
}

int InstructionStream::getRecordedLogicalLength()
{
    int len = logicalLength;
    if (len <= 0)
        len = 0;
    return len;
}

int InstructionStream::getOrGuessDepLength()
{
    ScheduleTable *table = getTable();
    if (!table) {
        reportError("getOrGuess - no work table\n");
        exitAfterCleanup(-1);
    }
    int len = 0;
    switch (table->codeModeValue) {
    case 1:
        len = clampDepLen(dependencyLength);
        break;
    case 3:
        len = table->getRawOrSyntheticLogicalLength();
        break;
    }
    return len;
}

int InstructionStream::getClampedDependencyLength()
{
    ScheduleTable *table = getTable();
    if (!table) {
        reportError("getOrGuess - no work table\n");
        exitAfterCleanup(-1);
    }
    int len = 0;
    switch (table->codeModeValue) {
    case 1:
    case 3:
        len = clampDepLen(dependencyLength);
        break;
    }
    return len;
}

int InstructionStream::getOrGuessLogicalLength()
{
    ScheduleTable *table = workTablePtr;
    if (!table)
        table = getWorkTable();
    if (!table) {
        reportError("getOrGuess - no work table\n");
        exitAfterCleanup(-1);
    }
    return table->getLogicalLen();
}

void statCounts::clear()
{
    for (int resourceIndex = 0; resourceIndex != 9; resourceIndex++)
        resourceInstructionCounts[resourceIndex] = 0;
}

bool statCounts::add(Instr *instruction)
{
    if (!instruction)
        return false;
    for (int resourceIndex = 0; resourceIndex != 9; resourceIndex++) {
        bool resourceEnabled = (instruction->opcodeResourceMask >> resourceIndex) & 1;
        if (resourceEnabled)
            resourceInstructionCounts[resourceIndex] += 2;
    }
    return true;
}

int statCounts::get(int statisticIndex)
{
    if (statisticIndex < 0 || statisticIndex > 8) {
        reportError("bad stat\n");
        exitAfterCleanup(-1);
    }
    return (resourceInstructionCounts[statisticIndex] + 1) >> 1;
}

