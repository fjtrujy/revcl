// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

char *dupString(const char *s);

extern int errorOccurred;
extern bool quiet_080bbd98;

opPair_s opPairTable_080b32e0[] = {
    {109, {1, 1, 0, 2}, 0},
    {110, {1, 1, 0, 2}, 0},
    {112, {2, 1, 0, 2}, 0},
    {158, {1, 2, 1, -1}, 1},
    {163, {1, 2, 1, -1}, 1},
    {159, {2, 2, 1, -1}, 1},
    {164, {2, 2, 1, -1}, 1},
    {0, {0}, 0},
};

opCount_s opCountTable_080b3340[] = {
    {155, 1}, {160, 1}, {156, 1}, {161, 1}, {0, 0},
};

Instr::Instr(iInfo *info)
{
    if (!info) {
        func_0804ad38("constructor called with null iInfo -... later will make this a NOP, but , for now..halt!\n");
        func_0804adf0(-1);
    }
    unk_138b = 0;
    unk_13d1 = 0;
    unk_1389 = 0;
    unk_138f = 0;
    unk_1434 = 0;
    unk_14 = 0;
    unk_18 = 0;
    unk_1a = 0;
    unk_1430 = 0;
    unk_1432 = 0;
    unk_142c = 0;
    unk_142e = 0;
    unk_1420 = -1;
    unk_139c = 0;
    unk_1398 = 0;
    unk_30 = 0;
    unk_1428 = 0;
    unk_13b0 = 0;
    unk_13b2 = 0;
    unk_13b4 = 0;
    unk_13b6 = 0;
    unk_13e4 = -1;
    unk_13e0 = -1;
    unk_1418 = 0;
    unk_141a = 0;
    unk_34 = info;
    unk_141e = 0;
    unk_08 = 0;
    unk_00 = 0;
    unk_04 = 0;
    unk_1391 = 0;
    unk_1392 = 0;
    unk_1394 = -1;
    unk_141c = 0;
    unk_13d2 = -1;
    unk_138e = -1;
    resetImmFixup();
    unk_34 = info;
    unk_13d7 = info->unk_0e;
    unk_138c = info->unk_0e;
    unk_138d = info->unk_10;
    unk_1388 = info->func_0806ffb4();
    unk_13cf = info->unk_12;
    unk_13ce = info->ramType();
    unk_13d0 = info->unk_18 & ~7;
    unk_138a = info->func_08070044();
    unk_13cd = info->func_08070020();
    for (int i = 0; info->unk_44[i].unk_00; i++) {
        unk_138b++;
        Operand *op = &unk_38[i];
        iOpDesc *d = &info->unk_44[i];
        op->unk_04 = d->unk_00;
        op->unk_26 = i;
        op->unk_0b = d->unk_04 != 0;
        op->unk_09 = d->unk_01;
        op->unk_08 = d->unk_02 != 0;
        op->unk_05 = d->unk_05;
        op->unk_0d = d->unk_06;
        if (op->unk_0d)
            unk_38[op->unk_0d - 1].unk_0d = i + 1;
        if (d->unk_01 == 2)
            op->unk_0a = d->unk_03;
        else
            op->unk_0a = 0;
    }
}

Instr::~Instr()
{
    if (unk_139c) {
        delete[] unk_139c;
        unk_139c = 0;
    }
    unk_13d8.clear();
    unk_13a0.clear();
    unk_13a8.clear();
}

bool Instr::func_08053f4c()
{
    if (unk_139c) {
        delete[] unk_139c;
        unk_139c = 0;
    }
    unk_13d8.clear();
    unk_13a0.clear();
    unk_13a8.clear();
    return true;
}

bool Instr::func_08053fac()
{
    if (!unk_34)
        return false;
    for (int i = 0; i != unk_138b; i++)
        if (!unk_38[i].func_080563f8())
            return false;
    return true;
}

