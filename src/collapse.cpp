#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegPoolEntry *g_080de6ac;
extern mValue *g_080de6b0;
extern int g_080de6b4;
int func_0806aa54(Instr *in);
int func_08067e18(Program *p, instStream *s);

static inline bool fitsSigned(int v, int bits)
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
    for (; ol; ol = ol->unk_00) {
        Instr *in = ol->unk_04;
        int val = 0;
        int srcOp = -1;
        int immOp = -1;
        int opc = 0;
        if (!immCalc(in, &srcOp, &immOp, &val, &opc)) {
            func_0804ad38("imm_calc failed during push\n");
            return false;
        }
        int nopc = opc;
        if (opc == 0x6e && val < 0)
            nopc = 0x70;
        if (opc == 0x70 && val < 0)
            nopc = 0x6e;
        if (opc != nopc) {
            val = -val;
            in->unk_34 = func_0806bf74(nopc);
        }
        if (!in->unk_38[immOp].unk_1c.setInt(val)) {
            func_0804ad38("failed to set new immediate value\n");
            return false;
        }
    }
    return true;
}

bool func_0806a74c(Program *p)
{
    bool changed = false;
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        workTable *t = s->getTable();
        for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            if (in->func_08055ac4() != 0x6d)
                continue;
            mValue *v = &in->unk_38[2].unk_1c;
            if (v->type != mValue::STRING) {
                int val = 0;
                if (!v->getInt(&val)) {
                    func_0804ad38("failed to get immediate value for an iaddi\n");
                    return false;
                }
                int opc;
                if (val < 0) {
                    val = -val;
                    opc = 0x70;
                } else
                    opc = 0x6e;
                in->unk_34 = func_0806bf74(opc);
                if (!in->unk_34) {
                    func_0804ad38("couldn't find replacement instruction\n");
                    return false;
                }
                if (!v->setInt(val)) {
                    func_0804ad38("failed to set new val\n");
                    return false;
                }
            }
        }
    }
    do {
        changed = false;
        for (s = p->unk_a0; s; s = s->unk_108) {
            workTable *t = s->getTable();
            for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
                int k = func_0806aa54(in);
                if (k == -2) {
                    func_0804ad38("an immediate is wrong someplace\n");
                    return false;
                }
                if (k < 0)
                    continue;
                bool ok = true;
                ol_s *list = in->unk_38[k].unk_2c[0].unk_54.unk_00;
                for (ol_s *ol = list; ol && ok; ol = ol->unk_00) {
                    Instr *user = ol->unk_04;
                    Operand *op = func_0805802c(ol);
                    if (user == in) {
                        ok = false;
                        break;
                    }
                    int idx = ol->unk_10;
                    int immOp = -1;
                    int srcOp = -1;
                    int val;
                    int opc = 0;
                    int same;
                    if (!immCalc(user, &srcOp, &immOp, &val, &opc))
                        same = false;
                    else
                        same = idx == srcOp;
                    if (!same) {
                        ok = false;
                        break;
                    }
                    for (ol_s *u = op->unk_2c[0].unk_54.unk_00; u; u = u->unk_00)
                        if (u->unk_04 != in) {
                            ok = false;
                            break;
                        }
                }
                if (ok) {
                    if (!pushImmediate(list)) {
                        func_0804ad38("failed to push an immediate into a register\n");
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

int func_0806aa54(Instr *in)
{
    int opc = in->func_08055ac4();
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
        g_080de6ac = in->unk_38[1].unk_18;
        g_080de6b0 = &in->unk_38[2].unk_1c;
        break;
    case 0x70:
        bits = 15;
        idx = 0;
        isSigned = false;
        g_080de6ac = in->unk_38[1].unk_18;
        g_080de6b0 = &in->unk_38[2].unk_1c;
        neg = true;
        break;
    }
    if (idx < 0)
        return -1;
    int ret = -1;
    if (g_080de6b0->type != mValue::STRING) {
        if (!g_080de6b0->getInt(&val)) {
            func_0804ad38("failed to get immediate value\n");
            return -2;
        }
        if (val == 0)
            return -1;
        if (isSigned) {
            if (!fitsSigned(val, bits)) {
                func_0804ac7c("immediate %d doesn't fit as signed value in %d bits - skipping\n", val, bits);
                return -1;
            }
        } else if (!fitsUnsigned(val, bits)) {
            func_0804ac7c("immediate %d doesn't fit as unsigned value in %d bits - skipping\n", val, bits);
            return -1;
        }
        g_080de6b4 = neg ? -val : val;
        ret = idx;
    }
    return ret;
}

bool immCalc(Instr *in, int *srcOp, int *immOp, int *val, int *opc)
{
    *srcOp = -1;
    *immOp = -1;
    *opc = in->func_08055ac4();
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
        sub = true;
        *immOp = 2;
        break;
    default:
        return false;
    }
    mValue *v = &in->unk_38[*immOp].unk_1c;
    if (v->type != mValue::STRING) {
        int x = 0;
        if (v->getInt(&x)) {
            x = sub ? x - g_080de6b4 : x + g_080de6b4;
            bool fits;
            if (sgn)
                fits = fitsSigned(x, 11);
            else
                fits = fitsUnsigned(x < 0 ? -x : x, 15);
            if (!fits)
                return false;
            *val = x;
            return true;
        }
        func_0804ad38("failed to get immediate value\n");
    }
    return false;
}
