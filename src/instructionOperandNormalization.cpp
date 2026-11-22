#include "os.h"
// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

int Instr::normalizeZeroSources()
{
    int r = 0;
    int opcode = opcodeId();
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
        RegPoolEntry *registerEntry = operands[1].assignedRegister;
        operands[1].assignedRegister = operands[2].assignedRegister;
        operands[2].assignedRegister = registerEntry;
        operands[1].forcedBaseRegister = operands[2].forcedBaseRegister;
        operands[2].forcedBaseRegister = 0;
    }
    goto done;

invalidOpcode:
    r = -1;
    goto done;

resetSources:
    {
        Operand *operand = &operands[1];
        if (operand->assignedRegister == operands[2].assignedRegister) {
            RegPoolEntry *z = getOperandZeroReg(operand->operandKind);
            if (!z) {
                reportError("failed to get a zero register");
                return 0;
            }
            for (int operandIndex = 1; operandIndex != 3; ) {
                r = 0;
                operands[operandIndex].assignedRegister = z;
                r = 1;
                if (++operandIndex == 3)
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
