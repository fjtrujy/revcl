// Operand, CIE group 4. Separate TU from Instr.cpp: its .rodata (strings from
// 0x08090848, jump tables from 0x080915e4) follows Instr's jump tables.
#include "Instr.h"
#include "infoList.h"
#include "os.h"
#include "VUStr.h"

int func_0804bf50(unsigned int);
bool func_0804bfcc(const char *a, const char *b);
void func_0804c0c8(char *s);
int mstrlen(const char *s);

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

static inline int operandZeroValue(Operand *op)
{
    int z = 0;
    if (op->unk_18) {
        if (op->unk_18 == op->unk_18->unk_04->unk_10)
            z = 1;
    }
    return z;
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
        if ((char)z != (char)operandZeroValue(this)) {
            func_0804ad38("Operand::Emit - zero mismatch..%s but %d\n", unk_18->func_0805a8e0(), operandZeroValue(this));
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
        RegPoolEntry *reg = unk_18;
        if (reg) {
            if (reg->unk_14) {
                func_0804ad38("non symbolic emit.. but found a virtual register in an operand!\n");
                return 0;
            }
            rn = reg->func_0805a8e0();
            reg->unk_35 |= unk_07;
        }
    }
    switch (unk_04) {
    case 13:
    emit_x:
        buf += func_0804adbc(buf, "%sx", rn);
        break;
    case 3:
        if (unk_07 & 1)
            goto emit_x;
        if (unk_07 & 2)
            goto emit_y;
        if (unk_07 & 4)
            goto emit_z;
        if (unk_07 & 8)
            goto emit_w;
        return 0;
    case 14:
    emit_y:
        buf += func_0804adbc(buf, "%sy", rn);
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
    case 15:
    emit_z:
        buf += func_0804adbc(buf, "%sz", rn);
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
    case 16:
    emit_w:
        buf += func_0804adbc(buf, "%sw", rn);
        break;
    case 11:
        buf += func_0804adbc(buf, "%sxyz", rn);
        break;
    case 1:
    case 2:
        buf += func_0804adbc(buf, "%s", rn);
        break;
    case 39:
    case 40:
    case 41:
    case 42:
        buf += func_0804adbc(buf, "%s", rn);
        break;
    case 43:
    case 44:
        buf += func_0804adbc(buf, "%s", rn);
        break;
    case 54:
        buf += func_0804adbc(buf, "%s", rn);
        break;
    case 34:
        buf += func_0804adbc(buf, "%s", rn);
        break;
    case 27: {
        mValue *imm = &unk_1c;
        if (!imm->isNumeric()) {
            char *s = imm->asChars();
            if (!s) {
                func_0804ad38("failed to get chars from immediate operand\n");
                return 0;
            }
            buf += func_0804adbc(buf, "%s", s);
        } else {
            if (func_08070300(unk_05) == 2)
                buf += func_0804adbc(buf, "%d", imm->i);
            else
                buf += func_0804adbc(buf, "0x%08x", imm->u);
        }
        break;
    }
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
        buf += func_0804adbc("ramreg-[%s]", rn);
        break;
    case 55:
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

static inline bool resetOperandLock(cls0805bcc4 *lock)
{
    return lock->func_0805bcc4();
}

bool Operand::func_08056a24()
{
    for (int i = 0; i != 4; i++) {
        OpSlot *s = &unk_2c[i];
        resetOperandLock(s);
        s->unk_50 = 0;
        s->unk_52[0] = 0;
    }
    return true;
}

bool buildImplicitOperand(int kind, ImplicitOp *out, bool)
{
    out->unk_1800[0] = 0;
    out->unk_1000[0] = 0;
    out->unk_2000 = 0;
    out->unk_200c = 0;
    switch (kind) {
    case 10: mstrcpy(out->name, "ACC");   out->unk_2000 = 7; return true;
    case 4:  mstrcpy(out->name, "I");     out->unk_2000 = 1; return true;
    case 5:  mstrcpy(out->name, "P");     out->unk_2000 = 1; return true;
    case 6:  mstrcpy(out->name, "tt_nP"); out->unk_2000 = 1; return true;
    case 7:  mstrcpy(out->name, "Q");     out->unk_2000 = 1; return true;
    case 8:  mstrcpy(out->name, "R");     out->unk_2000 = 1; return true;
    case 9:  mstrcpy(out->name, "ACC");   out->unk_2000 = 15; return true;
    case 17: mstrcpy(out->name, "CLIP");  out->unk_2000 = 1; return true;
    case 18: case 19: case 20: case 21:
        func_0804ad38("odecode for a V_STAT\n");
        return false;
    case 55:
        out->name[0] = 0;
        return true;
    case 22: case 23: case 24: case 25:
        out->name[0] = 0;
        return true;
    case 26: mstrcpy(out->name, "MAC");   out->unk_2000 = 1; return true;
    case 28: out->unk_2000 = 1; return true;
    case 37: mstrcpy(out->name, "ramreg"); return true;
    case 54:
        mstrcpy(out->unk_800, "$ra");
        mstrcpy(out->name, "$ra");
        out->unk_2000 = 3;
        return true;
    case 40: out->unk_2000 = 3; return true;
    case 45: mstrcpy(out->name, "CLIPA"); out->unk_2000 = 1; return true;
    default:
        func_0804ad38("fell through building an implicit operand of type %s\n", opKindName(kind));
        out->name[0] = 0;
        out->unk_800[0] = 0;
        return false;
    case 47: mstrcpy(out->name, "HILO0"); out->unk_2000 = 1; return true;
    case 48: mstrcpy(out->name, "HILO1"); out->unk_2000 = 1; return true;
    case 49: mstrcpy(out->name, "FSTAT"); out->unk_2000 = 1; return true;
    case 50: mstrcpy(out->name, "FCOND"); out->unk_2000 = 1; return true;
    case 51: mstrcpy(out->name, "FACC");  out->unk_2000 = 1; return true;
    case 52: mstrcpy(out->name, "FDIV");  out->unk_2000 = 1; return true;
    case 53: mstrcpy(out->name, "SHFTA"); out->unk_2000 = 1; return true;
    }
}

bool oDecodeToken(int kind, char *tok, ImplicitOp *out, bool newSyntax)
{
    bool bad2;
    bool *pbad = &bad2;
    int mask;
    int *pmask = &mask;
    bool bad;
    int m;
    int field;
    int count;
    char *p;
    char *q;
    char *s;
    char *d;
    char *end;
    char *start;

    if (!tok || !out || !*tok || *tok == '.')
        return false;
    if (mstrlen(tok) > 2047) {
        func_0804ad38("odecodeToken: token was too long\n");
        return false;
    }

    char *t = tok;
    while (*t && *t != ':')
        t++;
    if (*t == ':') {
        *t = 0;
        char *td = out->unk_1800;
        t++;
        int chars = 0;
        int seps = 1;
        while (*t) {
            if (*t != ':') {
                seps = 0;
                chars++;
            } else {
                chars = 0;
                seps++;
            }
            if (seps <= 1 || chars)
                *td++ = *t;
            t++;
        }
        if (seps)
            td--;
        if (td == out->unk_1800) {
            func_0804ac7c("ignoring empty tag\n");
            out->unk_1800[0] = 0;
        } else {
            td[0] = ':';
            td[1] = 0;
        }
    }

    char *e = tok;
    while (*e)
        e++;
    count = e - tok;
    out->name[0] = 0;
    out->unk_1000[0] = 0;
    out->unk_800[0] = 0;
    out->unk_200c = 0;
    out->unk_2000 = 0;
    out->unk_2004 = 0;

    switch (kind) {
    case 54:
        func_0804ad38("encountered a force bind to $ra in an explicit operand\n");
        return false;
    case 1:
        if (newSyntax) {
            out->unk_2000 = 15;
            mstrcpy(out->name, tok);
            return true;
        }
        out->unk_2000 = func_08052100(tok);
        if (!out->unk_2000)
            out->unk_2000 = 15;
        if (out->unk_2000 == -1) {
            func_0804ad38("oDecodeToken: bad FL4 bits\n");
            return false;
        }
        mstrcpy(out->name, tok);
        func_0804c0c8(out->name);
        return true;
    case 2:
        mstrcpy(out->name, tok);
        out->unk_2000 = 1;
        return true;
    case 3:
        if (newSyntax) {
            if (!func_08051f78(tok, pmask) || func_0804bf50(mask) != 1) {
                func_0804ad38("oDecodeToken: [x/y/z/w] token for new syntax\n");
                return false;
            }
            out->unk_2000 = mask;
            char *s = tok;
            char *d = out->name;
            while (*s)
                *d++ = *s++;
            *d = 0;
            do
                d--;
            while (*d != '[');
            *d = 0;
            return true;
        }
        mask = func_08052100(tok);
        if (func_0804bf50(mask) != 1) {
            func_0804ad38("oDecodeToken: bad FL1 bits\n");
            return false;
        }
        out->unk_2000 = mask;
        mstrcpy(out->name, tok);
        func_0804c0c8(out->name);
        return true;
    case 4:
        if (count != 1 || (*tok != 'i' && *tok != 'I')) {
            func_0804ad38("oDecode:: arg I\n");
            return false;
        }
        out->unk_2000 = 1;
        out->name[0] = 'I';
        out->name[1] = 0;
        return true;
    case 5:
        if (count != 1 || (*tok != 'p' && *tok != 'P'))
            func_0804ad38("oDecode:: arg P\n");
        out->unk_2000 = 1;
        out->name[0] = 'P';
        out->name[1] = 0;
        return true;
    case 7:
        if (count != 1 || (*tok != 'q' && *tok != 'Q')) {
            func_0804ad38("oDecode:: arg Q\n");
            return false;
        }
        out->unk_2000 = 1;
        out->name[0] = 'Q';
        out->name[1] = 0;
        return true;
    case 8:
        if (count != 1 || (*tok != 'r' && *tok != 'R')) {
            func_0804ad38("oDecode:: arg R\n");
            return false;
        }
        out->unk_2000 = 1;
        out->name[0] = 'R';
        out->name[1] = 0;
        return true;
    case 9: {
        bad = false;
        if (!func_08051f08(tok, &out->unk_2000, &bad))
            out->unk_2000 = 15;
        if (out->unk_2000 == -1) {
            func_0804ad38("oDecodeToken: bad FL4 bits\n");
            return false;
        }
        mstrcpy(out->name, "ACC");
        return true;
    }
    case 10:
        mask = func_08052100(tok);
        if (mask && mask != 7) {
            func_0804ad38("bad mod bits for ACCxyz\n");
            return false;
        }
        out->unk_2000 = 7;
        mstrcpy(out->name, "ACC");
        return true;
    case 11:
        field = 7;
    fieldSel:
        if (newSyntax) {
            m = 0;
            if (func_08051f78(tok, &m)) {
                if (m != field) {
                    func_0804ad84("instruction and operand have mismatching field select\n");
                    return false;
                }
                char *s = tok;
                char *d = out->name;
                while (*s)
                    *d++ = *s++;
                *d = 0;
                do
                    d--;
                while (*d != '[');
                *d = 0;
                out->unk_2000 = field;
                return true;
            }
            mstrcpy(out->name, tok);
            out->unk_2000 = field;
            return true;
        }
        mask = func_08052100(tok);
        if (mask && mask != field) {
            func_0804ad38("oDecode: bad FL?\n");
            return false;
        }
        out->unk_2000 = field;
        mstrcpy(out->name, tok);
        func_0804c0c8(out->name);
        return true;
    case 13:
        field = 1;
        goto fieldSel;
    case 14:
        field = 2;
        goto fieldSel;
    case 15:
        field = 4;
        goto fieldSel;
    case 16:
        field = 8;
        goto fieldSel;
    case 17:
        out->unk_2000 = 1;
        mstrcpy(out->name, "CLIP");
        return true;
    case 26:
        func_0804ad38("explicit mac register - but I'm writing ASTATUS here??\n");
        func_0804adf0(1);
        out->unk_2000 = 1;
        mstrcpy(out->name, "ASTATUS");
        return true;
    case 27:
        mstrcpy(out->unk_1000, tok);
        return true;
    case 28:
        out->unk_2000 = 1;
        if (count <= 2)
            return false;
        if (e[-1] != ')' || *tok != '(')
            return false;
        start = tok + 1;
        end = e - 1;
        goto copyName;
    case 30:
        out->unk_2000 = 1;
        if (count <= 3)
            return false;
        if (*tok != '(')
            return false;
        p = e - 1;
        while (p != tok && *p != ')') {
            switch (*p) {
            case 'x': out->unk_2004 |= 1; break;
            case 'y': out->unk_2004 |= 2; break;
            case 'z': out->unk_2004 |= 4; break;
            case 'w': out->unk_2004 |= 8; break;
            default: return false;
            }
            p--;
        }
        if (*p != ')')
            return false;
        start = tok + 1;
        end = p;
        goto copyName;
    case 31:
        out->unk_2000 = 1;
        if (count <= 4)
            return false;
        p = e - 1;
        while (p != tok && *p != ')') {
            switch (*p) {
            case 'x': out->unk_2004 |= 1; break;
            case 'y': out->unk_2004 |= 2; break;
            case 'z': out->unk_2004 |= 4; break;
            case 'w': out->unk_2004 |= 8; break;
            default: return false;
            }
            p--;
        }
        if (*p != ')')
            return false;
        end = p;
        q = e;
        while (q != tok && *q != '(')
            q--;
        if (*q != '(')
            return false;
        d = out->unk_1000;
        for (s = tok; s != q; )
            *d++ = *s++;
        *d = 0;
        if (!out->unk_1000[0]) {
            out->unk_1000[0] = '0';
            out->unk_1000[1] = 0;
        }
        start = q + 1;
    copyName:
        d = out->name;
        for (s = start; s != end; )
            *d++ = *s++;
        *d = 0;
        return true;
    case 32:
        out->unk_2000 = 1;
        if (count <= 4)
            return false;
        if (e[-1] != ')' || e[-2] != '+' || e[-3] != '+' || *tok != '(')
            return false;
        start = tok + 1;
        end = e - 3;
        goto copyName;
    case 33:
        out->unk_2000 = 1;
        if (count <= 4)
            return false;
        if (e[-1] != ')' || tok[0] != '(' || tok[1] != '-' || tok[2] != '-')
            return false;
        start = tok + 3;
        end = e - 1;
        goto copyName;
    case 34:
        if (!func_0804bfcc(tok, "vi01") && !func_0804bfcc(tok, "vi1")) {
            func_0804ad84("oDecode:: operand has to be vi01 for this instruction..\n");
            return false;
        }
        mstrcpy(out->unk_800, tok);
        mstrcpy(out->name, tok);
        out->unk_2000 = 1;
        return true;
    case 35: {
        d = out->name;
        out->unk_200c = 1;
        s = tok;
        while (*s && *s != '(')
            *d++ = *s++;
        *d = 0;
        if (*s) {
            if (*s != '(') {
                func_0804ad38("oDecodeToken: bad fl4s\n");
                return false;
            }
            s++;
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
            *d = 0;
        }
        if (newSyntax) {
            out->unk_2000 = 15;
            return true;
        }
        *pbad = false;
        bool ok = func_08051f08(out->name, &out->unk_2000, pbad);
        if (*pbad) {
            func_0804ad84("bad mod bits in operand\n");
            return false;
        }
        if (!ok)
            out->unk_2000 = 15;
        if (out->unk_2000 == -1) {
            func_0804ad38("oDecodeToken: bad FL4S bits\n");
            return false;
        }
        func_0804c0c8(out->name);
        return true;
    }
    case 36:
        d = out->name;
        out->unk_200c = 1;
        s = tok;
        while (*s && *s != '(')
            *d++ = *s++;
        *d = 0;
        if (*s) {
            if (*s != '(') {
                func_0804ad38("oDecodeToken: bad ints\n");
                return false;
            }
            s++;
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
            *d = 0;
        }
        out->unk_2000 = 1;
        return true;
    case 38:
        if (func_08051f78(tok, pmask)) {
            out->unk_2008 = 1;
            if (func_0804bf50(mask) != 1) {
                func_0804ad84("bad [x,y,z,w] token for scalar\n");
                return false;
            }
            out->unk_2000 = mask;
            char *s = tok;
            char *d = out->name;
            while (*s)
                *d++ = *s++;
            *d = 0;
            do
                d--;
            while (*d != '[');
            *d = 0;
            return true;
        }
        if (func_0804bfcc(tok, "q")) {
            out->unk_2008 = 1;
            out->unk_2000 = 1;
            mstrcpy(out->name, "Q");
            return true;
        }
        if (func_0804bfcc(tok, "i")) {
            out->unk_2008 = 1;
            out->unk_2000 = 1;
            mstrcpy(out->name, "I");
            return true;
        }
        out->unk_2000 = 15;
        out->unk_2008 = 2;
        mstrcpy(out->name, tok);
        return true;
    case 39:
        out->unk_2000 = 1;
        mstrcpy(out->name, tok);
        return true;
    case 40:
        out->unk_2000 = 3;
        mstrcpy(out->name, tok);
        return true;
    case 41:
        out->unk_2000 = 15;
        mstrcpy(out->name, tok);
        return true;
    case 42:
        d = out->name;
        out->unk_200c = 1;
        s = tok;
        while (*s && *s != '(')
            *d++ = *s++;
        *d = 0;
        if (*s) {
            if (*s != '(') {
                func_0804ad38("oDecodeToken: bad gprs\n");
                return false;
            }
            s++;
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
            *d = 0;
        }
        out->unk_2000 = 15;
        return true;
    case 43:
        mstrcpy(out->name, tok);
        out->unk_2000 = 1;
        return true;
    case 44:
        d = out->name;
        out->unk_200c = 1;
        s = tok;
        while (*s && *s != '(')
            *d++ = *s++;
        *d = 0;
        if (*s) {
            if (*s != '(') {
                func_0804ad38("oDecodeToken: bad fprs\n");
                return false;
            }
            s++;
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
            *d = 0;
        }
        out->unk_2000 = 1;
        return true;
    case 29:
    case 46:
        if (count <= 2)
            return false;
        out->unk_2000 = 1;
        p = e - 1;
        if (*p != ')')
            return false;
        do
            p--;
        while (*p != '(');
        start = ++p;
        d = out->unk_1000;
        for (s = tok; s != start - 1; )
            *d++ = *s++;
        end = e - 1;
        *d = 0;
        if (!out->unk_1000[0]) {
            out->unk_1000[0] = '0';
            out->unk_1000[1] = 0;
        }
        goto copyName;
    default:
        func_0804ad38("oDecode: general BAD '%s'\n", opKindName(kind));
        return false;
    }
}

const char *opKindName(int kind)
{
    const char *s = "BAD_OPERAND";
    switch (kind) {
    case 0: s = "OP_NULL"; break;
    case 1: s = "OP_V_FL4"; break;
    case 2: s = "OP_V_INT"; break;
    case 3: s = "OP_V_FL1"; break;
    case 4: s = "OP_V_I"; break;
    case 5: s = "OP_V_P"; break;
    case 6: s = "OP_V_nP"; break;
    case 7: s = "OP_V_Q"; break;
    case 8: s = "OP_V_R"; break;
    case 9: s = "OP_V_ACC"; break;
    case 10: s = "OP_V_ACCXYZ"; break;
    case 11: s = "OP_V_FLXYZ"; break;
    case 13: s = "OP_V_FLX"; break;
    case 14: s = "OP_V_FLY"; break;
    case 15: s = "OP_V_FLZ"; break;
    case 16: s = "OP_V_FLW"; break;
    case 17: s = "OP_V_CLIP"; break;
    case 18: case 19: case 20: case 21: s = "OP_V_STAT"; break;
    case 22: s = "OP_V_STAT_DIV"; break;
    case 23: s = "OP_V_STAT_ALU"; break;
    case 24: s = "OP_V_STAT_TST"; break;
    case 25: s = "OP_V_STAT_SET"; break;
    case 26: s = "OP_V_MAC"; break;
    case 27: s = "OP_IMM"; break;
    case 28: s = "OP_V_EA"; break;
    case 29: s = "OP_V_EA_IMM"; break;
    case 30: s = "OP_V_EA_REG"; break;
    case 31: s = "OP_V_EA_IMM_REG"; break;
    case 32: s = "OP_V_EA_INC"; break;
    case 33: s = "OP_V_EA_DEC"; break;
    case 34: s = "OP_V_INT1"; break;
    case 35: s = "OP_V_FL4S"; break;
    case 36: s = "OP_V_INTS"; break;
    case 37: s = "OP_V_MEMREG"; break;
    case 38: s = "OP_V_TBD"; break;
    case 39: s = "OP_C_GPR32"; break;
    case 40: s = "OP_C_GPR64"; break;
    case 41: s = "OP_C_GPR128"; break;
    case 42: s = "OP_C_GPRS"; break;
    case 43: s = "OP_C_FPR"; break;
    case 44: s = "OP_C_FPRS"; break;
    case 45: s = "OP_V_CLIPA"; break;
    case 46: s = "OP_C_EA_IMM"; break;
    case 47: s = "OP_C_HILO0"; break;
    case 48: s = "OP_C_HILO1"; break;
    case 49: s = "OP_C_FSTAT"; break;
    case 50: s = "OP_C_FCOND"; break;
    case 51: s = "OP_C_FACC"; break;
    case 52: s = "OP_C_FDIV"; break;
    case 53: s = "OP_C_SHFTA"; break;
    case 54: s = "OP_C_GPR64_31($ra)"; break;
    case 55: s = "OP_EXT_DEP"; break;
    }
    return s;
}

int func_08057a34(int kind)
{
    switch (kind) {
    case 1:
        return 9;
    case 3:
        return 9;
    case 11:
        return 9;
    case 13:
        return 9;
    case 14:
        return 9;
    case 15:
        return 9;
    case 16:
        return 9;
    case 35:
        return 9;
    default:
    case 55:
        return 0;
    case 0:
        return 0;
    case 2:
        return 8;
    case 28:
        return 8;
    case 29:
        return 8;
    case 30:
        return 8;
    case 31:
        return 8;
    case 32:
        return 8;
    case 33:
        return 8;
    case 34:
        return 8;
    case 36:
        return 8;
    case 37:
        return 11;
    case 43: case 44:
        return 2;
    case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 17:
    case 18: case 19: case 20: case 21: case 26: case 45:
        return 10;
    case 47: case 48: case 49: case 50: case 51: case 52: case 53:
        return 6;
    case 39: case 40: case 41: case 42: case 46: case 54:
        return 1;
    }
    return 0;
}

Instr *Operand::getOwner()
{
    Instr *owner = 0;
    if (unk_26 >= 0) {
        int off = (int)&((Instr *)0)->unk_38[unk_26];
        Instr *p = (Instr *)((char *)this - off);
        if (p->unk_138b < unk_26) {
            func_0804ad38("operand: getOwner - bad instruction?\n");
            func_0804adf0(-1);
        }
        owner = p;
    }
    return owner;
}

OpSlot *Operand::getPart(int part)
{
    if (part < 0 || part > 3) {
        func_0804ad38("failed to get operand part - out of range!!\n", part);
        func_0804adf0(-1);
    }
    if (!unk_18) {
        func_0804ad38("operand had no register for get part\n");
        func_0804adf0(-1);
    }
    if (part >= unk_18->unk_28) {
        func_0804ad38("operand::getPart: out of range for register\n");
        func_0804adf0(-1);
    }
    bool has = (unk_07 >> part) & 1;
    if (has)
        return &unk_2c[part];
    return 0;
}

char *Operand::func_08057b90(bool flag, char *buf)
{
    if (!buf)
        return 0;
    if (unk_08)
        return buf;
    switch (unk_04) {
    case 38: buf += func_0804adbc(buf, "vec_reg"); break;
    case 1:
        if (flag)
            buf += func_0804adbc(buf, "vf_reg");
        else
            buf += func_0804adbc(buf, "vf_reg[xyzw]");
        break;
    case 2: buf += func_0804adbc(buf, "vi_reg"); break;
    case 3: buf += func_0804adbc(buf, "vf_reg[x/y/z/w]"); break;
    case 4: buf += func_0804adbc(buf, "I"); break;
    case 5: buf += func_0804adbc(buf, "P"); break;
    case 7: buf += func_0804adbc(buf, "Q"); break;
    case 8: buf += func_0804adbc(buf, "R"); break;
    case 9: buf += func_0804adbc(buf, "ACC"); break;
    case 10: buf += func_0804adbc(buf, "ACCxyz"); break;
    case 11: buf += func_0804adbc(buf, "vf_regxyz"); break;
    case 13: buf += func_0804adbc(buf, "vf_regx"); break;
    case 14: buf += func_0804adbc(buf, "vf_regy"); break;
    case 15: buf += func_0804adbc(buf, "vf_regz"); break;
    case 16: buf += func_0804adbc(buf, "vf_regw"); break;
    case 27: buf += func_0804adbc(buf, "IMMEDIATE(%s)", func_0806a33c(unk_05)); break;
    case 28: buf += func_0804adbc(buf, "(vi_reg)"); break;
    case 29: buf += func_0804adbc(buf, "OFFSET(vi_reg)"); break;
    case 30: buf += func_0804adbc(buf, "(vi_reg)[x/y/z/w]"); break;
    case 31: buf += func_0804adbc(buf, "OFFSET(vi_reg)[x/y/z/w]"); break;
    case 32: buf += func_0804adbc(buf, "(vi_reg++)"); break;
    case 33: buf += func_0804adbc(buf, "(--vi_reg)"); break;
    case 34: buf += func_0804adbc(buf, "vi01"); break;
    case 35: buf += func_0804adbc(buf, "vf_reg[.xyzw][(base_reg)]"); break;
    case 36: buf += func_0804adbc(buf, "vi_reg[(base_reg)]"); break;
    case 39: case 40: case 41: buf += func_0804adbc(buf, "gpr"); break;
    case 42: buf += func_0804adbc(buf, "gpr(reg)"); break;
    case 43: buf += func_0804adbc(buf, "fpr"); break;
    case 44: buf += func_0804adbc(buf, "fpr(reg)"); break;
    case 46: buf += func_0804adbc(buf, "OFFSET(gpr)"); break;
    case 54: buf += func_0804adbc(buf, "$ra"); break;
    default: break;
    }
    return buf;
}

bool func_08057d60(int kind)
{
    return kind == 43 || kind == 3 || kind == 13 || kind == 14 ||
           kind == 15 || kind == 16 || kind == 5 || kind == 7;
}

bool func_08057d9c(int kind)
{
    return kind == 1 || kind == 9;
}

int Operand::getRegFrom(Operand *src)
{
    int a = func_08057a34(unk_04);
    int b = func_08057a34(src->unk_04);
    if (b != a) {
        func_0804ad38("incompatible type request getting type for operand\n");
        return 0;
    }
    unk_18 = src->unk_18;
    unk_14 = src->unk_14;
    unk_10 = src->unk_10;
    if (!unk_18) {
        func_0804ad38("no register set in get reg from\n");
        return 0;
    }
    if (unk_18->unk_24 != b) {
        func_0804ad38("register not of correct type\n");
        return 0;
    }
    return func_080563f8();
}

const char *Operand::func_08057e40()
{
    if (!unk_18)
        return "NO_REG";
    return unk_18->func_0805a8e0();
}
