#include "os.h"
// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

int Instr::normalizeZeroSources()
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
    if (isZeroOperand(&operands[1]) && !isZeroOperand(&operands[2])) {
        RegPoolEntry *t = operands[1].assignedRegister;
        operands[1].assignedRegister = operands[2].assignedRegister;
        operands[2].assignedRegister = t;
        operands[1].forcedBaseRegister = operands[2].forcedBaseRegister;
        operands[2].forcedBaseRegister = 0;
    }
    goto done;

invalidOpcode:
    r = -1;
    goto done;

resetSources:
    {
        Operand *op = &operands[1];
        if (op->assignedRegister == operands[2].assignedRegister) {
            RegPoolEntry *z = getOperandZeroReg(op->operandKind);
            if (!z) {
                reportError("failed to get a zero register");
                return 0;
            }
            for (int i = 1; i != 3; ) {
                r = 0;
                operands[i].assignedRegister = z;
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
