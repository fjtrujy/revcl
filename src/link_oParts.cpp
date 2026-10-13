#include "Instr.h"
#include <string.h>
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

char *dupString(const char *s);

extern int errorOccurred;
extern bool quiet_080bbd98;

bool Instr::changeBranchImmediate(const char *s)
{
    if (!unk_1388 || !unk_34) {
        func_0804ad38("changeBranchImmediate: not a branch\n");
        return false;
    }
    if (unk_1388 & 2) {
        func_0804ad38("changeBranchImmediate: target is register!\n");
        return false;
    }
    int n = unk_34->unk_1a;
    if (!n) {
        func_0804ad38("getBranchTargetImmediate - branch target operand not set\n");
        func_0804adf0(-1);
    }
    Operand *op = &unk_38[n - 1];
    if (op->unk_04 != 0x1b) {
        func_0804ad38("changeBranchImmediate::target operand isn't immediate!\n");
        func_0804adf0(-1);
    }
    return op->unk_1c.setString(s);
}

bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot)
{
    if (!a || !b) {
        func_0804ad38("link_oParts, null arg pointer!\n");
        return false;
    }
    if (bSlot != slot) {
        func_0804ad38("swizzle detected for ps2 ?\n");
        return false;
    }
    OpSlot *sa = &a->unk_38[aOp].unk_2c[slot];
    OpSlot *sb = &b->unk_38[bOp].unk_2c[slot];
    ol_s *oa = sa->unk_54.func_080580fc(b, bOp, slot, false);
    if (oa) {
        olList *lb = &sb->unk_54;
        ol_s *ob = lb->func_080580fc(a, aOp, slot, false);
        if (!ob) {
            func_0804ad38("failed to add ol\n");
            return false;
        }
        return true;
    }
    func_0804ad38("failed to add ol\n");
    return false;
}

static inline void dumpLinkOperand(Operand *op)
{
    op->func_0805665c(0);
}

bool link_oParts(ol_s *o, ol_s *i, int x, int y)
{
    if (!o) {
        func_0804ad38("link_oParts, null arg pointer2!\n");
        return false;
    }
    if (!i) {
        func_0804ad38("link_oParts, null arg pointer2!\n");
        return false;
    }
    Operand *out = func_0805802c(o);
    if (!out) {
        func_0804ad38("link_oParts(2) - output not set \n");
        return false;
    }
    if (!isRegisterOperand(out, 2)) {
        func_0804ad38("link_oParts - output was not output\n");
        dumpLinkOperand(out);
        return false;
    }
    Operand *in = func_0805802c(i);
    if (!in) {
        func_0804ad38("link_oParts(2) - input not set \n");
        return false;
    }
    if (!isRegisterOperand(in, 1)) {
        func_0804ad38("link_oParts - input was not input\n");
        return false;
    }
    int to = o->unk_11;
    int ti = i->unk_11;
    if (!((out->unk_07 >> to) & 1)) {
        func_0804ad38("link_oParts - mod bit not set for output\n");
        return false;
    }
    if (!((in->unk_07 >> ti) & 1)) {
        func_0804ad38("link_oParts - mod bit not set for input\n");
        return false;
    }
    if (!((in->unk_27 >> ti) & 1)) {
        func_0804ad38("link_oParts_ext - external bit not set for input\n");
        return false;
    }
    out->unk_27 |= 1 << to;
    if (func_08058618(o) != func_08058618(i)) {
        out->unk_06 |= 1 << o->unk_11;
        in->unk_06 |= 1 << i->unk_11;
    }
    if (to != ti) {
        func_0804ad38("link_oParts - not a bug, but should not happen for this cpu, t_o!=t_i (%d!=%d)\n", to, ti);
        return false;
    }
    OpSlot *so = out->unk_2c + ti;
    OpSlot *si = in->unk_2c + ti;
    olList *lo = &so->unk_54;
    ol_s *a = lo->func_08058348(i, true);
    if (!a) {
        func_0804ad38("failed to add ol_input to output\n");
        return false;
    }
    ol_s *b = si->unk_54.func_08058348(o, true);
    if (!b) {
        func_0804ad38("failed to add ol_ouput to input\n");
        return false;
    }
    int s = o->unk_12;
    if (s > 0xff)
        s = 0xff;
    a->unk_12 = s;
    a->unk_08 = x;
    a->unk_0c = y;
    b->unk_12 = 0;
    b->unk_08 = x;
    b->unk_0c = y;
    return true;
}

