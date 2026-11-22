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

char *emitGenericInstructionText(char *outputBuffer, Instr *instruction, bool upper, char *mods, bool symbolic, int unusedEmissionMode)
{
    OpcodeDescriptor *opcodeInfo = instruction->opcodeInfo;
    bool comma = false;
    if (!mods)
        mods = "";
    int len = formatIntoBuffer(outputBuffer, "%s%s", opcodeInfo->mnemonic, mods);
    char *outputCursor = outputBuffer + len;
    outputCursor = emitDestMask(outputCursor, (char)opcodeInfo->destinationMaskMode, instruction->destinationMask);
    if (!outputCursor) {
        reportError("failed to emit inst mods\n");
        return 0;
    }
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

char *emitReversibleBranch(char *outputBuffer, Instr *branchInstruction, bool upper, char *mods, bool symbolic, int unusedEmissionMode)
{
    OpcodeDescriptor *opcodeInfo = branchInstruction->opcodeInfo;
    bool comma = false;
    if (!mods)
        mods = "";
    if (branchInstruction->reverseBranchSense) {
        if ((branchInstruction->branchFlags >> 4) & 1) {
            reportError("can't reverse sense of a likely branch at the emit stage\n");
            return 0;
        }
        opcodeInfo = findOpcodeInfoByMnemonic(opcodeInfo->reverseSenseMnemonic, false);
        if (!opcodeInfo) {
            reportError("failed to find sense reversing instruction\n");
            return 0;
        }
    }
    int len = formatIntoBuffer(outputBuffer, "%s%s", opcodeInfo->mnemonic, mods);
    char *outputCursor = outputBuffer + len;
    outputCursor = emitDestMask(outputCursor, (char)opcodeInfo->destinationMaskMode, branchInstruction->destinationMask);
    if (!outputCursor) {
        reportError("failed to generate instruction mod string\n");
        return 0;
    }
    for (int paddingIndex = outputCursor - outputBuffer; paddingIndex < 14; paddingIndex++)
        *outputCursor++ = ' ';
    for (int operandIndex = 0; operandIndex != branchInstruction->operandCount; operandIndex++) {
        Operand *currentOperand = &branchInstruction->operands[operandIndex];
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
            dumpEmitInstruction(branchInstruction);
            return 0;
        }
    }
    return outputCursor;
}

RegisterEntry *findPhysicalRegister(unsigned type, const char *name);

bool extHook(void * unusedHookContext, Instr *instruction)
{
    int done = 0;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        if (operand->operandKind != 55)
            continue;
        if (operand->assignedRegister) {
            reportError("extHook - register already set\n");
            return 0;
        }
        RegisterEntry *registerEntry = findPhysicalRegister((unsigned char)operand->immediateDataType, "EXT");
        if (!registerEntry) {
            reportError("couldn't get ext register for pool\n");
            return 0;
        }
        operand->assignedRegister = registerEntry;
        operand->componentMask = (1 << registerEntry->partCount) - 1;
        if (!operand->componentMask) {
            reportError("no mod bits set for ext\n");
            return 0;
        }
        done++;
    }
    if (!done) {
        reportError("extHook - didn't do anything\n");
        return 0;
    }
    return 1;
}

