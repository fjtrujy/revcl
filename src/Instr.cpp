// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include "os.h"

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