bool Instr::setOwner(cls_unk30 *s)
{
    if (unk_30 && unk_30 != s) {
        func_0804ad38("in->setOwner - already owned..but by wrong stream!\n");
        func_0804adf0(-1);
    }
    unk_30 = s;
    return true;
}

const char *func_08055254(int t)
{
    switch (t) {
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

char *Instr::func_080552a8(bool flag, char *buf, int)
{
    if (!buf)
        return 0;
    iInfo *info = unk_34;
    if (info) {
        buf += func_0804adbc(buf, info->unk_04);
        switch ((char)info->unk_0c) {
        case 1:
            buf += func_0804adbc(buf, ".[xyzw]");
            break;
        case 2:
            buf += func_0804adbc(buf, ".xyz");
            break;
        case 3:
            buf += func_0804adbc(buf, ".[x/y/z/w]");
            break;
        }
    } else {
        buf += func_0804adbc(buf, "nop");
    }
    buf += func_0804adbc(buf, "\t");
    for (int n = 0; n != unk_138b; n++) {
        if (unk_38[n].unk_08)
            break;
        if (n)
            buf += func_0804adbc(buf, ",");
        buf = unk_38[n].func_08057b90(flag, buf);
        if (!buf)
            break;
    }
    return buf;
}

int Instr::func_080553b4(RegPoolEntry *reg, int mask)
{
    int result = 0;
    if (reg) {
        for (int i = 0; i != unk_138b; i++) {
            Operand *op = &unk_38[i];
            if (op->outRegZ() == reg)
                result |= op->unk_07;
        }
    }
    return result & mask;
}

int Instr::func_0805544c(RegPoolEntry *reg, int mask)
{
    int result = 0;
    if (reg) {
        for (int i = 0; i != unk_138b; i++) {
            Operand *op = &unk_38[i];
            if (op->inRegOp2() == reg)
                result |= op->unk_07;
        }
    }
    return result & mask;
}

const char *func_080554e4(int t)
{
    const char *s = "SYNTH_UNKNOWN";
    switch (t) {
    case 0: s = "SYNTH_NULL"; break;
    case 1: s = "SYNTH_ENTER"; break;
    case 2: s = "SYNTH_ENDENTER"; break;
    case 3: s = "SYNTH_EXIT"; break;
    case 4: s = "SYNTH_ENDEXIT"; break;
    case 5: s = "SYNTH_CONT"; break;
    case 6: s = "SYNTH_XINPUT"; break;
    case 7: s = "SYNTH_XOUTPUT"; break;
    case 8: s = "SYNTH_LOOP_MINSAFE"; break;
    case 9: s = "SYNTH_LOOP_EXTRA"; break;
    case 10: s = "SYNTH_BARRIER"; break;
    }
    return s;
}

bool Instr::func_08055548()
{
    if (unk_00)
        return true;
    if (unk_30)
        return unk_30->unk_09 == 3;
    return false;
}

char *Instr::getBranchTargetImmediate()
{
    iInfo *ii = unk_34;
    if (!unk_1388)
        return 0;
    if (!ii) {
        func_0804ad38("branch type set - but no ii\n");
        func_0804adf0(-1);
    }
    int n = ii->unk_1a;
    if (!n) {
        func_0804ad38("getBranchTargetImmediate - branch target operand not set\n");
        func_0804adf0(-1);
    }
    Operand *op = &unk_38[n - 1];
    if (!op->isInput()) {
        func_0804ad38("alleged branch target immediate wasn't an input\n");
        func_0804adf0(-1);
    }
    if (op->unk_04 == 0x1b)
        return op->unk_1c.getString();
    return 0;
}

RegPoolEntry *Instr::getBranchTargetRegister()
{
    iInfo *ii = unk_34;
    if (!(unk_1388 & 2))
        return 0;
    if (!ii) {
        func_0804ad38("branch type set - but no ii\n");
        func_0804adf0(-1);
    }
    int n = ii->unk_1a;
    if (!n) {
        func_0804ad38("getBranchTargetRegister - branch target operand not set\n");
        func_0804adf0(-1);
    }
    Operand *op = &unk_38[n - 1];
    if (!op->isInput()) {
        func_0804ad38("alleged branch target register wasn't an input\n");
        func_0804adf0(-1);
    }
    if (op->unk_04 == 0x1b)
        return 0;
    RegPoolEntry *r = op->unk_18;
    if (!r) {
        func_0804ad38("branch target register wasn't set\n");
        func_0804adf0(-1);
    }
    return r;
}

int Instr::func_08055718()
{
    return 0;
}

Operand *Instr::func_08055720(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (op->inRegOp2() == reg && (op->unk_07 & mask))
            return op;
    }
    return 0;
}

Operand *Instr::func_080557a4(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (op->outRegZ() == reg && (op->unk_07 & mask))
            return op;
    }
    return 0;
}

