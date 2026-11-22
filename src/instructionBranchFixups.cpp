#include "os.h"
// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

bool Instr::patchBranchLatencies()
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (!registerOperand(op, 1))
            continue;
        op->additionalLatency = 0;
        if (!(branchFlags & 4))
            continue;
        for (ol_s *ol = op->parts[0].operandLocations.head; ol; ol = ol->next) {
            Instr *in = ol->instruction;
            bool found = false;
            if (in->ramAccessType) {
                int c = ol->operandIndex;
                if ((char)c == 0)
                    continue;
            }
            for (int j = 0; j != in->operandCount; j++) {
                RegPoolEntry *r = registerOperand(&in->operands[j], 1);
                if (r && (unsigned)(r->registerIndex - 54) <= 5) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                op->additionalLatency = 1;
                break;
            }
        }
    }
    return true;
}

int Instr::simplifyBranch()
{
    int id, target;
    if (!opcodeInfo)
        return 0;
    target = -1;
    id = getOpcodeId(opcodeInfo);
    if (id < 0) {
        reportError("vpu: fixup branch - iUID<0\n");
        return -1;
    }
    if (!(branchFlags & 4))
        return 1;
    if (branchFlags & 8)
        return 1;
    if (operandCount == 2) {
        RegPoolEntry *r = operands[0].assignedRegister;
        if (!r)
            return -1;
        if (r->registerIndex == 0x20) {
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
    if (operandCount == 3 && operands[0].assignedRegister == operands[1].assignedRegister) {
        if (id == 0xac)
            target = 2;
        if (id == 0xad)
            return 0;
    }
    if (target > 0) {
        for (int i = 0; i != operandCount; i++) {
            operands[i].assignedRegister = 0;
            operands[i].operandKind = 0;
        }
        operandCount = 1;
        operands[0].operandKind = 0x1b;
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

