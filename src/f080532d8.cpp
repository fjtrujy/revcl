// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

int Instr::func_080532d8()
{
    int r = 0;
    int opcode = func_08055ac4();
    if (opcode == 0x6c)
        goto swapSources;
    if (opcode > 0x6c)
        goto checkUpperOpcode;
    if (opcode == 0x30)
        goto resetSources;
    if (opcode > 0x30)
        goto done;
    if (opcode == -1)
        goto invalidOpcode;
    if (opcode == 0x22)
        goto swapSources;
    goto done;

swapSources:
    if (isZeroOperand(&unk_38[1]) && !isZeroOperand(&unk_38[2])) {
        RegPoolEntry *t = unk_38[1].unk_18;
        unk_38[1].unk_18 = unk_38[2].unk_18;
        unk_38[2].unk_18 = t;
        unk_38[1].unk_10 = unk_38[2].unk_10;
        unk_38[2].unk_10 = 0;
    }
    goto done;

invalidOpcode:
    r = -1;
    goto done;

resetSources:
    {
        Operand *op = &unk_38[1];
        if (op->unk_18 == unk_38[2].unk_18) {
            RegPoolEntry *z = getOperandZeroReg(op->unk_04);
            if (!z) {
                func_0804ad38("failed to get a zero register");
                return 0;
            }
            for (int i = 1; i != 3; ) {
                r = 0;
                unk_38[i].unk_18 = z;
                r = 1;
                if (++i == 3)
                    break;
            }
        }
    }
    goto done;

checkUpperOpcode:
    if (opcode != 0x6f)
        goto done;
    goto resetSources;
done:
    return r;
}
