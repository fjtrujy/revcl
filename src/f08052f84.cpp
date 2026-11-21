// iHolder and infoList, CIE group 3.
#pragma implementation "infoNode_inline.h"
#include "infoList_shared.h"

bool Instr::func_08052f84()
{
    for (int i = 0; i != unk_138b; i++) {
        Operand *op = &unk_38[i];
        if (!registerOperand(op, 1))
            continue;
        op->unk_29 = 0;
        if (!(unk_1388 & 4))
            continue;
        for (ol_s *ol = op->unk_2c[0].unk_54.unk_00; ol; ol = ol->unk_00) {
            Instr *in = ol->unk_04;
            bool found = false;
            if (in->unk_13ce) {
                int c = ol->unk_10;
                if ((char)c == 0)
                    continue;
            }
            for (int j = 0; j != in->unk_138b; j++) {
                RegPoolEntry *r = registerOperand(&in->unk_38[j], 1);
                if (r && (unsigned)(r->unk_30 - 54) <= 5) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                op->unk_29 = 1;
                break;
            }
        }
    }
    return true;
}

int Instr::func_08053104()
{
    int id, target;
    if (!unk_34)
        return 0;
    target = -1;
    id = func_0806c72c(unk_34);
    if (id < 0) {
        func_0804ad38("vpu: fixup branch - iUID<0\n");
        return -1;
    }
    if (!(unk_1388 & 4))
        return 1;
    if (unk_1388 & 8)
        return 1;
    if (unk_138b == 2) {
        RegPoolEntry *r = unk_38[0].unk_18;
        if (!r)
            return -1;
        if (r->unk_30 == 0x20) {
            if (id == 0xa8)
                return 0;
            if (id == 0xa9)
                target = 1;
            if (id == 0xaa)
                return 0;
            if (id == 0xab)
                target = 1;
        }
    }
    if (unk_138b == 3 && unk_38[0].unk_18 == unk_38[1].unk_18) {
        if (id == 0xac)
            target = 2;
        if (id == 0xad)
            return 0;
    }
    if (target > 0) {
        for (int i = 0; i != unk_138b; i++) {
            unk_38[i].unk_18 = 0;
            unk_38[i].unk_04 = 0;
        }
        unk_138b = 1;
        unk_38[0].unk_04 = 0x1b;
        if (!unk_38[0].unk_1c.set(&unk_38[target].unk_1c)) {
            func_0804ad38("fixup branch -set new ximm\n");
            return -1;
        }
        unk_38[target].unk_1c.clear();
        unk_34 = func_0806bf8c("b", true);
        if (!func_08053fac()) {
            func_0804ad38("failed to convert a branch\n");
            return -1;
        }
        unk_1388 = 1;
        return 2;
    }
    return 1;
}

