// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

int Instr::func_080532d8()
{
    int r = 0;
    switch (func_08055ac4()) {
    case -1:
        r = -1;
        break;
    case 0x22:
    case 0x6c:
        if (unk_38[1].isZero() && !unk_38[2].isZero()) {
            RegPoolEntry *t = unk_38[1].unk_18;
            unk_38[1].unk_18 = unk_38[2].unk_18;
            unk_38[2].unk_18 = t;
            unk_38[1].unk_10 = unk_38[2].unk_10;
            unk_38[2].unk_10 = 0;
        }
        break;
    case 0x30:
    case 0x6f: {
        Operand *op = &unk_38[1];
        if (op->unk_18 == unk_38[2].unk_18) {
            RegPoolEntry *z = getOperandZeroReg(op->unk_04);
            if (!z) {
                func_0804ad38("failed to get a zero register");
                return 0;
            }
            for (int i = 1; i != 3; i++) {
                unk_38[i].unk_18 = z;
                r = 1;
            }
        }
        break;
    }
    }
    return r;
}
