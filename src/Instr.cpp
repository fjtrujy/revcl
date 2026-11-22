#include "VUStr.h"

// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include <string.h>
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

char *dupString(const char *s);

extern int errorOccurred;
extern bool quietDiagnostics;

opPair_s pairedOperandDescriptions[] = {
    {109, {1, 1, 0, 2}, 0},
    {110, {1, 1, 0, 2}, 0},
    {112, {2, 1, 0, 2}, 0},
    {158, {1, 2, 1, -1}, 1},
    {163, {1, 2, 1, -1}, 1},
    {159, {2, 2, 1, -1}, 1},
    {164, {2, 2, 1, -1}, 1},
    {0, {0}, 0},
};

opCount_s operandCountOverrides[] = {
    {155, 1}, {160, 1}, {156, 1}, {161, 1}, {0, 0},
};

Instr::Instr(OpcodeDescriptor *info)
{
    if (!info) {
        reportError("constructor called with null iInfo -... later will make this a NOP, but , for now..halt!\n");
        exitAfterCleanup(-1);
    }
    operandCount = 0;
    requiresFullLoopMoveDrag = 0;
    instructionModifierBits = 0;
    destinationMask = 0;
    previousThroughputWriter = 0;
    sourceTag = 0;
    sourceTokenStartIndex = 0;
    sourceTokenEndIndex = 0;
    moveRowOffset = 0;
    moveChainDepth = 0;
    abstractMoveOpcode = 0;
    concreteMoveOpcode = 0;
    parsedLoopId = -1;
    branchLabelTags = 0;
    replacedInstructionWord = 0;
    ownerTable = 0;
    moveRecordWord = 0;
    externalInputPairedConstraint = 0;
    externalInputConstraint = 0;
    externalOutputPairedConstraint = 0;
    externalOutputConstraint = 0;
    linearOrderIndex = -1;
    instructionSerial = -1;
    candidateAttemptCount = 0;
    candidateAttemptLimit = 0;
    opcodeInfo = info;
    candidateBaseRow = 0;
    cloneImage = 0;
    optimizedInstructionImageWord = 0;
    originalInstructionImageWord = 0;
    reverseBranchSense = 0;
    tildePrefix = 0;
    scheduledCycle = -1;
    candidateCostOffset = 0;
    expectedAliasLoopId = -1;
    schedulerCategory = -1;
    resetImmFixup();
    opcodeInfo = info;
    originalPipelineMask = info->pipelineMask;
    permittedPipelineMask = info->pipelineMask;
    opcodeResourceMask = info->resourceMask;
    branchFlags = info->branchFlags();
    baseIssueSpacing = info->minimumIssueSpacing;
    ramAccessType = info->ramType();
    opcodeFlags = info->kindAndFlags & ~7;
    rawOnly = info->isRawOnly();
    syntheticKind = info->syntheticKind();
    for (int operandIndex = 0; info->operandDescriptors[operandIndex].operandKind; operandIndex++) {
        operandCount++;
        Operand *operand = &operands[operandIndex];
        iOpDesc *descriptor = &info->operandDescriptors[operandIndex];
        operand->operandKind = descriptor->operandKind;
        operand->operandIndex = operandIndex;
        operand->externalBlockModeFlag = descriptor->externalBlockModeFlag != 0;
        operand->direction = descriptor->direction;
        operand->isImplicit = descriptor->isImplicit != 0;
        operand->immediateDataType = descriptor->immediateDataType;
        operand->pairedOperandIndexPlusOne = descriptor->patchedOutputIndexPlusOne;
        if (operand->pairedOperandIndexPlusOne)
            operands[operand->pairedOperandIndexPlusOne - 1].pairedOperandIndexPlusOne = operandIndex + 1;
        if (descriptor->direction == 2)
            operand->outputLatency = descriptor->outputLatency;
        else
            operand->outputLatency = 0;
    }
}

Instr::~Instr()
{
    if (branchLabelTags) {
        delete[] branchLabelTags;
        branchLabelTags = 0;
    }
    TypedValue *value = &addressImmediateValue;
    value->clear();
    predecessorDependencies.clear();
    successorDependencies.clear();
}

bool Instr::clearDynamicData()
{
    if (branchLabelTags) {
        delete[] branchLabelTags;
        branchLabelTags = 0;
    }
    addressImmediateValue.clear();
    predecessorDependencies.clear();
    successorDependencies.clear();
    return true;
}

bool Instr::validateOperands()
{
    if (!opcodeInfo)
        return false;
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++)
        if (!operands[operandIndex].validateKindAndMask())
            return false;
    return true;
}