void Instr::func_08054008(bool b)
{
    if (isValid()) {
        for (int i = 0; i != unk_138b; i++)
            unk_38[i].func_0805665c(0);
        if (b) {
            for (int j = 0; j != unk_138b; j++) {
                Operand *op = &unk_38[j];
                RegPoolEntry *r = op->unk_18;
                if (r && r->unk_14)
                    for (int k = 0; k != r->unk_28; k++)
                        if ((op->unk_07 >> k) & 1)
                            op->unk_2c[k].unk_54.func_08057e9c();
            }
        }
    }
}

bool Instr::func_08054138()
{
    if (!unk_34)
        return true;
    return unk_34->func_0807005c();
}

instStream *Instr::func_08054168()
{
    return unk_30 ? unk_30->unk_10 : 0;
}

bool Instr::func_0805417c()
{
    unk_13a0.clear();
    unk_13a8.clear();
    unk_1394 = -1;
    for (int i = 0; i != unk_138b; i++)
        if (!unk_38[i].func_080569cc())
            return false;
    unk_1434 = 0;
    return true;
}

bool Instr::func_080541fc()
{
    return !unk_13cd;
}

bool Instr::func_08054214()
{
    unk_1418 = 0;
    unk_141c = 0;
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        for (int k = 0; k != 4; k++)
            op->unk_2c[k].unk_50 = 0;
    }
    return true;
}

void Instr::leDump()
{
    leDump(-1, -1);
}

void Instr::leDump(int opnd)
{
    leDump(opnd, -1);
}

