#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quietDiagnostics;
void resetRange();
int setRangeFrom(ScheduleTable *t);
char *dupString(const char *s);

static inline void dumpEmitInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

bool rotateInputComponentMask(void * unusedHookContext, Instr *instruction)
{
    Operand *operand = &instruction->operands[1];
    operand->componentMask = (((operand->componentMask & 8) >> 3) | (operand->componentMask << 1)) & 0xf;
    return 1;
}

bool efu_Hook(void *opcodeMetadata, Instr *instruction)
{
    OpcodeDescriptor *opcodeInfo = (OpcodeDescriptor *)opcodeMetadata;
    Operand *operand = &instruction->operands[1];
    switch (opcodeInfo->efuMaskType) {
    case 1: break;
    case 2: operand->componentMask = 3; break;
    case 3: operand->componentMask = 5; break;
    case 4: operand->componentMask = VCL_COMPONENT_XYZ; break;
    case 5: operand->componentMask = VCL_COMPONENT_ALL; break;
    default:
        reportError("unknown EFU mod bits type during efu_Hook at build\n");
        return 0;
    }
    return 1;
}

bool asciiCaseEqual(const char *a, const char *b);

bool wr2w_Hook(void *opcodeMetadata, Instr *moveInstruction)
{
    OpcodeDescriptor *opcodeInfo = (OpcodeDescriptor *)opcodeMetadata;
    OpcodeDescriptor *instructionInfo;
    if (asciiCaseEqual(opcodeInfo->mnemonic, "ilwr"))
        instructionInfo = findOpcodeInfoByMnemonic("ilw", false);
    else if (asciiCaseEqual(opcodeInfo->mnemonic, "iswr"))
        instructionInfo = findOpcodeInfoByMnemonic("isw", false);
    else {
        reportError("wr2w_Hook - couldn't find instruction '%s'\n", opcodeInfo->mnemonic);
        return 0;
    }
    if (!instructionInfo) {
        reportError("wr2w_Hook - failed to find extended instruction\n");
        return 0;
    }
    moveInstruction->opcodeInfo = instructionInfo;
    moveInstruction->operands[1].operandKind = VCL_OP_V_EA_IMM_REG;
    moveInstruction->operands[1].immediateValue.setInt(0);
    return 1;
}

char *emitClip(char *outputBuffer, Instr *instruction, bool upper, char *mods, bool symbolic, int unusedEmissionMode)
{
    bool comma = false;
    int len = formatIntoBuffer(outputBuffer, "clipw%s.xyz", mods);
    char *outputCursor = outputBuffer + len;
    for (int paddingIndex = outputCursor - outputBuffer; paddingIndex < 14; paddingIndex++)
        *outputCursor++ = ' ';
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *currentOperand = &instruction->operands[operandIndex];
        if (currentOperand->isImplicit)
            continue;
        if (comma)
            *outputCursor++ = ',';
        comma = true;
        outputCursor = currentOperand->emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor) {
            reportError("failed to emit operand\n");
            currentOperand->diagnoseOperand(0);
            reportError("from\n");
            dumpEmitInstruction(instruction);
            return 0;
        }
    }
    return outputCursor;
}