void Instr::dumpOperandDiagnostics(int flags)
{
    if (isValid()) {
        int count = operandCount;
        for (int operandIndex = 0; operandIndex != count; operandIndex++) {
            operands[operandIndex].diagnoseOperand(0);
            count = operandCount;
        }
        if (flags & 1) {
            for (int diagnosticOperandIndex = 0; diagnosticOperandIndex != operandCount; diagnosticOperandIndex++) {
                Operand *operand = &operands[diagnosticOperandIndex];
                RegisterEntry *registerEntry = operand->assignedRegister;
                if (registerEntry && registerEntry->isVirtual)
                    for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++)
                        if ((operand->componentMask >> partIndex) & 1)
                            operand->parts[partIndex].operandLocations.validateOperandLocations();
            }
        }
    }
}

bool Instr::isNop()
{
    if (!opcodeInfo)
        return true;
    return opcodeInfo->isNop();
}

InstructionStream *Instr::getOwningStream()
{
    return ownerTable ? ownerTable->ownerStream : 0;
}

bool Instr::resetFlowDependencies()
{
    predecessorDependencies.clear();
    successorDependencies.clear();
    scheduledCycle = -1;
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++)
        if (!operands[operandIndex].resetFlowState())
            return false;
    previousThroughputWriter = 0;
    return true;
}

bool Instr::isNonSynthetic()
{
    return !syntheticKind;
}

bool Instr::clearTransientFlags()
{
    candidateAttemptCount = 0;
    candidateCostOffset = 0;
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        for (int partIndex = 0; partIndex != 4; partIndex++)
            operand->parts[partIndex].conflictingOutputLockCount = 0;
    }
    return true;
}

void Instr::dumpInstructionContext()
{
    dumpInstructionContext(-1, -1);
}

void Instr::dumpInstructionContext(int opnd)
{
    dumpInstructionContext(opnd, -1);
}

