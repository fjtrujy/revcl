#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quietDiagnostics;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

static inline void dumpEmitInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

char *emitGenericInstructionText(char *buf, Instr *instruction, bool upper, char *mods, bool b, int)
{
    iInfo *info = instruction->opcodeInfo;
    bool comma = false;
    if (!mods)
        mods = "";
    int len = formatIntoBuffer(buf, "%s%s", info->mnemonic, mods);
    char *outputCursor = buf + len;
    outputCursor = emitDestMask(outputCursor, (char)info->destinationMaskMode, instruction->destinationMask);
    if (!outputCursor) {
        reportError("failed to emit inst mods\n");
        return 0;
    }
    for (int paddingIndex = outputCursor - buf; paddingIndex < 14; paddingIndex++)
        *outputCursor++ = ' ';
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *currentOperand = &instruction->operands[operandIndex];
        if (currentOperand->isImplicit)
            continue;
        if (comma)
            *outputCursor++ = ',';
        comma = true;
        outputCursor = currentOperand->Emit(outputCursor, upper, b);
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

char *emitReversibleBranch(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    iInfo *info = in->opcodeInfo;
    bool comma = false;
    if (!mods)
        mods = "";
    if (in->reverseBranchSense) {
        if ((in->branchFlags >> 4) & 1) {
            reportError("can't reverse sense of a likely branch at the emit stage\n");
            return 0;
        }
        info = findOpcodeInfoByMnemonic(info->reverseSenseMnemonic, false);
        if (!info) {
            reportError("failed to find sense reversing instruction\n");
            return 0;
        }
    }
    int len = formatIntoBuffer(buf, "%s%s", info->mnemonic, mods);
    char *outputCursor = buf + len;
    outputCursor = emitDestMask(outputCursor, (char)info->destinationMaskMode, in->destinationMask);
    if (!outputCursor) {
        reportError("failed to generate instruction mod string\n");
        return 0;
    }
    for (int paddingIndex = outputCursor - buf; paddingIndex < 14; paddingIndex++)
        *outputCursor++ = ' ';
    for (int operandIndex = 0; operandIndex != in->operandCount; operandIndex++) {
        Operand *currentOperand = &in->operands[operandIndex];
        if (currentOperand->isImplicit)
            continue;
        if (comma)
            *outputCursor++ = ',';
        comma = true;
        outputCursor = currentOperand->Emit(outputCursor, upper, b);
        if (!outputCursor) {
            reportError("failed to emit operand\n");
            currentOperand->diagnoseOperand(0);
            reportError("from\n");
            dumpEmitInstruction(in);
            return 0;
        }
    }
    return outputCursor;
}

RegPoolEntry *getReg(unsigned type, const char *name);

bool extHook(void *, Instr *instruction)
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
        RegPoolEntry *registerEntry = getReg((unsigned char)operand->immediateDataType, "EXT");
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

