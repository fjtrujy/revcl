// Operand, CIE group 4. Separate TU from Instr.cpp: its .rodata (strings from
// 0x08090848, jump tables from 0x080915e4) follows Instr's jump tables.
#include "Instr.h"
#include "infoList.h"
#include "os.h"
#include "VUStr.h"

int func_0804bf50(unsigned int);
bool asciiCaseEqual(const char *a, const char *b);
void func_0804c0c8(char *s);
int mstrlen(const char *s);

Operand::Operand()
{
    operandIndex = -1;
    assignedRegister = 0;
    isImplicit = 0;
    operandKind = 0;
    componentMask = 0;
    additionalLatency = 0;
    forcedBaseRegister = 0;
    registerWeb = 0;
    unk_25 = -1;
    unk_24 = -1;
    pairedOperandIndexPlusOne = 0;
    originalRegister = 0;
    unk_28 = 0;
    externalComponentMask = 0;
    externalPartMask = 0;
    operandStatus = 0;
    int i;
    for (i = 0; i != 4; i++) {
        if (!parts[i].unk_54.clear()) {
            reportError("failed to flow reset part %d\n", i);
            break;
        }
    }
}

Operand::~Operand()
{
}

bool Operand::validateKindAndMask()
{
    switch (operandKind) {
    case 1:
    case 35:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FL4 or V_FL4S/ MEM!\n");
            return false;
        }
        if ((componentMask & 0xf) == componentMask)
            return true;
        reportError("not all!\n");
        return false;
    case 3:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FL1!\n");
            return false;
        }
        if (componentMask == 1 || componentMask == 2 || componentMask == 4 || componentMask == 8)
            return true;
        reportError("bad mod bits for FL1..%d\n", componentMask);
        return false;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
        if (!assignedRegister) {
            reportError("reg no reg set - V_P/Q/R/I/nQ!\n");
            return false;
        }
        if (componentMask == 1)
            return true;
        reportError("bad mod bits for V_P/Q/R/I\n");
        return false;
    case 9:
        if (!assignedRegister) {
            reportError("reg no reg set - V_ACC!\n");
            return false;
        }
        if ((componentMask & 0xf) == componentMask)
            return true;
        reportError("bad mod bits..hmm..out of range for acc\n");
        return false;
    case 10:
        if (!assignedRegister) {
            reportError("reg no reg set - V_ACCYZ!\n");
            return false;
        }
        if (componentMask == 7)
            return true;
        reportError("xyz not set to ACCXYZ!\n");
        return false;
    case 11:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLXYZ!\n");
            return false;
        }
        if (componentMask == 7)
            return true;
        reportError("xyz not set to XYZ!\n");
        return false;
    case 13:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLX!\n");
            return false;
        }
        if (componentMask == 1)
            return true;
        reportError("bad mod bits for FLX\n");
        return false;
    case 14:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLY!\n");
            return false;
        }
        if (componentMask == 2)
            return true;
        reportError("bad mod bits for FLY\n");
        return false;
    case 15:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLZ!\n");
            return false;
        }
        if (componentMask == 4)
            return true;
        reportError("bad mod bits for FLZ\n");
        return false;
    case 16:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLW!\n");
            return false;
        }
        if (componentMask == 8)
            return true;
        reportError("bad mod bits for FLW\n");
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
        return immediateValue.isValid() != 0;
    case 28:
        if (componentMask != 1)
            return false;
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 46:
        return assignedRegister != 0;
    case 2:
    case 36:
        if (!assignedRegister) {
            reportError("reg no reg set - V_INT/INTS!\n");
            return false;
        }
        if (componentMask == 1)
            return true;
        reportError("bad mod bits for int/ints!\n");
        return false;
    case 37:
        if (assignedRegister)
            return true;
        reportError("ramreg.. not ok.. no m_Reg\n");
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
        if (assignedRegister)
            return true;
        reportError("not ok type %d.. no reg!\n", operandKind);
        return false;
    }
    return false;
}