void Instr::dumpInstructionContext(int opnd, int mask)
{
    if (opnd >= operandCount) {
        reportError("Instruction::leDump - out of bounds operand!\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quietDiagnostics;
    if (sourceTag) {
        int idx = sourceTokenStartIndex;
        if (opnd >= 0) {
            if (operands[opnd].assignedRegister && operands[opnd].assignedRegister->partCount != 1 && mask >= 0)
                reportMessage("(field(s) : %s) ", formatComponentMaskLetters(mask));
            if (operands[opnd].isImplicit) {
                if (operands[opnd].assignedRegister)
                    reportMessage("implicit operand '%s'\n", operands[opnd].assignedRegister->registerName());
                else
                    reportError("implicit operand with no register??\n");
            } else
                idx = operands[opnd].sourceTokenStartIndex;
        }
        sourceTag->dumpSourceLine(idx);
    } else {
        reportMessage("operand %d modbits %x..\n", opnd, mask);
        dumpOperandDiagnostics(false);
    }
    errorOccurred = saved;
}

bool hasDirectOperandDependency(Instr *firstInstruction, Instr *secondInstruction)
{
    if (!firstInstruction || !secondInstruction)
        return false;
    for (int operandIndex = 0; operandIndex != secondInstruction->operandCount; operandIndex++) {
        Operand *operand = &secondInstruction->operands[operandIndex];
        RegisterEntry *registerEntry = registerOperand(operand, 1);
        if (registerEntry) {
            int registerPartCount = registerEntry->partCount;
            for (int partIndex = 0; partIndex != registerPartCount; partIndex++)
                if (((operand->componentMask & ~operand->externalPartMask) >> partIndex) & 1)
                    for (OperandLocation *currentLocation = operand->parts[partIndex].operandLocations.head; currentLocation; currentLocation = currentLocation->next)
                        if (currentLocation->instruction == firstInstruction)
                            return true;
        }
    }
    return false;
}

bool writesInputRegisterOf(Instr *firstInstruction, Instr *secondInstruction)
{
    if (!firstInstruction || !secondInstruction)
        return false;
    for (int readerOperandIndex = 0; readerOperandIndex != secondInstruction->operandCount; readerOperandIndex++) {
        Operand *operand = &secondInstruction->operands[readerOperandIndex];
        RegisterEntry *registerEntry = registerOperand(operand, 1);
        if (registerEntry) {
            int want = operand->componentMask;
            int mask = 0;
            Instr *instruction = firstInstruction;
            for (int writerOperandIndex = 0; writerOperandIndex != instruction->operandCount; writerOperandIndex++)
                if (registerOperand(&instruction->operands[writerOperandIndex], 2) == registerEntry)
                    mask |= instruction->operands[writerOperandIndex].componentMask;
            if (mask & want)
                return true;
        }
    }
    return false;
}



Instr *Instr::clone()
{
    if (!this)
        return 0;
    OpcodeDescriptor *info = opcodeInfo;
    if (!info)
        return 0;
    Instr *cloneInstruction = new Instr(info);
    if (!cloneInstruction) {
        reportError("failed to get a new instruction during clone!\n");
        exitAfterCleanup(-1);
    }
    if (!cloneInstruction->resetOptimizationState()) {
        reportError("failed to opt reset cloned instruction\n");
        exitAfterCleanup(-1);
    }
    cloneInstruction->destinationMask = destinationMask;
    cloneInstruction->rawOnly = rawOnly;
    cloneInstruction->moveRecordWord = 0;
    cloneInstruction->instructionModifierBits = instructionModifierBits;
    cloneInstruction->abstractMoveOpcode = abstractMoveOpcode;
    cloneInstruction->concreteMoveOpcode = concreteMoveOpcode;
    cloneInstruction->originalPipelineMask = originalPipelineMask;
    cloneInstruction->schedulingStatistics.copyCountersFrom(&schedulingStatistics);
    cloneInstruction->linearOrderIndex = linearOrderIndex;
    cloneInstruction->instructionSerial = instructionSerial;
    cloneInstruction->candidateAttemptLimit = candidateAttemptLimit;
    cloneInstruction->parsedLoopId = parsedLoopId;
    cloneInstruction->tildePrefix = tildePrefix;
    cloneInstruction->reverseBranchSense = reverseBranchSense;
    cloneInstruction->sourceTag = sourceTag;
    cloneInstruction->sourceTokenStartIndex = sourceTokenStartIndex;
    cloneInstruction->sourceTokenEndIndex = sourceTokenEndIndex;
    cloneInstruction->addressAliasMode = addressAliasMode;
    cloneInstruction->addressAliasDirection = addressAliasDirection;
    cloneInstruction->addressAliasOperandIndex = addressAliasOperandIndex;
    if (addressImmediateValue.isValid())
        cloneInstruction->addressImmediateValue.set(&addressImmediateValue);
    cloneInstruction->expectedAliasLoopId = expectedAliasLoopId;
    if (cloneImage) {
        reportError("during instruction clone - tmpClone was already set!\n");
        delete cloneInstruction;
        return 0;
    }
    cloneImage = cloneInstruction;
    cloneInstruction->cloneImage = this;
    if (branchLabelTags)
        cloneInstruction->branchLabelTags = dupString(branchLabelTags);
    cloneInstruction->operandCount = operandCount;
    Operand *sourceOperand;
    for (int operandIndex = 0; (sourceOperand = getOp(operandIndex)) != 0; operandIndex++) {
        Operand *destinationOperand = cloneInstruction->getOp(operandIndex);
        if (destinationOperand == 0) {
            reportError("fatal error, couldn't get operand during clone\n");
            exitAfterCleanup(-1);
        }
        if (sourceOperand->operandIndex != operandIndex) {
            reportError("bad index in operand for clone\n");
            exitAfterCleanup(-1);
        }
        destinationOperand->operandIndex = sourceOperand->operandIndex;
        destinationOperand->operandKind = sourceOperand->operandKind;
        destinationOperand->direction = sourceOperand->direction;
        destinationOperand->outputLatency = sourceOperand->outputLatency;
        destinationOperand->immediateDataType = sourceOperand->immediateDataType;
        destinationOperand->externalBlockModeFlag = sourceOperand->externalBlockModeFlag;
        destinationOperand->assignedRegister = sourceOperand->assignedRegister;
        destinationOperand->componentMask = sourceOperand->componentMask;
        destinationOperand->addressComponentMask = sourceOperand->addressComponentMask;
        destinationOperand->pairedOperandIndexPlusOne = sourceOperand->pairedOperandIndexPlusOne;
        destinationOperand->isImplicit = sourceOperand->isImplicit;
        destinationOperand->originalRegister = sourceOperand->originalRegister;
        destinationOperand->forcedBaseRegister = sourceOperand->forcedBaseRegister;
        if (sourceOperand->immediateValue.isValid()) {
            if (!destinationOperand->immediateValue.set(&sourceOperand->immediateValue)) {
                reportError("failed to clone immediate\n");
                exitAfterCleanup(-1);
            }
        }
        destinationOperand->sourceTokenStartIndex = sourceOperand->sourceTokenStartIndex;
        destinationOperand->sourceTokenEndIndex = sourceOperand->sourceTokenEndIndex;
        destinationOperand->additionalLatency = sourceOperand->additionalLatency;
        destinationOperand->externalPartMask = sourceOperand->externalPartMask;
        destinationOperand->operandStatus = 0;
        destinationOperand->externalComponentMask = sourceOperand->externalComponentMask;
        destinationOperand->reductionHazardMask = sourceOperand->reductionHazardMask;
    }
    return cloneInstruction;
}




int Instr::getRamBits()
{
    int ramBits;
    if (!ramAccessType) {
        memset(&ramBits, 0, sizeof(ramBits));
        return ramBits;
    }
    ramBits = 0;
    if (opcodeInfo) {
        switch ((char)opcodeInfo->destinationMaskMode) {
        case 1:
            ramBits = destinationMask;
            break;
        default:
            reportError("got here! - end of get rambits?\n");
            dumpOperandDiagnostics(false);
            exitAfterCleanup(-1);
            memset(&ramBits, 0, sizeof(ramBits));
            return ramBits;
        case 3: {
            Operand *operand = getOp(1);
            if (!operand) {
                reportError("failed to get op1 for mod_1 ram test\n");
                exitAfterCleanup(-1);
            }
            if (!operand->addressComponentMask)
                reportWarning("mb was 0 failed to get xmod bits for mod 1 inst\n");
            ramBits = operand->addressComponentMask;
            break;
        }
        case 0:
        case 4:
        case 5:
            ramBits = 15;
            break;
        }
    }
    return ramBits;
}

bool Instr::resetOptimizationState()
{
    position.clearInstructionPosition();
    nextMoveInstruction = 0;
    pendingLink.previousInstruction = 0;
    pendingLink.nextInstruction = 0;
    candidateAttemptCount = 0;
    replacedInstructionWord = 0;
    candidateCostOffset = 0;
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        if (!operands[operandIndex].resetOptimizationSlots()) {
            reportError("failed to optReset operand  %d\n", operandIndex);
            return false;
        }
    }
    return true;
}

int Instr::collectOutputRegisterMask(RegisterEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *operand;
    for (int operandIndex = 0; (operand = getOp(operandIndex)) != 0; operandIndex++) {
        RegisterEntry *registerEntry;
        if (operand->isOutput())
            registerEntry = operand->assignedRegister;
        else
            registerEntry = 0;
        if (reg == registerEntry) {
            *paired = *paired || operand->pairedOperandIndexPlusOne;
            mask |= operand->componentMask;
        }
    }
    return mask;
}

int Instr::collectInputRegisterMask(RegisterEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *operand;
    for (int operandIndex = 0; (operand = getOp(operandIndex)) != 0; operandIndex++) {
        RegisterEntry *registerEntry;
        if (operand->isInput())
            registerEntry = operand->assignedRegister;
        else
            registerEntry = 0;
        if (reg == registerEntry) {
            *paired = *paired || operand->pairedOperandIndexPlusOne;
            mask |= operand->componentMask;
        }
    }
    return mask;
}


bool Instr::getImmediateOperand(int operandIndex, int *out)
{
    Operand *operand = getOp(operandIndex);
    if (!operand) {
        reportError("failed to get operand %d\n", operandIndex);
        return false;
    }
    return operand->immediateValue.getInt(out);
}







bool Instr::changeBranchImmediate(const char *immediateText)
{
    if (!branchFlags || !opcodeInfo) {
        reportError("changeBranchImmediate: not a branch\n");
        return false;
    }
    if (branchFlags & 2) {
        reportError("changeBranchImmediate: target is register!\n");
        return false;
    }
    int targetOperandSelector = opcodeInfo->branchTargetOperandSelector;
    if (!targetOperandSelector) {
        reportError("getBranchTargetImmediate - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *operand = &operands[targetOperandSelector - 1];
    if (operand->operandKind != 0x1b) {
        reportError("changeBranchImmediate::target operand isn't immediate!\n");
        exitAfterCleanup(-1);
    }
    return operand->immediateValue.setString(immediateText);
}

bool linkOperandParts(Instr *instruction, int aOp, int slot, Instr *secondInstruction, int bOp, int bSlot)
{
    if (!instruction || !secondInstruction) {
        reportError("link_oParts, null arg pointer!\n");
        return false;
    }
    if (bSlot != slot) {
        reportError("swizzle detected for ps2 ?\n");
        return false;
    }
    OpSlot *firstOperandSlot = &instruction->operands[aOp].parts[slot];
    OpSlot *secondOperandSlot = &secondInstruction->operands[bOp].parts[slot];
    OperandLocation *location = firstOperandSlot->operandLocations.addOperandLocation(secondInstruction, bOp, slot, false);
    if (location) {
        olList *locationList = &secondOperandSlot->operandLocations;
        OperandLocation *reciprocalLocation = locationList->addOperandLocation(instruction, aOp, slot, false);
        if (!reciprocalLocation) {
            reportError("failed to add ol\n");
            return false;
        }
        return true;
    }
    reportError("failed to add ol\n");
    return false;
}

static inline void dumpLinkOperand(Operand *operand)
{
    operand->diagnoseOperand(0);
}

bool linkOperandParts(OperandLocation *location, OperandLocation *inputLocation, int minimumPathKey, int maximumPathKey)
{
    if (!location) {
        reportError("link_oParts, null arg pointer2!\n");
        return false;
    }
    if (!inputLocation) {
        reportError("link_oParts, null arg pointer2!\n");
        return false;
    }
    Operand *out = operandAtLocation(location);
    if (!out) {
        reportError("link_oParts(2) - output not set \n");
        return false;
    }
    if (!isRegisterOperand(out, 2)) {
        reportError("link_oParts - output was not output\n");
        dumpLinkOperand(out);
        return false;
    }
    Operand *operand = operandAtLocation(inputLocation);
    if (!operand) {
        reportError("link_oParts(2) - input not set \n");
        return false;
    }
    if (!isRegisterOperand(operand, 1)) {
        reportError("link_oParts - input was not input\n");
        return false;
    }
    int outputComponentIndex = location->componentIndex;
    int inputComponentIndex = inputLocation->componentIndex;
    if (!((out->componentMask >> outputComponentIndex) & 1)) {
        reportError("link_oParts - mod bit not set for output\n");
        return false;
    }
    if (!((operand->componentMask >> inputComponentIndex) & 1)) {
        reportError("link_oParts - mod bit not set for input\n");
        return false;
    }
    if (!((operand->externalPartMask >> inputComponentIndex) & 1)) {
        reportError("link_oParts_ext - external bit not set for input\n");
        return false;
    }
    out->externalPartMask |= 1 << outputComponentIndex;
    if (locationOwningStream(location) != locationOwningStream(inputLocation)) {
        out->externalComponentMask |= 1 << location->componentIndex;
        operand->externalComponentMask |= 1 << inputLocation->componentIndex;
    }
    if (outputComponentIndex != inputComponentIndex) {
        reportError("link_oParts - not a bug, but should not happen for this cpu, t_o!=t_i (%d!=%d)\n", outputComponentIndex, inputComponentIndex);
        return false;
    }
    OpSlot *outputOperandSlot = out->parts + inputComponentIndex;
    OpSlot *inputOperandSlot = operand->parts + inputComponentIndex;
    olList *locationList = &outputOperandSlot->operandLocations;
    OperandLocation *linkedInputLocation = locationList->copyOperandLocation(inputLocation, true);
    if (!linkedInputLocation) {
        reportError("failed to add ol_input to output\n");
        return false;
    }
    olList *inputLocationList = &inputOperandSlot->operandLocations;
    OperandLocation *linkedOutputLocation = inputLocationList->copyOperandLocation(location, true);
    if (!linkedOutputLocation) {
        reportError("failed to add ol_ouput to input\n");
        return false;
    }
    int clampedCycleOffset = location->cycleOffset;
    if (clampedCycleOffset > 0xff)
        clampedCycleOffset = 0xff;
    linkedInputLocation->cycleOffset = clampedCycleOffset;
    linkedInputLocation->pathKey = minimumPathKey;
    linkedInputLocation->maximumPathKey = maximumPathKey;
    linkedOutputLocation->cycleOffset = 0;
    linkedOutputLocation->pathKey = minimumPathKey;
    linkedOutputLocation->maximumPathKey = maximumPathKey;
    return true;
}

bool Instr::setOwner(InstructionOwnerTableView *ownerTablePointer)
{
    if (ownerTable && ownerTable != ownerTablePointer) {
        reportError("in->setOwner - already owned..but by wrong stream!\n");
        exitAfterCleanup(-1);
    }
    ownerTable = ownerTablePointer;
    return true;
}

const char *branchTypeNameByCode(int branchType)
{
    switch (branchType) {
    case 0:
        return "BTYPE_NULL";
    case 1:
        return "BTYPE_ABS";
    case 3:
        return "BTYPE_REG";
    case 5:
        return "BTYPE_COND_ABS";
    case 7:
        return "BTYPE_COND_REG";
    case 9:
        return "BTYPE_ABS_LINK";
    case 11:
        return "BTYPE_REG_LINK";
    default:
        return "BTYPE_TYPE_THIS_IN!";
    case 13:
        return "BTYPE_COND_ABS_LINK";
    }
}

char *Instr::emitExpectedSyntax(bool flag, char *outputBuffer, int unusedFormatMode)
{
    if (!outputBuffer)
        return 0;
    OpcodeDescriptor *info = opcodeInfo;
    if (info) {
        outputBuffer += formatIntoBuffer(outputBuffer, info->mnemonic);
        switch ((char)info->destinationMaskMode) {
        case 1:
            outputBuffer += formatIntoBuffer(outputBuffer, ".[xyzw]");
            break;
        case 2:
            outputBuffer += formatIntoBuffer(outputBuffer, ".xyz");
            break;
        case 3:
            outputBuffer += formatIntoBuffer(outputBuffer, ".[x/y/z/w]");
            break;
        }
    } else {
        outputBuffer += formatIntoBuffer(outputBuffer, "nop");
    }
    outputBuffer += formatIntoBuffer(outputBuffer, "\t");
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        if (operands[operandIndex].isImplicit)
            break;
        if (operandIndex)
            outputBuffer += formatIntoBuffer(outputBuffer, ",");
        outputBuffer = operands[operandIndex].emitExpectedOperandType(flag, outputBuffer);
        if (!outputBuffer)
            break;
    }
    return outputBuffer;
}

int Instr::outputMaskForRegister(RegisterEntry *registerEntry, int mask)
{
    int result = 0;
    if (registerEntry) {
        for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
            Operand *operand = &operands[operandIndex];
            if (operand->outRegZ() == registerEntry)
                result |= operand->componentMask;
        }
    }
    return result & mask;
}

int Instr::inputMaskForRegister(RegisterEntry *registerEntry, int mask)
{
    int result = 0;
    if (registerEntry) {
        for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
            Operand *operand = &operands[operandIndex];
            if (operand->inRegOp2() == registerEntry)
                result |= operand->componentMask;
        }
    }
    return result & mask;
}