bool Instr::func_08055828(unsigned int n)
{
    if (!unk_34)
        return false;
    for (; n < unk_138b; n++) {
        Operand *op = &unk_38[n];
        RegPoolEntry *r = registerOperand(op, 2);
        if (r) {
            if (op->unk_27)
                return true;
            for (int j = 0; j != r->unk_28; j++)
                if (((op->unk_07 >> j) & 1) && op->unk_2c[j].unk_54.unk_04)
                    return true;
        }
    }
    return false;
}

bool Instr::func_080558f4()
{
    if (unk_13ce == 2) {
        unk_138e = 1;
        return true;
    }
    if (unk_13d0 & 0x40) {
        unk_138e = 2;
        return true;
    }
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (isRegisterOperand(op, 2) && op->unk_06) {
            unk_138e = 2;
            return true;
        }
    }
    unk_138e = 0;
    return true;
}

Instr **Instr::getIE()
{
    cls_unk30 *table = unk_30;
    if (!table) {
        func_0804ad38("failed to get itable from instruction\n");
        return 0;
    }
    cls08058718 *pos = &unk_13e8;
    Instr **ie = table->getIE_ck(pos->unk_00);
    if (!ie) {
        func_0804ad38("failed to get ie from table for in::getIE()\n");
        return 0;
    }
    if (ie[pos->unk_02] != this) {
        func_0804ad38("we aren't where we thought we were for in::getIE\n");
        return 0;
    }
    return ie;
}

int Instr::func_08055a0c()
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (op->outRegZ() && !op->unk_0b)
            return op->unk_0a;
    }
    return 0;
}

bool Instr::func_08055a88()
{
    bool r = !unk_0e_1;
    unk_0e_1 = 1;
    return r;
}

bool Instr::func_08055aa8()
{
    bool r = !unk_0e_0;
    unk_0e_0 = 1;
    return r;
}

int Instr::func_08055ac4()
{
    if (!unk_34)
        return -1;
    return func_0806c72c(unk_34);
}

static inline bool isLiteral(mValue *v)
{
    return v->type == mValue::STRING;
}

