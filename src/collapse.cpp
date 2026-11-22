#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

RegPoolEntry *g_080de6ac;
mValue *g_080de6b0;
int g_080de6b4;
int func_0806aa54(Instr *in);
int func_08067e18(Program *p, instStream *s);

static inline int fitsSigned(int v, int bits)
{
    int r = 0;
    int hi = 1 << (bits - 1);
    int lo = -hi;
    if (v >= lo && v < hi)
        r = 1;
    return r;
}

static inline bool fitsUnsigned(int v, int bits)
{
    return v >= 0 && v < (1 << bits);
}
bool immCalc(Instr *in, int *srcOp, int *immOp, int *val, int *opc);

bool pushImmediate(ol_s *ol)
{
    for (; ol; ol = ol->next) {
        Instr *in = ol->instruction;
        int val = 0;
        int srcOp = -1;
        int immOp = -1;
        int opc = 0;
        if (!immCalc(in, &srcOp, &immOp, &val, &opc)) {
            reportError("imm_calc failed during push\n");
            return false;
        }
        int nopc = opc;
        if (opc == 0x6e && val < 0)
            nopc = 0x70;
        if (opc == 0x70 && val < 0)
            nopc = 0x6e;
        if (opc != nopc) {
            val = -val;
            in->opcodeInfo = getOpcodeInfoById(nopc);
        }
        if (!in->operands[immOp].immediateValue.setInt(val)) {
            reportError("failed to set new immediate value\n");
            return false;
        }
    }
    return true;
}

bool func_0806a74c(Program *p)
{
    bool changed = false;
    instStream *s;
    for (s = p->streamHead; s; s = s->nextStream) {
        workTable *t = s->getTable();
        for (Instr *in = t->linearInstructions.head; in; in = in->unk_1c.unk_00) {
            if (__builtin_expect(in->opcodeId() != 0x6d, 1))
                continue;
            mValue *v = &in->operands[2].immediateValue;
            if (v->type != mValue::STRING) {
                int val = 0;
                if (!v->getInt(&val)) {
                    reportError("failed to get immediate value for an iaddi\n");
                    return false;
                }
                int opc;
                if (val < 0) {
                    val = -val;
                    opc = 0x70;
                } else
                    opc = 0x6e;
                in->opcodeInfo = getOpcodeInfoById(opc);
                if (!in->opcodeInfo) {
                    reportError("couldn't find replacement instruction\n");
                    return false;
                }
                if (!v->setInt(val)) {
                    reportError("failed to set new val\n");
                    return false;
                }
            }
        }
    }
    do {
        changed = false;
        for (s = p->streamHead; s; s = s->nextStream) {
            workTable *t = s->getTable();
            for (Instr *in = t->linearInstructions.head; in; in = in->unk_1c.unk_00) {
                int k = func_0806aa54(in);
                if (k == -2) {
                    reportError("an immediate is wrong someplace\n");
                    return false;
                }
                if (k < 0)
                    continue;
                bool ok = true;
                ol_s *list = in->operands[k].parts[0].unk_54.head;
                for (ol_s *ol = list; ol && ok; ol = ol->next) {
                    Instr *user = ol->instruction;
                    Operand *op = operandAtLocation(ol);
                    if (user == in) {
                        ok = false;
                        break;
                    }
                    int idx = ol->operandIndex;
                    int immOp = -1;
                    int srcOp = -1;
                    int val;
                    int opc = 0;
                    int same;
                    if (!immCalc(user, &srcOp, &immOp, &val, &opc))
                        same = false;
                    else
                        same = idx == srcOp;
                    if (!(char)same) {
                        ok = false;
                        break;
                    }
                    for (ol_s *u = op->parts[0].unk_54.head; u; u = u->next)
                        if (u->instruction != in) {
                            ok = false;
                            break;
                        }
                }
                if (ok) {
                    if (!pushImmediate(list)) {
                        reportError("failed to push an immediate into a register\n");
                        return false;
                    }
                    g_080de6b0->setInt(0);
                    changed = true;
                }
            }
        }
    } while (changed);
    return true;
}

static inline bool isStringValue(mValue *v)
{
    return v->type == mValue::STRING;
}

int func_0806aa54(Instr *in)
{
    int opc = in->opcodeId();
    int bits = 0;
    int idx = -1;
    bool neg = false;
    bool isSigned = false;
    int val = 0;
    g_080de6b0 = 0;
    g_080de6ac = 0;
    switch (opc) {
    case 0x6e:
        bits = 15;
        idx = 0;
        isSigned = false;
        g_080de6ac = in->operands[1].assignedRegister;
        g_080de6b0 = &in->operands[2].immediateValue;
        break;
    case 0x70:
        bits = 15;
        isSigned = false;
        g_080de6ac = in->operands[1].assignedRegister;
        g_080de6b0 = &in->operands[2].immediateValue;
        idx = 0;
        neg = true;
        break;
    }
    if (idx < 0)
        return -1;
    if (isStringValue(g_080de6b0))
        return -1;
    if (!g_080de6b0->getInt(&val)) {
        reportError("failed to get immediate value\n");
        return -2;
    }
    if (val == 0)
        return -1;
    if (isSigned) {
        if (!(char)fitsSigned(val, bits)) {
            reportWarning("immediate %d doesn't fit as signed value in %d bits - skipping\n", val, bits);
            return -1;
        }
    } else if (!fitsUnsigned(val, bits)) {
        reportWarning("immediate %d doesn't fit as unsigned value in %d bits - skipping\n", val, bits);
        return -1;
    }
    g_080de6b4 = neg ? -val : val;
    return idx;
}

static inline int immFitsUnsigned(int v, int bits)
{
    int r = 0;
    if (v >= 0 && v < (1 << bits))
        r = 1;
    return r;
}

bool immCalc(Instr *in, int *srcOp, int *immOp, int *val, int *opc)
{
    *srcOp = -1;
    *immOp = -1;
    *opc = in->opcodeId();
    bool sgn = false;
    bool sub = false;
    switch (*opc) {
    case 0x9b:
    case 0x9c:
    case 0xa0:
    case 0xa1:
        *srcOp = 1;
        sgn = true;
        *immOp = 1;
        break;
    case 0x6e:
        *srcOp = 1;
        *immOp = 2;
        break;
    case 0x70:
        *srcOp = 1;
        *immOp = 2;
        sub = true;
        break;
    default:
        return false;
    }
    mValue *v = &in->operands[*immOp].immediateValue;
    if (isStringValue(v))
        return false;
    int x = 0;
    if (!v->getInt(&x)) {
        reportError("failed to get immediate value\n");
        return false;
    }
    x = sub ? x - g_080de6b4 : x + g_080de6b4;
    if (sgn) {
        if (!(char)fitsSigned(x, 11))
            return false;
    } else if (!(char)immFitsUnsigned(x < 0 ? -x : x, 15)) {
        return false;
    }
    *val = x;
    return true;
}
