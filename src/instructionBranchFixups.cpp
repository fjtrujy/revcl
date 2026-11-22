#include "os.h"
// iHolder and infoList, CIE group 3.
#include "infoList_shared.h"

bool Instr::func_08052f84()
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (!registerOperand(op, 1))
            continue;
        op->unk_29 = 0;
        if (!(unk_1388 & 4))
            continue;
        for (ol_s *ol = op->unk_2c[0].unk_54.head; ol; ol = ol->unk_00) {
            Instr *in = ol->unk_04;
            bool found = false;
            if (in->unk_13ce) {
                int c = ol->unk_10;
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
    if (!opcodeInfo)
        return 0;
    target = -1;
    id = func_0806c72c(opcodeInfo);
    if (id < 0) {
        reportError("vpu: fixup branch - iUID<0\n");
        return -1;
    }
    if (!(unk_1388 & 4))
        return 1;
    if (unk_1388 & 8)
        return 1;
    if (operandCount == 2) {
        RegPoolEntry *r = operands[0].unk_18;
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
    if (operandCount == 3 && operands[0].unk_18 == operands[1].unk_18) {
        if (id == 0xac)
            target = 2;
        if (id == 0xad)
            return 0;
    }
    if (target > 0) {
        for (int i = 0; i != operandCount; i++) {
            operands[i].unk_18 = 0;
            operands[i].unk_04 = 0;
        }
        operandCount = 1;
        operands[0].unk_04 = 0x1b;
        if (!operands[0].unk_1c.set(&operands[target].unk_1c)) {
            reportError("fixup branch -set new ximm\n");
            return -1;
        }
        operands[target].unk_1c.clear();
        opcodeInfo = func_0806bf8c("b", true);
        if (!func_08053fac()) {
            reportError("failed to convert a branch\n");
            return -1;
        }
        unk_1388 = 1;
        return 2;
    }
    return 1;
}

