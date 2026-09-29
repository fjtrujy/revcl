// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

char *dupString(const char *s);

extern int errorOccurred;
extern bool quiet_080bbd98;

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
    func_0806a310();
    unk_34 = info;
    unk_13d7 = info->unk_0e;
    unk_138c = info->unk_0e;
    unk_138d = info->unk_10;
    unk_1388 = info->func_0806ffb4();
    unk_13cf = info->unk_12;
    unk_13ce = info->func_0806ffd8();
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
        RegPoolEntry *r = op->getReg();
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