bool Instr::func_08055ae4()
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (op->unk_04 < 22 || op->unk_04 > 25)
            continue;
        if (i != unk_138b - 1) {
            func_0804ad38("virtual stat operand not final\n");
            return false;
        }
        unk_138b--;
        switch (op->unk_04) {
        case 23:
            op->unk_08 = 1;
            op->unk_04 = 21;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(55);
            op->unk_07 = 1;
            op->unk_0a = 4;
            op->unk_0b = 0;
            op->unk_26 = unk_138b++;
            op = &unk_38[i + 1];
            op->unk_08 = 1;
            op->unk_04 = 18;
            op->unk_09 = 1;
            op->unk_14 = op->unk_18 = func_0806c224(57);
            op->unk_07 = 1;
            op->unk_0a = -1;
            op->unk_0b = 1;
            op->unk_26 = unk_138b++;
            break;
        case 22:
            op->unk_08 = 1;
            op->unk_04 = 20;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(56);
            op->unk_07 = 1;
            op->unk_0a = unk_38[0].unk_0a;
            op->unk_0b = 0;
            op->unk_26 = unk_138b;
            op = &unk_38[i + 1];
            unk_138b++;
            op->unk_08 = 1;
            op->unk_04 = 19;
            op->unk_09 = 1;
            op->unk_14 = op->unk_18 = func_0806c224(58);
            op->unk_07 = 1;
            op->unk_0a = -1;
            op->unk_0b = 1;
            op->unk_26 = unk_138b++;
            break;
        case 24: {
            if (unk_38[i - 1].unk_04 != 27) {
                func_0804ad38("not immediate predecessor in fs test\n");
                return false;
            }
            bool a = true;
            bool b = true;
            bool c = true;
            bool d = true;
            mValue *v = &unk_38[i - 1].unk_1c;
            unsigned int imm;
            if (isLiteral(v)) {
                func_0804ac7c("fsop has literals - so assuming the worst - if it were numerical vcl could figure out dependencies better\n");
            } else if (v->getUInt(&imm)) {
                unsigned int bits;
                int id = -1;
                if (unk_34)
                    id = func_0806c72c(unk_34);
                switch (id) {
                case 147:
                    break;
                case 148:
                    bits = ~imm;
                    goto test;
                case 146:
                    bits = imm;
                test:
                    a = (bits & 0xf) != 0;
                    b = (bits & 0x30) != 0;
                    c = (bits & 0xc00) != 0;
                    d = (bits & 0x3c0) != 0;
                    break;
                default:
                    func_0804ad38("bad fsop \n");
                    return false;
                }
            } else {
                func_0804ad38("failed to get immediate value\n");
                return false;
            }
            a = a | c;
            b = b | d;
            if (a) {
                op->unk_08 = 1;
                op->unk_04 = 21;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(55);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 0;
                op->unk_26 = unk_138b;
                op++;
                unk_138b++;
            }
            if (b) {
                op->unk_08 = 1;
                op->unk_04 = 20;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(56);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 0;
                op->unk_26 = unk_138b;
                op++;
                unk_138b++;
            }
            if (c) {
                op->unk_08 = 1;
                op->unk_04 = 18;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(57);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b;
                op++;
                unk_138b++;
                op->unk_08 = 1;
                op->unk_04 = 18;
                op->unk_09 = 2;
                op->unk_14 = op->unk_18 = func_0806c224(57);
                op->unk_07 = 1;
                op->unk_0a = 1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b;
                op++;
                unk_138b++;
            }
            if (d) {
                op->unk_08 = 1;
                op->unk_04 = 19;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(58);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b;
                op++;
                unk_138b++;
                op->unk_08 = 1;
                op->unk_04 = 19;
                op->unk_09 = 2;
                op->unk_14 = op->unk_18 = func_0806c224(58);
                op->unk_07 = 1;
                op->unk_0a = 1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b++;
            }
            break;
        }
        case 25:
            op->unk_08 = 1;
            op->unk_04 = 18;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(57);
            op->unk_07 = 1;
            op->unk_0a = 1;
            op->unk_0b = 1;
            op->unk_26 = unk_138b;
            op = &unk_38[i + 1];
            unk_138b++;
            op->unk_08 = 1;
            op->unk_04 = 19;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(58);
            op->unk_07 = 1;
            op->unk_0a = 1;
            op->unk_0b = 1;
            op->unk_26 = unk_138b++;
            break;
        }
    }
    return true;
}

void Instr::func_08055f4c()
{
    unk_13b0 = 0;
    unk_13b2 = 0;
    unk_13b4 = 0;
    unk_13b6 = 0;
}

void Instr::func_08055f78(Instr *src)
{
    unk_13b0 = src->unk_13b0;
    unk_13b2 = src->unk_13b2;
    unk_13b4 = src->unk_13b4;
    unk_13b6 = src->unk_13b6;
}

void Instr::func_08055fbc(short v)
{
    unk_13b0 = unk_13b0 < v ? v : unk_13b0;
}

void Instr::func_08055fdc(short v)
{
    unk_13b2 = unk_13b2 < v ? v : unk_13b2;
}

void Instr::func_08055ffc(short v)
{
    unk_13b4 = unk_13b4 < v ? v : unk_13b4;
}

void Instr::func_0805601c(short v)
{
    unk_13b6 = unk_13b6 < v ? v : unk_13b6;
}

OpSlot::OpSlot()
{
}

OpSlot::~OpSlot()
{
}

void OpSlot::func_080560bc()
{
}

bool OpSlot::func_080560c4()
{
    func_0805bcc4();
    unk_50 = 0;
    unk_52[0] = 0;
    return true;
}

int OpSlot::func_080560e8()
{
    return unk_54.clear();
}
