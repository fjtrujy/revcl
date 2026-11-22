#include "os.h"
// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

int Instr::normalizeZeroSources()
{
    int normalizedCount = 0;
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
        RegisterEntry *registerEntry = operands[1].assignedRegister;
        operands[1].assignedRegister = operands[2].assignedRegister;
        operands[2].assignedRegister = registerEntry;
        operands[1].forcedBaseRegister = operands[2].forcedBaseRegister;
        operands[2].forcedBaseRegister = 0;
    }
    goto done;

invalidOpcode:
    normalizedCount = -1;
    goto done;

resetSources:
    {
        Operand *operand = &operands[1];
        if (operand->assignedRegister == operands[2].assignedRegister) {
            RegisterEntry *zeroRegister = getOperandZeroRegister(operand->operandKind);
            if (!zeroRegister) {
                reportError("failed to get a zero register");
                return 0;
            }
            for (int operandIndex = 1; operandIndex != 3; ) {
                normalizedCount = 0;
                operands[operandIndex].assignedRegister = zeroRegister;
                normalizedCount = 1;
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
    return normalizedCount;
}
