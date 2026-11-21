#include "Instr.h"

bool func_08087860(void *, Instr *in);

static inline bool isOutput(Operand *op) { return op->unk_09 == 2; }
static inline bool isInput(Operand *op) { return op->unk_09 == 1; }

static inline RegPoolEntry *outReg(Operand *op)
{
    if (isOutput(op))
        return op->unk_18;
    return 0;
}

static inline RegPoolEntry *inReg(Operand *op)
{
    if (isInput(op))
        return op->unk_18;
    return 0;
}

class Program;
bool func_0806b4f0(Program *p)
{
    return true;
}

int hackIOmask(Instr *in, int outOp, unsigned char mask, int inOp)
{
    iInfo *info = in->unk_34;
    if (!info) {
        func_0804ad38("hackIOmask - no inst set\n");
        return -1;
    }
    Operand *out = in->getOp(outOp);
    if (!out) {
        func_0804ad38("failed to get output op\n");
        return -1;
    }
    Operand *ip = in->getOp(inOp);
    if (!ip) {
        func_0804ad38("failed to get input op\n");
        return -1;
    }
    if (!outReg(out)) {
        func_0804ad38("output op had no register!\n");
        return -1;
    }
    if (!inReg(ip)) {
        func_0804ad38("input op had no register!\n");
        return -1;
    }
    int bits = mask & out->unk_07;
    if (bits != mask) {
        func_0804ad38("out of range t_bits vs modbits for input\n");
        return -1;
    }
    if (!mask)
        return 0;
    if ((ip->unk_04 == 1 || ip->unk_04 == 9) && (out->unk_04 == 1 || out->unk_04 == 9)) {
        if (info->unk_20 == func_08087860)
            return ((bits << 1) & 0xf) | ((mask & 8) >> 3);
        return bits;
    }
    return ip->unk_07;
}
