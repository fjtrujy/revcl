// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

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
