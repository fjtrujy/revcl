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

bool rotateInputComponentMask(void *, Instr *instruction)
{
    Operand *operand = &instruction->operands[1];
    operand->componentMask = (((operand->componentMask & 8) >> 3) | (operand->componentMask << 1)) & 0xf;
    return 1;
}

bool efu_Hook(void *p, Instr *instruction)
{
    iInfo *info = (iInfo *)p;
    Operand *operand = &instruction->operands[1];
    switch (info->efuMaskType) {
    case 1: break;
    case 2: operand->componentMask = 3; break;
    case 3: operand->componentMask = 5; break;
    case 4: operand->componentMask = 7; break;
    case 5: operand->componentMask = 15; break;
    default:
        reportError("unknown EFU mod bits type during efu_Hook at build\n");
        return 0;
    }
    return 1;
}

bool asciiCaseEqual(const char *a, const char *b);

bool wr2w_Hook(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    iInfo *instructionInfo;
    if (asciiCaseEqual(info->mnemonic, "ilwr"))
        instructionInfo = findOpcodeInfoByMnemonic("ilw", false);
    else if (asciiCaseEqual(info->mnemonic, "iswr"))
        instructionInfo = findOpcodeInfoByMnemonic("isw", false);
    else {
        reportError("wr2w_Hook - couldn't find instruction '%s'\n", info->mnemonic);
        return 0;
    }
    if (!instructionInfo) {
        reportError("wr2w_Hook - failed to find extended instruction\n");
        return 0;
    }
    in->opcodeInfo = instructionInfo;
    in->operands[1].operandKind = 0x1f;
    in->operands[1].immediateValue.setInt(0);
    return 1;
}

char *emitClip(char *buf, Instr *instruction, bool upper, char *mods, bool b, int)
{
    bool comma = false;
    int len = formatIntoBuffer(buf, "clipw%s.xyz", mods);
    char *outputCursor = buf + len;
    for (int n = outputCursor - buf; n < 14; n++)
        *outputCursor++ = ' ';
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *op = &instruction->operands[operandIndex];
        if (op->isImplicit)
            continue;
        if (comma)
            *outputCursor++ = ',';
        comma = true;
        outputCursor = op->Emit(outputCursor, upper, b);
        if (!outputCursor) {
            reportError("failed to emit operand\n");
            op->diagnoseOperand(0);
            reportError("from\n");
            dumpEmitInstruction(instruction);
            return 0;
        }
    }
    return outputCursor;
}

char *vmove_Emit(char *buf, Instr *instruction, bool upper, char *mods, bool b, int)
{
    char *p;
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
    p = buf;
    switch (instruction->concreteMoveOpcode) {
    case 0xb3:
        p += formatIntoBuffer(buf, "iaddiu");
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = instruction->operands[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = instruction->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        *p++ = '0';
        break;
    case 0xb5:
        p += formatIntoBuffer(buf, "%s%s", "move", mods);
        p = emitDestMask(p, 1, instruction->destinationMask);
        if (!p) {
            reportError("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = instruction->operands[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = instruction->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        break;
    case 0xb6:
        p += formatIntoBuffer(buf, "%s%s", "max", mods);
        p = emitDestMask(p, 1, instruction->destinationMask);
        if (!p) {
            reportError("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = instruction->operands[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = instruction->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = instruction->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        break;
    default:
        reportError("unknown vmove output requested\n");
        return 0;
    }
    *p = 0;
    goto done;
fail:
    reportError("failed to emit operand 0\n");
    return 0;
done:
    return p;
}

bool semSynth(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    char name[32];
    char *nameDestination = name;
    const char *mnemonicSource = info->mnemonic;
    while (*mnemonicSource)
        *nameDestination++ = *mnemonicSource++;
    if (in->operands[0].operandKind == 9) {
        if (nameDestination[-1] != 'a' && nameDestination[-1] != 'A') {
            *nameDestination++ = 'a';
            in->operands[0].outputLatency = 1;
        }
    } else
        in->operands[0].outputLatency = 4;
    char k = in->operands[2].operandKind;
    if (k == 4) {
        in->operands[2].externalBlockModeFlag = 1;
        *nameDestination++ = 'i';
    }
    if (k == 7) {
        *nameDestination++ = 'q';
        in->operands[2].externalBlockModeFlag = 0;
    }
    if (k == 3) {
        unsigned char m = in->operands[2].componentMask;
        int n = 0;
        if (m == 1) { *nameDestination++ = 'x'; n++; }
        if (m == 2) { *nameDestination++ = 'y'; n++; }
        if (m == 4) { *nameDestination++ = 'z'; n++; }
        if (m == 8) { *nameDestination++ = 'w'; n++; }
        if (n != 1) {
            reportError("semSynth - bad FL1 bits\n");
            return 0;
        }
    }
    *nameDestination = 0;
    iInfo *instructionInfo = findOpcodeInfoByMnemonic(name, false);
    if (!instructionInfo) {
        reportError("failed to find derived instruction type '%s'\n", name);
        return 0;
    }
    in->opcodeInfo = instructionInfo;
    return 1;
}
