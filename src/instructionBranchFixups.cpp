#include "os.h"
// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

bool Instr::patchBranchLatencies()
{
    for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
        Operand *operand = &operands[operandIndex];
        if (!registerOperand(operand, 1))
            continue;
        operand->additionalLatency = 0;
        if (!(branchFlags & 4))
            continue;
        for (OperandLocation *location = operand->parts[0].operandLocations.head; location; location = location->next) {
            Instr *branchInstruction = location->instruction;
            bool found = false;
            if (branchInstruction->ramAccessType) {
                int locationOperandIndex = location->operandIndex;
                if ((char)locationOperandIndex == 0)
                    continue;
            }
            for (int branchOperandIndex = 0; branchOperandIndex != branchInstruction->operandCount; branchOperandIndex++) {
                RegisterEntry *registerEntry = registerOperand(&branchInstruction->operands[branchOperandIndex], 1);
                if (registerEntry && (unsigned)(registerEntry->registerIndex - 54) <= 5) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                operand->additionalLatency = 1;
                break;
            }
        }
    }
    return true;
}

int Instr::simplifyBranch()
{
    int opcodeId, target;
    if (!opcodeInfo)
        return 0;
    target = -1;
    opcodeId = getOpcodeId(opcodeInfo);
    if (opcodeId < 0) {
        reportError("vpu: fixup branch - iUID<0\n");
        return -1;
    }
    if (!(branchFlags & 4))
        return 1;
    if (branchFlags & 8)
        return 1;
    if (operandCount == 2) {
        RegisterEntry *registerEntry = operands[0].assignedRegister;
        if (!registerEntry)
            return -1;
        if (registerEntry->registerIndex == 0x20) {
            if (opcodeId == 0xa8)
                return 0;
            if (opcodeId == 0xa9)
                target = 1;
            if (opcodeId == 0xaa)
                return 0;
            if (opcodeId == 0xab)
                target = 1;
        }
    }
    if (operandCount == 3 && operands[0].assignedRegister == operands[1].assignedRegister) {
        if (opcodeId == 0xac)
            target = 2;
        if (opcodeId == 0xad)
            return 0;
    }
    if (target > 0) {
        for (int operandIndex = 0; operandIndex != operandCount; operandIndex++) {
            operands[operandIndex].assignedRegister = 0;
            operands[operandIndex].operandKind = VCL_OP_NULL;
        }
        operandCount = 1;
        operands[0].operandKind = VCL_OP_IMM;
        if (!operands[0].immediateValue.set(&operands[target].immediateValue)) {
            reportError("fixup branch -set new ximm\n");
            return -1;
        }
        operands[target].immediateValue.clear();
        opcodeInfo = findOpcodeInfoByMnemonic("b", true);
        if (!validateOperands()) {
            reportError("failed to convert a branch\n");
            return -1;
        }
        branchFlags = 1;
        return 2;
    }
    return 1;
}