void Operand::diagnoseOperand(int)
{
    if (validateKindAndMask()) {
        bool ok = true;
        switch (operandKind) {
        case 0:
            break;
        case 28:
        case 29:
        case 30:
        case 32:
        case 33:
        case 46:
            ok = immediateValue.isValid();
            break;
        case 27:
            immediateValue.dump();
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
    if (op->assignedRegister) {
        if (op->assignedRegister == op->assignedRegister->ownerPool->zeroRegister)
            z = 1;
    }
    return z;
}

char *Operand::Emit(char *buf, bool, bool symbolic)
{
    const char *rn = 0;
    if (isImplicit)
        return 0;
    if (!validateKindAndMask())
        return 0;
    if (assignedRegister) {
        int z = assignedRegister == assignedRegister->ownerPool->zeroRegister;
        if ((char)z != (char)operandZeroValue(this)) {
            reportError("Operand::Emit - zero mismatch..%s but %d\n", assignedRegister->registerName(), operandZeroValue(this));
            return 0;
        }
    }
    if (symbolic) {
        if (assignedRegister) {
            if (!originalRegister) {
                reportError("no original register set register is %s\n", assignedRegister->registerName());
                return 0;
            }
            rn = originalRegister->registerName();
        }
    } else {
        RegPoolEntry *reg = assignedRegister;
        if (reg) {
            if (reg->isVirtual) {
                reportError("non symbolic emit.. but found a virtual register in an operand!\n");
                return 0;
            }
            rn = reg->registerName();
            reg->usedSlotMask |= componentMask;
        }
    }
    switch (operandKind) {
    case 13:
    emit_x:
        buf += formatIntoBuffer(buf, "%sx", rn);
        break;
    case 3:
        if (componentMask & 1)
            goto emit_x;
        if (componentMask & 2)
            goto emit_y;
        if (componentMask & 4)
            goto emit_z;
        if (componentMask & 8)
            goto emit_w;
        return 0;
    case 14:
    emit_y:
        buf += formatIntoBuffer(buf, "%sy", rn);
        break;
    case 4:
        buf += formatIntoBuffer(buf, "I");
        break;
    case 5:
        buf += formatIntoBuffer(buf, "P");
        break;
    case 6:
        buf += formatIntoBuffer(buf, "nP");
        break;
    case 7:
        buf += formatIntoBuffer(buf, "Q");
        break;
    case 15:
    emit_z:
        buf += formatIntoBuffer(buf, "%sz", rn);
        break;
    case 8:
        buf += formatIntoBuffer(buf, "R");
        break;
    case 9:
        buf += formatIntoBuffer(buf, "ACC");
        break;
    case 10:
        buf += formatIntoBuffer(buf, "ACCxyz");
        break;
    case 16:
    emit_w:
        buf += formatIntoBuffer(buf, "%sw", rn);
        break;
    case 11:
        buf += formatIntoBuffer(buf, "%sxyz", rn);
        break;
    case 1:
    case 2:
        buf += formatIntoBuffer(buf, "%s", rn);
        break;
    case 39:
    case 40:
    case 41:
    case 42:
        buf += formatIntoBuffer(buf, "%s", rn);
        break;
    case 43:
    case 44:
        buf += formatIntoBuffer(buf, "%s", rn);
        break;
    case 54:
        buf += formatIntoBuffer(buf, "%s", rn);
        break;
    case 34:
        buf += formatIntoBuffer(buf, "%s", rn);
        break;
    case 27: {
        mValue *imm = &immediateValue;
        if (!imm->isNumeric()) {
            char *s = imm->asChars();
            if (!s) {
                reportError("failed to get chars from immediate operand\n");
                return 0;
            }
            buf += formatIntoBuffer(buf, "%s", s);
        } else {
            if (decodeImmediateCategory(unk_05) == 2)
                buf += formatIntoBuffer(buf, "%d", imm->i);
            else
                buf += formatIntoBuffer(buf, "0x%08x", imm->u);
        }
        break;
    }
    case 29:
    case 46: {
        if (!rn) {
            reportError("null rn - abort _BLAH_EA_IMM\n");
            return 0;
        }
        char *s = immediateValue.asChars();
        if (!s) {
            reportError("failed to get chars from ximm blah_ea_imm\n");
            return 0;
        }
        buf += formatIntoBuffer(buf, "%s(%s)", s, rn);
        break;
    }
    case 31: {
        char *s = immediateValue.asChars();
        if (!s)
            reportError("failed to get chars from ximm for v_ea_imm\n");
        buf += formatIntoBuffer(buf, "%s", s);
    }
    case 30:
        if (!rn) {
            reportError("null rn - abort _V_EA_REG\n");
            return 0;
        }
    case 28:
        buf += formatIntoBuffer(buf, "(%s)", rn);
        break;
    case 32:
        if (!rn) {
            reportError("null rn - abort _V_EA_INC\n");
            return 0;
        }
        buf += formatIntoBuffer(buf, "(%s++)", rn);
        break;
    case 33:
        if (!rn) {
            reportError("null rn - abort _V_EA_DEC\n");
            return 0;
        }
        buf += formatIntoBuffer(buf, "(--%s)", rn);
        break;
    case 37:
        if (!rn) {
            reportError("emit ramreg - no rn\n");
            return 0;
        }
        buf += formatIntoBuffer("ramreg-[%s]", rn);
        break;
    case 55:
    default:
        reportError("asked to emit an unknown/unemittable operand\n");
        return 0;
    }
    return buf;
}

bool Operand::resetFlowState()
{
    int i = 0;
    unk_28 = 0;
    externalComponentMask = 0;
    externalPartMask = 0;
    operandStatus = 0;
    for (; i != 4; i++) {
        if (!parts[i].unk_54.clear()) {
            reportError("failed to flow reset part %d\n", i);
            return false;
        }
    }
    return true;
}

static inline bool resetOperandLock(OperandLock *lock)
{
    return lock->resetLock();
}

bool Operand::resetOptimizationSlots()
{
    for (int i = 0; i != 4; i++) {
        OpSlot *s = &parts[i];
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
        reportError("odecode for a V_STAT\n");
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
        reportError("fell through building an implicit operand of type %s\n", opKindName(kind));
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
    int uA;
    int uB;
    char zG;
    if (mstrlen(tok) > 2047) {
        reportError("odecodeToken: token was too long\n");
        return false;
    }

    char *t = tok;
    char c;
    while ((c = *t) && c != ':')
        t++;
    if (c == ':') {
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
        uA = ((int *)tok)[1];
        uB = ((int *)tok)[2];
        if (td == out->unk_1800) {
            ((int *)tok)[1] = uA;
            ((int *)tok)[2] = uB;
            reportWarning("ignoring empty tag\n");
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
    case 54: {
        int *spk = &out->unk_2004;
        int svx = *spk;
        int zs = (unsigned char)tok[2];
        zs >>= 4;
        zs >>= 4;
        if (zs)
            return false;
        *spk = svx;
        reportError("encountered a force bind to $ra in an explicit operand\n");
        return false;
    }
    case 35: {
        d = out->name;
        out->unk_200c = 1;
        e = tok;
        while (*e && *e != '(')
            *d++ = *e++;
        *d = 0;
        if (*e) {
            if (*e != '(') {
                reportError("oDecodeToken: bad fl4s\n");
                return false;
            }
            e++;
            d = out->unk_800;
            while (*e && *e != ')')
                *d++ = *e++;
            *d = 0;
        }
        if (newSyntax) {
            {
                field = ((int *)tok)[kind];
                int zsk0_c35 = (unsigned char)tok[2];
                zsk0_c35 >>= 4;
                zsk0_c35 >>= 4;
                if (zsk0_c35)
                    return false;
                ((int *)tok)[kind] = field;
            }
            out->unk_2000 = 15;
            return true;
        } else {
            *pbad = false;
            bool ok = func_08051f08(out->name, &out->unk_2000, pbad);
            if (*pbad) {
                reportUserMessage("bad mod bits in operand\n");
                return false;
            }
            if (!ok) {
                int svk_35b = ((int *)tok)[kind];
                int zsk_35b = (unsigned char)tok[2];
                zsk_35b >>= 4;
                zsk_35b >>= 4;
                if (zsk_35b)
                    return false;
                ((int *)tok)[kind] = svk_35b;
                out->unk_2000 = 15;
            }
            if (out->unk_2000 == -1) {
                reportError("oDecodeToken: bad FL4S bits\n");
                return false;
            }
            func_0804c0c8(out->name);
        }
        return true;
    }
    case 42:
        d = out->name;
        out->unk_200c = 1;
        s = tok;
        while (*s && *s != '(')
            *d++ = *s++;
        *d = 0;
        if (*s) {
            if (*s != '(') {
                reportError("oDecodeToken: bad gprs\n");
                return false;
            }
            s++;
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
            *d = 0;
        }
        {
            int svO_c42 = ((int *)tok)[kind];
            int svI_c42 = ((int *)tok)[1];
            int zp0_c42 = (unsigned char)tok[2];
            zp0_c42 >>= 4;
            zp0_c42 >>= 4;
            if (zp0_c42)
                return false;
            ((int *)tok)[1] = svI_c42;
            ((int *)tok)[kind] = svO_c42;
        }
        out->unk_2000 = 15;
        return true;
    case 1:
        {
            int sv1_c1 = ((int *)tok)[kind];
            int zp1_c1 = (unsigned char)tok[2];
            zp1_c1 >>= 4;
            zp1_c1 >>= 4;
            if (zp1_c1)
                return false;
            ((int *)tok)[kind] = sv1_c1;
        }
        if (newSyntax) {
            out->unk_2000 = 15;
            mstrcpy(out->name, tok);
        } else {
            uB = func_08052100(tok);
            out->unk_2000 = uB;
            if (!uB)
                out->unk_2000 = 15;
            if (out->unk_2000 == -1) {
                int *spk1 = &out->unk_2004;
                int svx1 = *spk1;
                int zs1 = (unsigned char)tok[2];
                zs1 >>= 4;
                zs1 >>= 4;
                if (zs1)
                    return false;
                *spk1 = svx1;
                reportError("oDecodeToken: bad FL4 bits\n");
                return false;
            }
            mstrcpy(out->name, tok);
            func_0804c0c8(out->name);
        }
        return true;
    case 2:
        mstrcpy(out->name, tok);
        out->unk_2000 = 1;
        return true;
    case 38:
        {
            int svk_c38 = ((int *)tok)[kind];
            int zsk_c38 = (unsigned char)tok[2];
            zsk_c38 >>= 4;
            zsk_c38 >>= 4;
            if (zsk_c38)
                return false;
            ((int *)tok)[kind] = svk_c38;
        }
        if (func_08051f78(tok, pmask)) {
            out->unk_2008 = 1;
            if (func_0804bf50(mask) != 1) {
                reportUserMessage("bad [x,y,z,w] token for scalar\n");
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
        if (asciiCaseEqual(tok, "q")) {
            out->unk_2008 = 1;
            out->unk_2000 = 1;
            mstrcpy(out->name, "Q");
            return true;
        }
        if (asciiCaseEqual(tok, "i")) {
            out->unk_2008 = 1;
            out->unk_2000 = 1;
            mstrcpy(out->name, "I");
            return true;
        }
        out->unk_2000 = 15;
        out->unk_2008 = 2;
        mstrcpy(out->name, tok);
        return true;
    case 3:
        {
            field = ((int *)tok)[kind];
            int zsk0_c3 = (unsigned char)tok[2];
            zsk0_c3 >>= 4;
            zsk0_c3 >>= 4;
            if (zsk0_c3)
                return false;
            ((int *)tok)[kind] = field;
        }
        if (newSyntax) {
            if (!func_08051f78(tok, pmask) || func_0804bf50(mask) != 1) {
                reportError("oDecodeToken: [x/y/z/w] token for new syntax\n");
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
        } else {
            mask = func_08052100(tok);
            if (func_0804bf50(mask) != 1) {
                reportError("oDecodeToken: bad FL1 bits\n");
                return false;
            }
            out->unk_2000 = mask;
            mstrcpy(out->name, tok);
            func_0804c0c8(out->name);
        }
        return true;
    case 4:
        if (count != 1 || (*tok != 'i' && *tok != 'I')) {
            reportError("oDecode:: arg I\n");
            return false;
        }
        out->unk_2000 = 1;
        out->name[0] = 'I';
        out->name[1] = 0;
        return true;
    case 5:
        {
            int svk_c5 = ((int *)tok)[kind];
            int zsk_c5 = (unsigned char)tok[2];
            zsk_c5 >>= 4;
            zsk_c5 >>= 4;
            if (zsk_c5)
                return false;
            ((int *)tok)[kind] = svk_c5;
        }
        if (count != 1 || (*tok != 'p' && *tok != 'P'))
            reportError("oDecode:: arg P\n");
        out->unk_2000 = 1;
        out->name[0] = 'P';
        out->name[1] = 0;
        return true;
    case 7:
        if (count != 1 || (*tok != 'q' && *tok != 'Q')) {
            reportError("oDecode:: arg Q\n");
            return false;
        }
        out->unk_2000 = 1;
        out->name[0] = 'Q';
        out->name[1] = 0;
        return true;
    case 8:
        {
            field = ((int *)tok)[kind];
            int zsk0_c8 = (unsigned char)tok[2];
            zsk0_c8 >>= 4;
            zsk0_c8 >>= 4;
            if (zsk0_c8)
                return false;
            ((int *)tok)[kind] = field;
        }
        if (count != 1 || (*tok != 'r' && *tok != 'R')) {
            reportError("oDecode:: arg R\n");
            return false;
        }
        out->unk_2000 = 1;
        out->name[0] = 'R';
        out->name[1] = 0;
        return true;
    case 9: {
        {
            field = ((int *)tok)[kind];
            int zsk0_c9p = (unsigned char)tok[2];
            zsk0_c9p >>= 4;
            zsk0_c9p >>= 4;
            if (zsk0_c9p)
                return false;
            ((int *)tok)[kind] = field;
        }
        bad = false;
        int *pm9 = &out->unk_2000;
        if (!func_08051f08(tok, pm9, &bad))
            out->unk_2000 = 15;
        if (out->unk_2000 == -1) {
            {
            int svk_c9 = ((int *)tok)[kind];
            int zsk_c9 = (unsigned char)tok[2];
            zsk_c9 >>= 4;
            zsk_c9 >>= 4;
            if (zsk_c9)
                return false;
            ((int *)tok)[kind] = svk_c9;
        }
            reportError("oDecodeToken: bad FL4 bits\n");
            return false;
        }
        mstrcpy(out->name, "ACC");
        return true;
    }
    case 10:
        mask = func_08052100(tok);
        if (mask && mask != 7) {
            reportError("bad mod bits for ACCxyz\n");
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
                    reportUserMessage("instruction and operand have mismatching field select\n");
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
        } else {
            mask = func_08052100(tok);
            if (mask && mask != field) {
                reportError("oDecode: bad FL?\n");
                return false;
            }
            out->unk_2000 = field;
            mstrcpy(out->name, tok);
            func_0804c0c8(out->name);
        }
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
    case 27:
        mstrcpy(out->unk_1000, tok);
        return true;
    case 28: {
        out->unk_2000 = 1;
        uA = (unsigned char)tok[1];
        if (count <= 2)
            return false;
        zG = tok[uA];
        int z0 = (unsigned char)*tok;
        z0 >>= 4;
        z0 >>= 4;
        if (z0)
            return false;
        tok[uA] = zG;
        char *r = e - 1;
        if (*r != ')' || *tok != '(')
            return false;
        start = tok + 1;
        end = r;
        goto copyName;
    }
    case 36:
        d = out->name;
        out->unk_200c = 1;
        e = tok;
        while (*e && *e != '(')
            *d++ = *e++;
        *d = 0;
        if (*e) {
            if (*e != '(') {
                reportError("oDecodeToken: bad ints\n");
                return false;
            }
            e++;
            d = out->unk_800;
            while (*e && *e != ')')
                *d++ = *e++;
            *d = 0;
        }
        {
            int svO_c36 = ((int *)tok)[kind];
            int svI_c36 = ((int *)tok)[1];
            int zp0_c36 = (unsigned char)tok[2];
            zp0_c36 >>= 4;
            zp0_c36 >>= 4;
            if (zp0_c36)
                return false;
            ((int *)tok)[1] = svI_c36;
            ((int *)tok)[kind] = svO_c36;
        }
        out->unk_2000 = 1;
        return true;
    case 43:
        {
            int svO_c43 = ((int *)tok)[kind];
            int svI_c43 = ((int *)tok)[1];
            int zp0_c43 = (unsigned char)tok[2];
            zp0_c43 >>= 4;
            zp0_c43 >>= 4;
            if (zp0_c43)
                return false;
            ((int *)tok)[1] = svI_c43;
            ((int *)tok)[kind] = svO_c43;
        }
        mstrcpy(out->name, tok);
        out->unk_2000 = 1;
        return true;
    case 30: {
        out->unk_2000 = 1;
        uB = (unsigned char)tok[2];
        if (count <= 3)
            return false;
        zG = tok[uB];
        int z1 = (unsigned char)*tok;
        z1 >>= 4;
        z1 >>= 4;
        if (z1)
            return false;
        tok[uB] = zG;
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
    }
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
        s = start;
        for (d = out->name; s != end; )
            *d++ = *s++;
        *d = 0;
        return true;
    case 32: {
        out->unk_2000 = 1;
        if (count <= 4)
            return false;
        if (e[-1] != ')' || e[-2] != '+')
            return false;
        char *r = e - 3;
        if (*r != '+' || *tok != '(')
            return false;
        start = tok + 1;
        end = r;
        goto copyName;
    }
    case 33: {
        out->unk_2000 = 1;
        if (count <= 4)
            return false;
        char *r = e - 1;
        if (*r != ')' || tok[0] != '(' || tok[1] != '-' || tok[2] != '-')
            return false;
        start = tok + 3;
        end = r;
        goto copyName;
    }
    case 34:
        {
            int svk_c34 = ((int *)tok)[kind];
            int zsk_c34 = (unsigned char)tok[2];
            zsk_c34 >>= 4;
            zsk_c34 >>= 4;
            if (zsk_c34)
                return false;
            ((int *)tok)[kind] = svk_c34;
        }
        if (!asciiCaseEqual(tok, "vi01") && !asciiCaseEqual(tok, "vi1")) {
            reportUserMessage("oDecode:: operand has to be vi01 for this instruction..\n");
            return false;
        }
        mstrcpy(out->unk_800, tok);
        mstrcpy(out->name, tok);
        out->unk_2000 = 1;
        return true;
    case 26:
        reportError("explicit mac register - but I'm writing ASTATUS here??\n");
        exitAfterCleanup(1);
        out->unk_2000 = 1;
        mstrcpy(out->name, "ASTATUS");
        return true;
    case 39:
        {
            int svk_c39 = ((int *)tok)[kind];
            int zsk_c39 = (unsigned char)tok[2];
            zsk_c39 >>= 4;
            zsk_c39 >>= 4;
            if (zsk_c39)
                return false;
            ((int *)tok)[kind] = svk_c39;
        }
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
    case 44:
        d = out->name;
        out->unk_200c = 1;
        s = tok;
        while (*s && *s != '(')
            *d++ = *s++;
        *d = 0;
        if (*s) {
            if (*s != '(') {
                reportError("oDecodeToken: bad fprs\n");
                return false;
            }
            s++;
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
            *d = 0;
        }
        {
            int sv1_c44 = ((int *)tok)[kind];
            int zp1_c44 = (unsigned char)tok[2];
            zp1_c44 >>= 4;
            zp1_c44 >>= 4;
            if (zp1_c44)
                return false;
            ((int *)tok)[kind] = sv1_c44;
        }
        out->unk_2000 = 1;
        return true;
    case 29:
    case 46: {
        if (count <= 2)
            return false;
        out->unk_2000 = 1;
        p = e - 1;
        if (*p != ')')
            return false;
        do
            p--;
        while (*p != '(');
        start = p + 1;
        p = tok;
        char *stop = start - 1;
        end = e - 1;
        d = out->unk_1000;
        for (s = tok; s != stop; )
            *d++ = *s++;
        *d = 0;
        if (!out->unk_1000[0]) {
            out->unk_1000[0] = '0';
            out->unk_1000[1] = 0;
        }
        goto copyName;
    }
    default: {
        s = tok + kind;
        int zd = (unsigned char)tok[2];
        zd >>= 4;
        zd >>= 4;
        if (zd) {
            d = out->unk_800;
            while (*s && *s != ')')
                *d++ = *s++;
        }
        reportError("oDecode: general BAD '%s'\n", opKindName(kind));
        e = 0;
        q = 0;
        break;
    }
    }
    return false;
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

int registerClassForOperandKind(int kind)
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
    if (operandIndex >= 0) {
        VclPtrWord off = (VclPtrWord)&((Instr *)0)->operands[operandIndex];
        Instr *p = (Instr *)((char *)this - off);
        if (p->operandCount < operandIndex) {
            reportError("operand: getOwner - bad instruction?\n");
            exitAfterCleanup(-1);
        }
        owner = p;
    }
    return owner;
}

OpSlot *Operand::getPart(int part)
{
    if (part < 0 || part > 3) {
        reportError("failed to get operand part - out of range!!\n", part);
        exitAfterCleanup(-1);
    }
    if (!assignedRegister) {
        reportError("operand had no register for get part\n");
        exitAfterCleanup(-1);
    }
    if (part >= assignedRegister->partCount) {
        reportError("operand::getPart: out of range for register\n");
        exitAfterCleanup(-1);
    }
    bool has = (componentMask >> part) & 1;
    if (has)
        return &parts[part];
    return 0;
}

char *Operand::emitExpectedOperandType(bool flag, char *buf)
{
    if (!buf)
        return 0;
    if (isImplicit)
        return buf;
    switch (operandKind) {
    case 38: buf += formatIntoBuffer(buf, "vec_reg"); break;
    case 1:
        if (flag)
            buf += formatIntoBuffer(buf, "vf_reg");
        else
            buf += formatIntoBuffer(buf, "vf_reg[xyzw]");
        break;
    case 2: buf += formatIntoBuffer(buf, "vi_reg"); break;
    case 3: buf += formatIntoBuffer(buf, "vf_reg[x/y/z/w]"); break;
    case 4: buf += formatIntoBuffer(buf, "I"); break;
    case 5: buf += formatIntoBuffer(buf, "P"); break;
    case 7: buf += formatIntoBuffer(buf, "Q"); break;
    case 8: buf += formatIntoBuffer(buf, "R"); break;
    case 9: buf += formatIntoBuffer(buf, "ACC"); break;
    case 10: buf += formatIntoBuffer(buf, "ACCxyz"); break;
    case 11: buf += formatIntoBuffer(buf, "vf_regxyz"); break;
    case 13: buf += formatIntoBuffer(buf, "vf_regx"); break;
    case 14: buf += formatIntoBuffer(buf, "vf_regy"); break;
    case 15: buf += formatIntoBuffer(buf, "vf_regz"); break;
    case 16: buf += formatIntoBuffer(buf, "vf_regw"); break;
    case 27: buf += formatIntoBuffer(buf, "IMMEDIATE(%s)", immediateTypeName(unk_05)); break;
    case 28: buf += formatIntoBuffer(buf, "(vi_reg)"); break;
    case 29: buf += formatIntoBuffer(buf, "OFFSET(vi_reg)"); break;
    case 30: buf += formatIntoBuffer(buf, "(vi_reg)[x/y/z/w]"); break;
    case 31: buf += formatIntoBuffer(buf, "OFFSET(vi_reg)[x/y/z/w]"); break;
    case 32: buf += formatIntoBuffer(buf, "(vi_reg++)"); break;
    case 33: buf += formatIntoBuffer(buf, "(--vi_reg)"); break;
    case 34: buf += formatIntoBuffer(buf, "vi01"); break;
    case 35: buf += formatIntoBuffer(buf, "vf_reg[.xyzw][(base_reg)]"); break;
    case 36: buf += formatIntoBuffer(buf, "vi_reg[(base_reg)]"); break;
    case 39: case 40: case 41: buf += formatIntoBuffer(buf, "gpr"); break;
    case 42: buf += formatIntoBuffer(buf, "gpr(reg)"); break;
    case 43: buf += formatIntoBuffer(buf, "fpr"); break;
    case 44: buf += formatIntoBuffer(buf, "fpr(reg)"); break;
    case 46: buf += formatIntoBuffer(buf, "OFFSET(gpr)"); break;
    case 54: buf += formatIntoBuffer(buf, "$ra"); break;
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
    int a = registerClassForOperandKind(operandKind);
    int b = registerClassForOperandKind(src->operandKind);
    if (b != a) {
        reportError("incompatible type request getting type for operand\n");
        return 0;
    }
    assignedRegister = src->assignedRegister;
    originalRegister = src->originalRegister;
    forcedBaseRegister = src->forcedBaseRegister;
    if (!assignedRegister) {
        reportError("no register set in get reg from\n");
        return 0;
    }
    if (assignedRegister->registerClass != b) {
        reportError("register not of correct type\n");
        return 0;
    }
    return validateKindAndMask();
}

const char *Operand::getRegisterNameOrNone()
{
    if (!assignedRegister)
        return "NO_REG";
    return assignedRegister->registerName();
}