const char *syntheticTypeName(int syntheticKind)
{
    const char *syntheticName = "SYNTH_UNKNOWN";
    switch (syntheticKind) {
    case 0: syntheticName = "SYNTH_NULL"; break;
    case 1: syntheticName = "SYNTH_ENTER"; break;
    case 2: syntheticName = "SYNTH_ENDENTER"; break;
    case 3: syntheticName = "SYNTH_EXIT"; break;
    case 4: syntheticName = "SYNTH_ENDEXIT"; break;
    case 5: syntheticName = "SYNTH_CONT"; break;
    case 6: syntheticName = "SYNTH_XINPUT"; break;
    case 7: syntheticName = "SYNTH_XOUTPUT"; break;
    case 8: syntheticName = "SYNTH_LOOP_MINSAFE"; break;
    case 9: syntheticName = "SYNTH_LOOP_EXTRA"; break;
    case 10: syntheticName = "SYNTH_BARRIER"; break;
    }
    return syntheticName;
}

bool Instr::hasRawTableOrImmediateToken()
{
    if (optimizedInstructionImageWord)
        return true;
    if (ownerTable)
        return ownerTable->codeModeValue == 3;
    return false;
}

char *Instr::getBranchTargetImmediate()
{
    OpcodeDescriptor *branchOpcodeInfo = opcodeInfo;
    if (!branchFlags)
        return 0;
    if (!branchOpcodeInfo) {
        reportError("branch type set - but no ii\n");
        exitAfterCleanup(-1);
    }
    int targetOperandSelector = branchOpcodeInfo->branchTargetOperandSelector;
    if (!targetOperandSelector) {
        reportError("getBranchTargetImmediate - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *operand = &operands[targetOperandSelector - 1];
    if (!operand->isInput()) {
        reportError("alleged branch target immediate wasn't an input\n");
        exitAfterCleanup(-1);
    }
    if (operand->operandKind == 0x1b)
        return operand->immediateValue.getString();
    return 0;
}

RegisterEntry *Instr::getBranchTargetRegister()
{
    OpcodeDescriptor *branchOpcodeInfo = opcodeInfo;
    if (!(branchFlags & 2))
        return 0;
    if (!branchOpcodeInfo) {
        reportError("branch type set - but no ii\n");
        exitAfterCleanup(-1);
    }
    int targetOperandSelector = branchOpcodeInfo->branchTargetOperandSelector;
    if (!targetOperandSelector) {
        reportError("getBranchTargetRegister - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *operand = &operands[targetOperandSelector - 1];
    if (!operand->isInput()) {
        reportError("alleged branch target register wasn't an input\n");
        exitAfterCleanup(-1);
    }
    if (operand->operandKind == 0x1b)
        return 0;
    RegisterEntry *registerEntry = operand->assignedRegister;
    if (!registerEntry) {
        reportError("branch target register wasn't set\n");
        exitAfterCleanup(-1);
    }
    return registerEntry;
}

int Instr::constantZeroInstructionHook()
{
    return 0;
}

Operand *Instr::findInputOperand(RegisterEntry *registerEntry, int mask)
{
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        if (operand->inRegOp2() == registerEntry && (operand->componentMask & mask))
            return operand;
    }
    return 0;
}

Operand *Instr::findOutputOperand(RegisterEntry *registerEntry, int mask)
{
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        if (operand->outRegZ() == registerEntry && (operand->componentMask & mask))
            return operand;
    }
    return 0;
}

bool Instr::hasLinkedOutputFrom(unsigned int operandIndex)
{
    if (!opcodeInfo)
        return false;
    for (; operandIndex < operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        RegisterEntry *registerEntry = registerOperand(operand, 2);
        if (registerEntry) {
            if (operand->externalPartMask)
                return true;
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++)
                if (((operand->componentMask >> partIndex) & 1) && operand->parts[partIndex].operandLocations.count)
                    return true;
        }
    }
    return false;
}

bool Instr::classifySchedulingCategory()
{
    if (ramAccessType == 2) {
        schedulerCategory = 1;
        return true;
    }
    if (opcodeFlags & 0x40) {
        schedulerCategory = 2;
        return true;
    }
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        if (isRegisterOperand(operand, 2) && operand->externalComponentMask) {
            schedulerCategory = 2;
            return true;
        }
    }
    schedulerCategory = 0;
    return true;
}

