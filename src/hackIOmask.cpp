#include "os.h"
#include "Instr.h"

bool rotateInputComponentMask(void *, Instr *in);

static inline bool isOutput(Operand *op) { return op->direction == 2; }
static inline bool isInput(Operand *op) { return op->direction == 1; }

static inline RegPoolEntry *outReg(Operand *op)
{
    if (isOutput(op))
        return op->assignedRegister;
    return 0;
}

static inline RegPoolEntry *inReg(Operand *op)
{
    if (isInput(op))
        return op->assignedRegister;
    return 0;
}

class Program;
bool noopIOmaskProgramHook(Program *p)
{
    return true;
}

int hackIOmask(Instr *in, int outOp, unsigned char mask, int inOp)
{
    iInfo *info = in->opcodeInfo;
    if (!info) {
        reportError("hackIOmask - no inst set\n");
        return -1;
    }
    Operand *out = in->getOp(outOp);
    if (!out) {
        reportError("failed to get output op\n");
        return -1;
    }
    Operand *ip = in->getOp(inOp);
    if (!ip) {
        reportError("failed to get input op\n");
        return -1;
    }
    if (!outReg(out)) {
        reportError("output op had no register!\n");
        return -1;
    }
    if (!inReg(ip)) {
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
    if ((ip->operandKind == 1 || ip->operandKind == 9) && (out->operandKind == 1 || out->operandKind == 9)) {
        if (info->decodeHook == rotateInputComponentMask)
            return ((bits << 1) & 0xf) | ((mask & 8) >> 3);
        return bits;
    }
    return ip->componentMask;
}
