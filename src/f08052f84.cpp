// iHolder and infoList, CIE group 3.
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