Instr **Instr::getIE()
{
    InstructionOwnerTableView *table = ownerTable;
    if (!table) {
        reportError("failed to get itable from instruction\n");
        return 0;
    }
    InstructionPosition *pos = &position;
    Instr **pipelineEntries = table->getIE_ck(pos->linearRow);
    if (!pipelineEntries) {
        reportError("failed to get ie from table for in::getIE()\n");
        return 0;
    }
    if (pipelineEntries[pos->pipelineIndex] != this) {
        reportError("we aren't where we thought we were for in::getIE\n");
        return 0;
    }
    return pipelineEntries;
}

int Instr::getFirstOutputLatency()
{
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        if (operand->outRegZ() && !operand->externalBlockModeFlag)
            return operand->outputLatency;
    }
    return 0;
}

bool Instr::markDependencyInputReachable()
{
    bool wasInputUnreachable = !inputDependencyReachable;
    inputDependencyReachable = 1;
    return wasInputUnreachable;
}

bool Instr::markDependencyOutputReachable()
{
    bool wasOutputUnreachable = !outputDependencyReachable;
    outputDependencyReachable = 1;
    return wasOutputUnreachable;
}

int Instr::opcodeId()
{
    if (!opcodeInfo)
        return -1;
    return getOpcodeId(opcodeInfo);
}