char *vmove_Emit(char *outputBuffer, Instr *instruction, bool upper, char *mods, bool symbolic, int unusedEmissionMode)
{
    char *outputCursor;
    if (!instruction)
        return 0;
    if (!instruction->abstractMoveOpcode) {
        reportError("baseMuid not set in vmove_Emit\n");
        return 0;
    }
    if (!instruction->concreteMoveOpcode) {
        reportError("trying to emit an abstract move\n");
        return 0;
    }
    outputCursor = outputBuffer;
    switch (instruction->concreteMoveOpcode) {
    case 0xb3:
        outputCursor += formatIntoBuffer(outputBuffer, "iaddiu");
        for (int paddingIndex = outputCursor - outputBuffer; paddingIndex < 14; paddingIndex++)
            *outputCursor++ = ' ';
        outputCursor = instruction->operands[0].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        *outputCursor++ = ',';
        outputCursor = instruction->operands[1].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        *outputCursor++ = ',';
        *outputCursor++ = '0';
        break;
    case 0xb5:
        outputCursor += formatIntoBuffer(outputBuffer, "%s%s", "move", mods);
        outputCursor = emitDestMask(outputCursor, 1, instruction->destinationMask);
        if (!outputCursor) {
            reportError("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int secondPaddingIndex = outputCursor - outputBuffer; secondPaddingIndex < 14; secondPaddingIndex++)
            *outputCursor++ = ' ';
        outputCursor = instruction->operands[0].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        *outputCursor++ = ',';
        outputCursor = instruction->operands[1].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        break;
    case 0xb6:
        outputCursor += formatIntoBuffer(outputBuffer, "%s%s", "max", mods);
        outputCursor = emitDestMask(outputCursor, 1, instruction->destinationMask);
        if (!outputCursor) {
            reportError("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int thirdPaddingIndex = outputCursor - outputBuffer; thirdPaddingIndex < 14; thirdPaddingIndex++)
            *outputCursor++ = ' ';
        outputCursor = instruction->operands[0].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        *outputCursor++ = ',';
        outputCursor = instruction->operands[1].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        *outputCursor++ = ',';
        outputCursor = instruction->operands[1].emitOperandText(outputCursor, upper, symbolic);
        if (!outputCursor)
            goto fail;
        break;
    default:
        reportError("unknown vmove output requested\n");
        return 0;
    }
    *outputCursor = 0;
    goto done;
fail:
    reportError("failed to emit operand 0\n");
    return 0;
done:
    return outputCursor;
}

bool semSynth(void *opcodeMetadata, Instr *syntheticInstruction)
{
    OpcodeDescriptor *opcodeInfo = (OpcodeDescriptor *)opcodeMetadata;
    char name[32];
    char *nameDestination = name;
    const char *mnemonicSource = opcodeInfo->mnemonic;
    while (*mnemonicSource)
        *nameDestination++ = *mnemonicSource++;
    if (syntheticInstruction->operands[0].operandKind == VCL_OP_V_ACC) {
        if (nameDestination[-1] != 'a' && nameDestination[-1] != 'A') {
            *nameDestination++ = 'a';
            syntheticInstruction->operands[0].outputLatency = 1;
        }
    } else
        syntheticInstruction->operands[0].outputLatency = 4;
    char operandKind = syntheticInstruction->operands[2].operandKind;
    if (operandKind == VCL_OP_V_I) {
        syntheticInstruction->operands[2].externalBlockModeFlag = 1;
        *nameDestination++ = 'i';
    }
    if (operandKind == VCL_OP_V_Q) {
        *nameDestination++ = 'q';
        syntheticInstruction->operands[2].externalBlockModeFlag = 0;
    }
    if (operandKind == VCL_OP_V_FL1) {
        unsigned char componentMask = syntheticInstruction->operands[2].componentMask;
        int componentCount = 0;
        if (componentMask == VCL_COMPONENT_X) { *nameDestination++ = 'x'; componentCount++; }
        if (componentMask == VCL_COMPONENT_Y) { *nameDestination++ = 'y'; componentCount++; }
        if (componentMask == VCL_COMPONENT_Z) { *nameDestination++ = 'z'; componentCount++; }
        if (componentMask == VCL_COMPONENT_W) { *nameDestination++ = 'w'; componentCount++; }
        if (componentCount != 1) {
            reportError("semSynth - bad FL1 bits\n");
            return 0;
        }
    }
    *nameDestination = 0;
    OpcodeDescriptor *instructionInfo = findOpcodeInfoByMnemonic(name, false);
    if (!instructionInfo) {
        reportError("failed to find derived instruction type '%s'\n", name);
        return 0;
    }
    syntheticInstruction->opcodeInfo = instructionInfo;
    return 1;
}
