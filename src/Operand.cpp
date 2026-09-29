// Operand, CIE group 4. Separate TU from Instr.cpp: its .rodata (strings from
// 0x08090848, jump tables from 0x080915e4) follows Instr's jump tables.
#include "Instr.h"
#include "os.h"

Operand::Operand()
{
    unk_26 = -1;
    unk_18 = 0;
    unk_08 = 0;
    unk_04 = 0;
    unk_07 = 0;
    unk_29 = 0;
    unk_10 = 0;
    unk_00 = 0;
    unk_25 = -1;
    unk_24 = -1;
    unk_0d = 0;
    unk_14 = 0;
    unk_28 = 0;
    unk_06 = 0;
    unk_27 = 0;
    unk_2b = 0;
    int i;
    for (i = 0; i != 4; i++) {
        if (!unk_2c[i].unk_54.clear()) {
            func_0804ad38("failed to flow reset part %d\n", i);
            break;
        }
    }
}

Operand::~Operand()
{
}

bool Operand::func_080563f8()
{
    switch (unk_04) {
    case 1:
    case 35:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FL4 or V_FL4S/ MEM!\n");
            return false;
        }
        if ((unk_07 & 0xf) == unk_07)
            return true;
        func_0804ad38("not all!\n");
        return false;
    case 3:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FL1!\n");
            return false;
        }
        if (unk_07 == 1 || unk_07 == 2 || unk_07 == 4 || unk_07 == 8)
            return true;
        func_0804ad38("bad mod bits for FL1..%d\n", unk_07);
        return false;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_P/Q/R/I/nQ!\n");
            return false;
        }
        if (unk_07 == 1)
            return true;
        func_0804ad38("bad mod bits for V_P/Q/R/I\n");
        return false;
    case 9:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_ACC!\n");
            return false;
        }
        if ((unk_07 & 0xf) == unk_07)
            return true;
        func_0804ad38("bad mod bits..hmm..out of range for acc\n");
        return false;
    case 10:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_ACCYZ!\n");
            return false;
        }
        if (unk_07 == 7)
            return true;
        func_0804ad38("xyz not set to ACCXYZ!\n");
        return false;
    case 11:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FLXYZ!\n");
            return false;
        }
        if (unk_07 == 7)
            return true;
        func_0804ad38("xyz not set to XYZ!\n");
        return false;
    case 13:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FLX!\n");
            return false;
        }
        if (unk_07 == 1)
            return true;
        func_0804ad38("bad mod bits for FLX\n");
        return false;
    case 14:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FLY!\n");
            return false;
        }
        if (unk_07 == 2)
            return true;
        func_0804ad38("bad mod bits for FLY\n");
        return false;
    case 15:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FLZ!\n");
            return false;
        }
        if (unk_07 == 4)
            return true;
        func_0804ad38("bad mod bits for FLZ\n");
        return false;
    case 16:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_FLW!\n");
            return false;
        }
        if (unk_07 == 8)
            return true;
        func_0804ad38("bad mod bits for FLW\n");
        return false;
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 34:
    case 45:
    case 55:
        return true;
    case 27:
        return unk_1c.isValid() != 0;
    case 28:
        if (unk_07 != 1)
            return false;
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 46:
        return unk_18 != 0;
    case 2:
    case 36:
        if (!unk_18) {
            func_0804ad38("reg no reg set - V_INT/INTS!\n");
            return false;
        }
        if (unk_07 == 1)
            return true;
        func_0804ad38("bad mod bits for int/ints!\n");
        return false;
    case 37:
        if (unk_18)
            return true;
        func_0804ad38("ramreg.. not ok.. no m_Reg\n");
        return false;
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
        if (unk_18)
            return true;
        func_0804ad38("not ok type %d.. no reg!\n", unk_04);
        return false;
    }
    return false;
}

void Operand::func_0805665c(int)
{
    if (func_080563f8()) {
        bool ok = true;
        switch (unk_04) {
        case 0:
            break;
        case 28:
        case 29:
        case 30:
        case 32:
        case 33:
        case 46:
            ok = unk_1c.isValid();
            break;
        case 27:
            unk_1c.dump();
            break;
        case 55:
            break;
        default:
            return;
        }
        if (!ok)
            return;
    }
}