static inline bool isLiteral(TypedValue *value)
{
    return value->type == TypedValue::STRING;
}

bool Instr::expandVirtualStatOperand()
{
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        if (operand->operandKind < 22 || operand->operandKind > 25)
            continue;
        if (operandIndex != operandCount - 1) {
            reportError("virtual stat operand not final\n");
            return false;
        }
        operandCount--;
        switch (operand->operandKind) {
        case 23:
            operand->isImplicit = 1;
            operand->operandKind = 21;
            operand->direction = 2;
            operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(55);
            operand->componentMask = 1;
            operand->outputLatency = 4;
            operand->externalBlockModeFlag = 0;
            operand->operandIndex = operandCount++;
            operand = &operands[operandIndex + 1];
            operand->isImplicit = 1;
            operand->operandKind = 18;
            operand->direction = 1;
            operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(57);
            operand->componentMask = 1;
            operand->outputLatency = -1;
            operand->externalBlockModeFlag = 1;
            operand->operandIndex = operandCount++;
            break;
        case 22:
            operand->isImplicit = 1;
            operand->operandKind = 20;
            operand->direction = 2;
            operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(56);
            operand->componentMask = 1;
            operand->outputLatency = operands[0].outputLatency;
            operand->externalBlockModeFlag = 0;
            operand->operandIndex = operandCount;
            operand = &operands[operandIndex + 1];
            operandCount++;
            operand->isImplicit = 1;
            operand->operandKind = 19;
            operand->direction = 1;
            operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(58);
            operand->componentMask = 1;
            operand->outputLatency = -1;
            operand->externalBlockModeFlag = 1;
            operand->operandIndex = operandCount++;
            break;
        case 24: {
            if (operands[operandIndex - 1].operandKind != 27) {
                reportError("not immediate predecessor in fs test\n");
                return false;
            }
            bool aluStatusInputNeeded = true;
            bool divisionStatusInputNeeded = true;
            bool aluStatusShadowNeeded = true;
            bool divisionStatusShadowNeeded = true;
            TypedValue *value = &operands[operandIndex - 1].immediateValue;
            unsigned int imm;
            if (isLiteral(value)) {
                reportWarning("fsop has literals - so assuming the worst - if it were numerical vcl could figure out dependencies better\n");
            } else if (value->getUInt(&imm)) {
                unsigned int bits;
                int opcodeId = -1;
                if (opcodeInfo)
                    opcodeId = getOpcodeId(opcodeInfo);
                switch (opcodeId) {
                case 147:
                    break;
                case 148:
                    bits = ~imm;
                    goto test;
                case 146:
                    bits = imm;
                test:
                    aluStatusInputNeeded = (bits & 0xf) != 0;
                    divisionStatusInputNeeded = (bits & 0x30) != 0;
                    aluStatusShadowNeeded = (bits & 0xc00) != 0;
                    divisionStatusShadowNeeded = (bits & 0x3c0) != 0;
                    break;
                default:
                    reportError("bad fsop \n");
                    return false;
                }
            } else {
                reportError("failed to get immediate value\n");
                return false;
            }
            aluStatusInputNeeded = aluStatusInputNeeded | aluStatusShadowNeeded;
            divisionStatusInputNeeded = divisionStatusInputNeeded | divisionStatusShadowNeeded;
            if (aluStatusInputNeeded) {
                operand->isImplicit = 1;
                operand->operandKind = 21;
                operand->direction = 1;
                operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(55);
                operand->componentMask = 1;
                operand->outputLatency = -1;
                operand->externalBlockModeFlag = 0;
                operand->operandIndex = operandCount;
                operand++;
                operandCount++;
            }
            if (divisionStatusInputNeeded) {
                operand->isImplicit = 1;
                operand->operandKind = 20;
                operand->direction = 1;
                operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(56);
                operand->componentMask = 1;
                operand->outputLatency = -1;
                operand->externalBlockModeFlag = 0;
                operand->operandIndex = operandCount;
                operand++;
                operandCount++;
            }
            if (aluStatusShadowNeeded) {
                operand->isImplicit = 1;
                operand->operandKind = 18;
                operand->direction = 1;
                operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(57);
                operand->componentMask = 1;
                operand->outputLatency = -1;
                operand->externalBlockModeFlag = 1;
                operand->operandIndex = operandCount;
                operand++;
                operandCount++;
                operand->isImplicit = 1;
                operand->operandKind = 18;
                operand->direction = 2;
                operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(57);
                operand->componentMask = 1;
                operand->outputLatency = 1;
                operand->externalBlockModeFlag = 1;
                operand->operandIndex = operandCount;
                operand++;
                operandCount++;
            }
            if (divisionStatusShadowNeeded) {
                operand->isImplicit = 1;
                operand->operandKind = 19;
                operand->direction = 1;
                operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(58);
                operand->componentMask = 1;
                operand->outputLatency = -1;
                operand->externalBlockModeFlag = 1;
                operand->operandIndex = operandCount;
                operand++;
                operandCount++;
                operand->isImplicit = 1;
                operand->operandKind = 19;
                operand->direction = 2;
                operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(58);
                operand->componentMask = 1;
                operand->outputLatency = 1;
                operand->externalBlockModeFlag = 1;
                operand->operandIndex = operandCount++;
            }
            break;
        }
        case 25:
            operand->isImplicit = 1;
            operand->operandKind = 18;
            operand->direction = 2;
            operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(57);
            operand->componentMask = 1;
            operand->outputLatency = 1;
            operand->externalBlockModeFlag = 1;
            operand->operandIndex = operandCount;
            operand = &operands[operandIndex + 1];
            operandCount++;
            operand->isImplicit = 1;
            operand->operandKind = 19;
            operand->direction = 2;
            operand->originalRegister = operand->assignedRegister = findRegisterByGlobalIndex(58);
            operand->componentMask = 1;
            operand->outputLatency = 1;
            operand->externalBlockModeFlag = 1;
            operand->operandIndex = operandCount++;
            break;
        }
    }
    return true;
}

