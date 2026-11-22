#include "os.h"
#include "Instr.h"

bool rotateInputComponentMask(void *, Instr *in);

static inline bool isOutput(Operand *operand) { return operand->direction == VCL_OPERAND_WRITE; }
static inline bool isInput(Operand *operand) { return operand->direction == VCL_OPERAND_READ; }

static inline RegisterEntry *outReg(Operand *operand)
{
    if (isOutput(operand))
        return operand->assignedRegister;
    return 0;
}

static inline RegisterEntry *inReg(Operand *operand)
{
    if (isInput(operand))
        return operand->assignedRegister;
    return 0;
}

class Program;
bool noopIOmaskProgramHook(Program *program)
{
    return true;
}

int hackIOmask(Instr *instruction, int outOp, unsigned char mask, int inOp)
{
    OpcodeDescriptor *opcodeInfo = instruction->opcodeInfo;
    if (!opcodeInfo) {
        reportError("hackIOmask - no inst set\n");
        return -1;
    }
    Operand *out = instruction->getOp(outOp);
    if (!out) {
        reportError("failed to get output op\n");
        return -1;
    }
    Operand *operand = instruction->getOp(inOp);
    if (!operand) {
        reportError("failed to get input op\n");
        return -1;
    }
    if (!outReg(out)) {
        reportError("output op had no register!\n");
        return -1;
    }
    if (!inReg(operand)) {
        reportError("input op had no register!\n");
        return -1;
    }
    int bits = mask & out->componentMask;
    if (bits != mask) {
        reportError("out of range t_bits vs modbits for input\n");
        return -1;
    }
    if (!mask)
        return 0;
    if ((operand->operandKind == VCL_OP_V_FL4 || operand->operandKind == VCL_OP_V_ACC) && (out->operandKind == VCL_OP_V_FL4 || out->operandKind == VCL_OP_V_ACC)) {
        if (opcodeInfo->decodeHook == rotateInputComponentMask)
            return ((bits << 1) & 0xf) | ((mask & 8) >> 3);
        return bits;
    }
    return operand->componentMask;
}
