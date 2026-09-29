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

char *Operand::Emit(char *buf, bool, bool symbolic)
{
    const char *rn = 0;
    if (unk_08)
        return 0;
    if (!func_080563f8())
        return 0;
    if (unk_18) {
        int z = unk_18 == unk_18->unk_04->unk_10;
        if ((bool)z != isZero()) {
            func_0804ad38("Operand::Emit - zero mismatch..%s but %d\n", unk_18->func_0805a8e0(), z);
            return 0;
        }
    }
    if (symbolic) {
        if (unk_18) {
            if (!unk_14) {
                func_0804ad38("no original register set register is %s\n", unk_18->func_0805a8e0());
                return 0;
            }
            rn = unk_14->func_0805a8e0();
        }
    } else {
        if (unk_18) {
            if (unk_18->unk_14) {
                func_0804ad38("non symbolic emit.. but found a virtual register in an operand!\n");
                return 0;
            }
            rn = unk_18->func_0805a8e0();
            unk_18->unk_35 |= unk_07;
        }
    }
    switch (unk_04) {
    case 1:
    case 2:
    case 34:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 54:
        buf += func_0804adbc(buf, "%s", rn);
        break;
    case 3:
        if (unk_07 & 1)
            buf += func_0804adbc(buf, "%sx", rn);
        else if (unk_07 & 2)
            buf += func_0804adbc(buf, "%sy", rn);
        else if (unk_07 & 4)
            buf += func_0804adbc(buf, "%sz", rn);
        else if (unk_07 & 8)
            buf += func_0804adbc(buf, "%sw", rn);
        else
            return 0;
        break;
    case 4:
        buf += func_0804adbc(buf, "I");
        break;
    case 5:
        buf += func_0804adbc(buf, "P");
        break;
    case 6:
        buf += func_0804adbc(buf, "nP");
        break;
    case 7:
        buf += func_0804adbc(buf, "Q");
        break;
    case 8:
        buf += func_0804adbc(buf, "R");
        break;
    case 9:
        buf += func_0804adbc(buf, "ACC");
        break;
    case 10:
        buf += func_0804adbc(buf, "ACCxyz");
        break;
    case 11:
        buf += func_0804adbc(buf, "%sxyz", rn);
        break;
    case 13:
        buf += func_0804adbc(buf, "%sx", rn);
        break;
    case 14:
        buf += func_0804adbc(buf, "%sy", rn);
        break;
    case 15:
        buf += func_0804adbc(buf, "%sz", rn);
        break;
    case 16:
        buf += func_0804adbc(buf, "%sw", rn);
        break;
    case 27:
        if (unk_1c.isNumeric()) {
            if (func_08070300(unk_05) == 2)
                buf += func_0804adbc(buf, "%d", unk_1c.i);
            else
                buf += func_0804adbc(buf, "0x%08x", unk_1c.u);
        } else {
            char *s = unk_1c.asChars();
            if (!s) {
                func_0804ad38("failed to get chars from immediate operand\n");
                return 0;
            }
            buf += func_0804adbc(buf, "%s", s);
        }
        break;
    case 31: {
        char *s = unk_1c.asChars();
        if (!s)
            func_0804ad38("failed to get chars from ximm for v_ea_imm\n");
        buf += func_0804adbc(buf, "%s", s);
    }
    case 30:
        if (!rn) {
            func_0804ad38("null rn - abort _V_EA_REG\n");
            return 0;
        }
    case 28:
        buf += func_0804adbc(buf, "(%s)", rn);
        break;
    case 32:
        if (!rn) {
            func_0804ad38("null rn - abort _V_EA_INC\n");
            return 0;
        }
        buf += func_0804adbc(buf, "(%s++)", rn);
        break;
    case 33:
        if (!rn) {
            func_0804ad38("null rn - abort _V_EA_DEC\n");
            return 0;
        }
        buf += func_0804adbc(buf, "(--%s)", rn);
        break;
    case 37:
        if (!rn) {
            func_0804ad38("emit ramreg - no rn\n");
            return 0;
        }
        buf += func_0804adbc(buf, "ramreg-[%s]", rn);
        break;
    case 29:
    case 46: {
        if (!rn) {
            func_0804ad38("null rn - abort _BLAH_EA_IMM\n");
            return 0;
        }
        char *s = unk_1c.asChars();
        if (!s) {
            func_0804ad38("failed to get chars from ximm blah_ea_imm\n");
            return 0;
        }
        buf += func_0804adbc(buf, "%s(%s)", s, rn);
        break;
    }
    case 55:
        break;
    default:
        func_0804ad38("asked to emit an unknown/unemittable operand\n");
        return 0;
    }
    return buf;
}

bool Operand::func_080569cc()
{
    int i = 0;
    unk_28 = 0;
    unk_06 = 0;
    unk_27 = 0;
    unk_2b = 0;
    for (; i != 4; i++) {
        if (!unk_2c[i].unk_54.clear()) {
            func_0804ad38("failed to flow reset part %d\n", i);
            return false;
        }
    }
    return true;
}

bool Operand::func_08056a24()
{
    for (int i = 0; i != 4; i++) {
        OpSlot *s = &unk_2c[i];
        s->func_0805bcc4();
        s->unk_50 = 0;
        s->unk_52[0] = 0;
    }
    return true;
}