void Instr::clearExternalTimingConstraints()
{
    externalInputPairedConstraint = 0;
    externalInputConstraint = 0;
    externalOutputPairedConstraint = 0;
    externalOutputConstraint = 0;
}

void Instr::copyExternalTimingConstraints(Instr *sourceInstruction)
{
    externalInputPairedConstraint = sourceInstruction->externalInputPairedConstraint;
    externalInputConstraint = sourceInstruction->externalInputConstraint;
    externalOutputPairedConstraint = sourceInstruction->externalOutputPairedConstraint;
    externalOutputConstraint = sourceInstruction->externalOutputConstraint;
}

void Instr::raiseExternalInputPairedConstraint(short pairedInputConstraint)
{
    externalInputPairedConstraint = externalInputPairedConstraint < pairedInputConstraint ? pairedInputConstraint : externalInputPairedConstraint;
}

void Instr::raiseExternalInputConstraint(short inputConstraint)
{
    externalInputConstraint = externalInputConstraint < inputConstraint ? inputConstraint : externalInputConstraint;
}

void Instr::raiseExternalOutputPairedConstraint(short pairedOutputConstraint)
{
    externalOutputPairedConstraint = externalOutputPairedConstraint < pairedOutputConstraint ? pairedOutputConstraint : externalOutputPairedConstraint;
}

void Instr::raiseExternalOutputConstraint(short outputConstraint)
{
    externalOutputConstraint = externalOutputConstraint < outputConstraint ? outputConstraint : externalOutputConstraint;
}

OpSlot::OpSlot()
{
}

OpSlot::~OpSlot()
{
}

void OpSlot::noopOperandSlotHook()
{
}

bool OpSlot::resetSlotLock()
{
    resetLock();
    conflictingOutputLockCount = 0;
    outputConflictMarked[0] = 0;
    return true;
}


int OpSlot::clearSlotLocations()
{
    return operandLocations.clear();
}