void Instr::leDump(int opnd, int mask)
{
    if (opnd >= unk_138b) {
        func_0804ad38("Instruction::leDump - out of bounds operand!\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    if (unk_14) {
        int idx = unk_18;
        if (opnd >= 0) {
            if (unk_38[opnd].unk_18 && unk_38[opnd].unk_18->unk_28 != 1 && mask >= 0)
                func_0804ac50("(field(s) : %s) ", func_08052064(mask));
            if (unk_38[opnd].unk_08) {
                if (unk_38[opnd].unk_18)
                    func_0804ac50("implicit operand '%s'\n", unk_38[opnd].unk_18->func_0805a8e0());
                else
                    func_0804ad38("implicit operand with no register??\n");
            } else
                idx = unk_38[opnd].unk_24;
        }
        unk_14->func_08050c40(idx);
    } else {
        func_0804ac50("operand %d modbits %x..\n", opnd, mask);
        func_08054008(false);
    }
    errorOccurred = saved;
}

bool func_080543bc(Instr *a, Instr *b)
{
    if (!a || !b)
        return false;
    for (int i = 0; i != b->unk_138b; i++) {
        Operand *op = &b->unk_38[i];
        RegPoolEntry *r = op->regOp();
        if (r) {
            int n = r->unk_28;
            for (int k = 0; k != n; k++)
                if (((op->unk_07 & ~op->unk_27) >> k) & 1)
                    for (ol_s *p = op->unk_2c[k].unk_54.unk_00; p; p = p->unk_00)
                        if (p->unk_04 == a)
                            return true;
        }
    }
    return false;
}

bool func_0805449c(Instr *a, Instr *b)
{
    if (!a || !b)
        return false;
    for (int i = 0; i != b->unk_138b; i++) {
        Operand *op = &b->unk_38[i];
        RegPoolEntry *r = op->getReg();
        if (r) {
            int want = op->unk_07;
            int mask = 0;
            int n = a->unk_138b;
            for (int j = 0; j != n; j++)
                if (a->unk_38[j].getReg2() == r)
                    mask |= a->unk_38[j].unk_07;
            if (want & mask)
                return true;
        }
    }
    return false;
}

Instr *Instr::clone()
{
    if (!this)
        return 0;
    iInfo *info = unk_34;
    if (!info)
        return 0;
    Instr *c = new Instr(info);
    if (!c) {
        func_0804ad38("failed to get a new instruction during clone!\n");
        func_0804adf0(-1);
    }
    if (!c->func_08054b84()) {
        func_0804ad38("failed to opt reset cloned instruction\n");
        func_0804adf0(-1);
    }
    c->unk_138f = unk_138f;
    c->unk_138a = unk_138a;
    c->unk_1428 = 0;
    c->unk_1389 = unk_1389;
    c->unk_142c = unk_142c;
    c->unk_142e = unk_142e;
    c->unk_13d7 = unk_13d7;
    c->unk_13f0.func_0805e37c(&unk_13f0);
    c->unk_13e4 = unk_13e4;
    c->unk_13e0 = unk_13e0;
    c->unk_141a = unk_141a;
    c->unk_1420 = unk_1420;
    c->unk_1392 = unk_1392;
    c->unk_1391 = unk_1391;
    c->unk_14 = unk_14;
    c->unk_18 = unk_18;
    c->unk_1a = unk_1a;
    c->unk_13d4 = unk_13d4;
    c->unk_13d5 = unk_13d5;
    c->unk_13d6 = unk_13d6;
    if (unk_13d8.isValid())
        c->unk_13d8.set(&unk_13d8);
    c->unk_13d2 = unk_13d2;
    if (unk_08) {
        func_0804ad38("during instruction clone - tmpClone was already set!\n");
        delete c;
        return 0;
    }
    unk_08 = c;
    c->unk_08 = this;
    if (unk_139c)
        c->unk_139c = dupString(unk_139c);
    c->unk_138b = unk_138b;
    Operand *s;
    for (int i = 0; (s = getOp(i)) != 0; i++) {
        Operand *d = c->getOp(i);
        if (d == 0) {
            func_0804ad38("fatal error, couldn't get operand during clone\n");
            func_0804adf0(-1);
        }
        if (s->unk_26 != i) {
            func_0804ad38("bad index in operand for clone\n");
            func_0804adf0(-1);
        }
        d->unk_26 = s->unk_26;
        d->unk_04 = s->unk_04;
        d->unk_09 = s->unk_09;
        d->unk_0a = s->unk_0a;
        d->unk_05 = s->unk_05;
        d->unk_0b = s->unk_0b;
        d->unk_18 = s->unk_18;
        d->unk_07 = s->unk_07;
        d->unk_0c = s->unk_0c;
        d->unk_14 = s->unk_14;
        d->unk_0d = s->unk_0d;
        d->unk_08 = s->unk_08;
        d->unk_10 = s->unk_10;
        if (s->unk_1c.isValid()) {
            if (!d->unk_1c.set(&s->unk_1c)) {
                func_0804ad38("failed to clone immediate\n");
                func_0804adf0(-1);
            }
        }
        d->unk_24 = s->unk_24;
        d->unk_25 = s->unk_25;
        d->unk_29 = s->unk_29;
        d->unk_27 = s->unk_27;
        d->unk_2b = 0;
        d->unk_06 = s->unk_06;
        d->unk_28 = s->unk_28;
    }
    return c;
}

int Instr::getRamBits()
{
    if (!unk_13ce)
        return 0;
    int r = 0;
    if (unk_34) {
        switch (unk_34->unk_0c) {
        case 1:
            r = unk_138f;
            break;
        default:
            func_0804ad38("got here! - end of get rambits?\n");
            func_08054008(false);
            func_0804adf0(-1);
            return 0;
        case 3: {
            Operand *op = getOp(1);
            if (!op) {
                func_0804ad38("failed to get op1 for mod_1 ram test\n");
                func_0804adf0(-1);
            }
            if (!op->unk_0c)
                func_0804ac7c("mb was 0 failed to get xmod bits for mod 1 inst\n");
            r = op->unk_0c;
            break;
        }
        case 0:
        case 4:
        case 5:
            r = 15;
            break;
        }
    }
    return r;
}

bool Instr::func_08054b84()
{
    unk_13e8.func_080586e0();
    unk_2c = 0;
    unk_24.unk_04 = 0;
    unk_24.unk_00 = 0;
    unk_1418 = 0;
    unk_1398 = 0;
    unk_141c = 0;
    for (int i = 0; i != unk_138b; i++) {
        if (!unk_38[i].func_08056a24()) {
            func_0804ad38("failed to optReset operand  %d\n", i);
            return false;
        }
    }
    return true;
}

int Instr::func_08054c24(RegPoolEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *op;
    for (int i = 0; (op = getOp(i)) != 0; i++) {
        RegPoolEntry *r;
        if (op->unk_09 == 2)
            r = op->unk_18;
        else
            r = 0;
        if (reg == r) {
            *paired = *paired || op->unk_0d;
            mask |= op->unk_07;
        }
    }
    return mask;
}

int Instr::func_08054cc4(RegPoolEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *op;
    for (int i = 0; (op = getOp(i)) != 0; i++) {
        RegPoolEntry *r;
        if (op->unk_09 == 1)
            r = op->unk_18;
        else
            r = 0;
        if (reg == r) {
            *paired = *paired || op->unk_0d;
            mask |= op->unk_07;
        }
    }
    return mask;
}

bool Instr::func_08054d64(int n, int *out)
{
    Operand *op = getOp(n);
    if (!op) {
        func_0804ad38("failed to get operand %d\n", n);
        return false;
    }
    return op->unk_1c.getInt(out);
}

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
    olList *la = &a->unk_38[aOp].unk_2c[slot].unk_54;
    olList *lb = &b->unk_38[bOp].unk_2c[slot].unk_54;
    if (!la->func_080580fc(b, bOp, slot, false) || !lb->func_080580fc(a, aOp, slot, false)) {
        func_0804ad38("failed to add ol\n");
        return false;
    }
    return true;
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
    if (!out->isOutRegOp()) {
        func_0804ad38("link_oParts - output was not output\n");
        out->func_0805665c(0);
        return false;
    }
    Operand *in = func_0805802c(i);
    if (!in) {
        func_0804ad38("link_oParts(2) - input not set \n");
        return false;
    }
    if (!in->isInRegOp()) {
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
    ol_s *a = so->unk_54.func_08058348(i, true);
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
            if (op->getReg2() == reg)
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
            if (op->getReg() == reg)
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
    if (op->unk_09 != 1) {
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
    if (op->unk_09 != 1) {
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
        if (op->getReg() == reg && (op->unk_07 & mask))
            return op;
    }
    return 0;
}

Operand *Instr::func_080557a4(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (op->getReg2() == reg && (op->unk_07 & mask))
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
        RegPoolEntry *r = op->getReg2();
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
        if (op->isReg2() && op->unk_06) {
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
        if (op->getReg2() && !op->unk_0b)
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
        case 22:
            op->unk_08 = 1;
            op->unk_04 = 20;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(56);
            op->unk_07 = 1;
            op->unk_0a = unk_38[0].unk_0a;
            op->unk_0b = 0;
            op->unk_26 = unk_138b++;
            op = &unk_38[i + 1];
            op->unk_08 = 1;
            op->unk_04 = 19;
            op->unk_09 = 1;
            op->unk_14 = op->unk_18 = func_0806c224(58);
            op->unk_07 = 1;
            op->unk_0a = -1;
            op->unk_0b = 1;
            op->unk_26 = unk_138b++;
            break;
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
            if (v->type == mValue::STRING) {
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
                op->unk_26 = unk_138b++;
                op++;
            }
            if (b) {
                op->unk_08 = 1;
                op->unk_04 = 20;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(56);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 0;
                op->unk_26 = unk_138b++;
                op++;
            }
            if (c) {
                op->unk_08 = 1;
                op->unk_04 = 18;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(57);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b++;
                op++;
                op->unk_08 = 1;
                op->unk_04 = 18;
                op->unk_09 = 2;
                op->unk_14 = op->unk_18 = func_0806c224(57);
                op->unk_07 = 1;
                op->unk_0a = 1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b++;
                op++;
            }
            if (d) {
                op->unk_08 = 1;
                op->unk_04 = 19;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(58);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 1;
                op->unk_26 = unk_138b++;
                op++;
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
            op->unk_26 = unk_138b++;
            op = &unk_38[i + 1];
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
