// Operand, CIE group 4. Separate TU from Instr.cpp: its .rodata (strings from
// 0x08090848, jump tables from 0x080915e4) follows Instr's jump tables.
#include "Instr.h"
#include "infoList.h"
#include "os.h"
#include "VUStr.h"

int countSetBits(unsigned int);
bool asciiCaseEqual(const char *a, const char *b);
void removeTrailingComponentSuffix(char *s);
int checkedStringLength(const char *s);

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
    sourceTokenEndIndex = -1;
    sourceTokenStartIndex = -1;
    pairedOperandIndexPlusOne = 0;
    originalRegister = 0;
    reductionHazardMask = 0;
    externalComponentMask = 0;
    externalPartMask = 0;
    operandStatus = 0;
    int partIndex;
    for (partIndex = 0; partIndex != 4; partIndex++) {
        if (!parts[partIndex].operandLocations.clear()) {
            reportError("failed to flow reset part %d\n", partIndex);
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

void Operand::diagnoseOperand(int unusedDiagnosticMode)
{
    if (validateKindAndMask()) {
        bool operandValueValid = true;
        switch (operandKind) {
        case 0:
            break;
        case 28:
        case 29:
        case 30:
        case 32:
        case 33:
        case 46:
            operandValueValid = immediateValue.isValid();
            break;
        case 27:
            immediateValue.dump();
            break;
        case 55:
            break;
        default:
            return;
        }
        if (!operandValueValid)
            return;
    }
}

static inline int operandZeroValue(Operand *operand)
{
    int isZeroRegister = 0;
    if (operand->assignedRegister) {
        if (operand->assignedRegister == operand->assignedRegister->ownerPool->zeroRegister)
            isZeroRegister = 1;
    }
    return isZeroRegister;
}

char *Operand::emitOperandText(char *outputBuffer, bool unusedUppercaseFlag, bool symbolic)
{
    const char *registerNameText = 0;
    if (isImplicit)
        return 0;
    if (!validateKindAndMask())
        return 0;
    if (assignedRegister) {
        int isZeroRegister = assignedRegister == assignedRegister->ownerPool->zeroRegister;
        if ((char)isZeroRegister != (char)operandZeroValue(this)) {
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
            registerNameText = originalRegister->registerName();
        }
    } else {
        RegisterEntry *registerEntry = assignedRegister;
        if (registerEntry) {
            if (registerEntry->isVirtual) {
                reportError("non symbolic emit.. but found a virtual register in an operand!\n");
                return 0;
            }
            registerNameText = registerEntry->registerName();
            registerEntry->usedSlotMask |= componentMask;
        }
    }
    switch (operandKind) {
    case 13:
    emit_x:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sx", registerNameText);
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
        outputBuffer += formatIntoBuffer(outputBuffer, "%sy", registerNameText);
        break;
    case 4:
        outputBuffer += formatIntoBuffer(outputBuffer, "I");
        break;
    case 5:
        outputBuffer += formatIntoBuffer(outputBuffer, "P");
        break;
    case 6:
        outputBuffer += formatIntoBuffer(outputBuffer, "nP");
        break;
    case 7:
        outputBuffer += formatIntoBuffer(outputBuffer, "Q");
        break;
    case 15:
    emit_z:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sz", registerNameText);
        break;
    case 8:
        outputBuffer += formatIntoBuffer(outputBuffer, "R");
        break;
    case 9:
        outputBuffer += formatIntoBuffer(outputBuffer, "ACC");
        break;
    case 10:
        outputBuffer += formatIntoBuffer(outputBuffer, "ACCxyz");
        break;
    case 16:
    emit_w:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sw", registerNameText);
        break;
    case 11:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sxyz", registerNameText);
        break;
    case 1:
    case 2:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case 39:
    case 40:
    case 41:
    case 42:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case 43:
    case 44:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case 54:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case 34:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case 27: {
        TypedValue *imm = &immediateValue;
        if (!imm->isNumeric()) {
            char *immediateText = imm->asChars();
            if (!immediateText) {
                reportError("failed to get chars from immediate operand\n");
                return 0;
            }
            outputBuffer += formatIntoBuffer(outputBuffer, "%s", immediateText);
        } else {
            if (decodeImmediateCategory(immediateDataType) == 2)
                outputBuffer += formatIntoBuffer(outputBuffer, "%d", imm->i);
            else
                outputBuffer += formatIntoBuffer(outputBuffer, "0x%08x", imm->u);
        }
        break;
    }
    case 29:
    case 46: {
        if (!registerNameText) {
            reportError("null rn - abort _BLAH_EA_IMM\n");
            return 0;
        }
        char *immediateLiteralText = immediateValue.asChars();
        if (!immediateLiteralText) {
            reportError("failed to get chars from ximm blah_ea_imm\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer(outputBuffer, "%s(%s)", immediateLiteralText, registerNameText);
        break;
    }
    case 31: {
        char *immediateLiteralText = immediateValue.asChars();
        if (!immediateLiteralText)
            reportError("failed to get chars from ximm for v_ea_imm\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", immediateLiteralText);
    }
    case 30:
        if (!registerNameText) {
            reportError("null rn - abort _V_EA_REG\n");
            return 0;
        }
    case 28:
        outputBuffer += formatIntoBuffer(outputBuffer, "(%s)", registerNameText);
        break;
    case 32:
        if (!registerNameText) {
            reportError("null rn - abort _V_EA_INC\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer(outputBuffer, "(%s++)", registerNameText);
        break;
    case 33:
        if (!registerNameText) {
            reportError("null rn - abort _V_EA_DEC\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer(outputBuffer, "(--%s)", registerNameText);
        break;
    case 37:
        if (!registerNameText) {
            reportError("emit ramreg - no rn\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer("ramreg-[%s]", registerNameText);
        break;
    case 55:
    default:
        reportError("asked to emit an unknown/unemittable operand\n");
        return 0;
    }
    return outputBuffer;
}

bool Operand::resetFlowState()
{
    int partIndex = 0;
    reductionHazardMask = 0;
    externalComponentMask = 0;
    externalPartMask = 0;
    operandStatus = 0;
    for (; partIndex != 4; partIndex++) {
        if (!parts[partIndex].operandLocations.clear()) {
            reportError("failed to flow reset part %d\n", partIndex);
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
    for (int partIndex = 0; partIndex != 4; partIndex++) {
        OpSlot *slot = &parts[partIndex];
        resetOperandLock(slot);
        slot->conflictingOutputLockCount = 0;
        slot->outputConflictMarked[0] = 0;
    }
    return true;
}

bool buildImplicitOperand(int kind, ImplicitOperandDescription *out, bool unusedSyntaxFlag)
{
    out->branchLabelText[0] = 0;
    out->immediateText[0] = 0;
    out->componentMask = 0;
    out->forcePhysicalBinding = 0;
    switch (kind) {
    case 10: checkedStringCopy(out->name, "ACC");   out->componentMask = 7; return true;
    case 4:  checkedStringCopy(out->name, "I");     out->componentMask = 1; return true;
    case 5:  checkedStringCopy(out->name, "P");     out->componentMask = 1; return true;
    case 6:  checkedStringCopy(out->name, "tt_nP"); out->componentMask = 1; return true;
    case 7:  checkedStringCopy(out->name, "Q");     out->componentMask = 1; return true;
    case 8:  checkedStringCopy(out->name, "R");     out->componentMask = 1; return true;
    case 9:  checkedStringCopy(out->name, "ACC");   out->componentMask = 15; return true;
    case 17: checkedStringCopy(out->name, "CLIP");  out->componentMask = 1; return true;
    case 18: case 19: case 20: case 21:
        reportError("odecode for a V_STAT\n");
        return false;
    case 55:
        out->name[0] = 0;
        return true;
    case 22: case 23: case 24: case 25:
        out->name[0] = 0;
        return true;
    case 26: checkedStringCopy(out->name, "MAC");   out->componentMask = 1; return true;
    case 28: out->componentMask = 1; return true;
    case 37: checkedStringCopy(out->name, "ramreg"); return true;
    case 54:
        checkedStringCopy(out->physicalRegisterName, "$ra");
        checkedStringCopy(out->name, "$ra");
        out->componentMask = 3;
        return true;
    case 40: out->componentMask = 3; return true;
    case 45: checkedStringCopy(out->name, "CLIPA"); out->componentMask = 1; return true;
    default:
        reportError("fell through building an implicit operand of type %s\n", opKindName(kind));
        out->name[0] = 0;
        out->physicalRegisterName[0] = 0;
        return false;
    case 47: checkedStringCopy(out->name, "HILO0"); out->componentMask = 1; return true;
    case 48: checkedStringCopy(out->name, "HILO1"); out->componentMask = 1; return true;
    case 49: checkedStringCopy(out->name, "FSTAT"); out->componentMask = 1; return true;
    case 50: checkedStringCopy(out->name, "FCOND"); out->componentMask = 1; return true;
    case 51: checkedStringCopy(out->name, "FACC");  out->componentMask = 1; return true;
    case 52: checkedStringCopy(out->name, "FDIV");  out->componentMask = 1; return true;
    case 53: checkedStringCopy(out->name, "SHFTA"); out->componentMask = 1; return true;
    }
}

bool oDecodeToken(int kind, char *tok, ImplicitOperandDescription *out, bool newSyntax)
{
    bool bad2;
    bool *pbad = &bad2;
    int mask;
    int *pmask = &mask;
    bool bad;
    int parsedComponentMask;
    int field;
    int count;
    char *closingParenCursor;
    char *openingParenCursor;
    char *sourceCursor;
    char *destinationCursor;
    char *end;
    char *start;

    if (!tok || !out || !*tok || *tok == '.')
        return false;
    int savedTokenWord1;
    int savedTokenWord2;
    char savedTokenCharacter;
    if (checkedStringLength(tok) > 2047) {
        reportError("odecodeToken: token was too long\n");
        return false;
    }

    char *tokenCursor = tok;
    char tokenCharacter;
    while ((tokenCharacter = *tokenCursor) && tokenCharacter != ':')
        tokenCursor++;
    if (tokenCharacter == ':') {
        *tokenCursor = 0;
        char *labelCursor = out->branchLabelText;
        tokenCursor++;
        int chars = 0;
        int seps = 1;
        while (*tokenCursor) {
            if (*tokenCursor != ':') {
                seps = 0;
                chars++;
            } else {
                chars = 0;
                seps++;
            }
            if (seps <= 1 || chars)
                *labelCursor++ = *tokenCursor;
            tokenCursor++;
        }
        if (seps)
            labelCursor--;
        savedTokenWord1 = ((int *)tok)[1];
        savedTokenWord2 = ((int *)tok)[2];
        if (labelCursor == out->branchLabelText) {
            ((int *)tok)[1] = savedTokenWord1;
            ((int *)tok)[2] = savedTokenWord2;
            reportWarning("ignoring empty tag\n");
            out->branchLabelText[0] = 0;
        } else {
            labelCursor[0] = ':';
            labelCursor[1] = 0;
        }
    }

    char *tokenEnd = tok;
    while (*tokenEnd)
        tokenEnd++;
    count = tokenEnd - tok;
    out->name[0] = 0;
    out->immediateText[0] = 0;
    out->physicalRegisterName[0] = 0;
    out->forcePhysicalBinding = 0;
    out->componentMask = 0;
    out->addressComponentMask = 0;

    switch (kind) {
    case 54: {
        int *spk = &out->addressComponentMask;
        int svx = *spk;
        int tokenHighNibble = (unsigned char)tok[2];
        tokenHighNibble >>= 4;
        tokenHighNibble >>= 4;
        if (tokenHighNibble)
            return false;
        *spk = svx;
        reportError("encountered a force bind to $ra in an explicit operand\n");
        return false;
    }
    case 35: {
        destinationCursor = out->name;
        out->forcePhysicalBinding = 1;
        tokenEnd = tok;
        while (*tokenEnd && *tokenEnd != '(')
            *destinationCursor++ = *tokenEnd++;
        *destinationCursor = 0;
        if (*tokenEnd) {
            if (*tokenEnd != '(') {
                reportError("oDecodeToken: bad fl4s\n");
                return false;
            }
            tokenEnd++;
            destinationCursor = out->physicalRegisterName;
            while (*tokenEnd && *tokenEnd != ')')
                *destinationCursor++ = *tokenEnd++;
            *destinationCursor = 0;
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
            out->componentMask = 15;
            return true;
        } else {
            *pbad = false;
            bool componentMaskParsed = parseTrailingComponentMask(out->name, &out->componentMask, pbad);
            if (*pbad) {
                reportUserMessage("bad mod bits in operand\n");
                return false;
            }
            if (!componentMaskParsed) {
                int svk_35b = ((int *)tok)[kind];
                int zsk_35b = (unsigned char)tok[2];
                zsk_35b >>= 4;
                zsk_35b >>= 4;
                if (zsk_35b)
                    return false;
                ((int *)tok)[kind] = svk_35b;
                out->componentMask = 15;
            }
            if (out->componentMask == -1) {
                reportError("oDecodeToken: bad FL4S bits\n");
                return false;
            }
            removeTrailingComponentSuffix(out->name);
        }
        return true;
    }
    case 42:
        destinationCursor = out->name;
        out->forcePhysicalBinding = 1;
        sourceCursor = tok;
        while (*sourceCursor && *sourceCursor != '(')
            *destinationCursor++ = *sourceCursor++;
        *destinationCursor = 0;
        if (*sourceCursor) {
            if (*sourceCursor != '(') {
                reportError("oDecodeToken: bad gprs\n");
                return false;
            }
            sourceCursor++;
            destinationCursor = out->physicalRegisterName;
            while (*sourceCursor && *sourceCursor != ')')
                *destinationCursor++ = *sourceCursor++;
            *destinationCursor = 0;
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
        out->componentMask = 15;
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
            out->componentMask = 15;
            checkedStringCopy(out->name, tok);
        } else {
            savedTokenWord2 = readTrailingComponentMask(tok);
            out->componentMask = savedTokenWord2;
            if (!savedTokenWord2)
                out->componentMask = 15;
            if (out->componentMask == -1) {
                int *spk1 = &out->addressComponentMask;
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
            checkedStringCopy(out->name, tok);
            removeTrailingComponentSuffix(out->name);
        }
        return true;
    case 2:
        checkedStringCopy(out->name, tok);
        out->componentMask = 1;
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
        if (parseBracketedComponentMask(tok, pmask)) {
            out->genericOperandForm = 1;
            if (countSetBits(mask) != 1) {
                reportUserMessage("bad [x,y,z,w] token for scalar\n");
                return false;
            }
            out->componentMask = mask;
            char *nameSource = tok;
            char *nameDestination = out->name;
            while (*nameSource)
                *nameDestination++ = *nameSource++;
            *nameDestination = 0;
            do
                nameDestination--;
            while (*nameDestination != '[');
            *nameDestination = 0;
            return true;
        }
        if (asciiCaseEqual(tok, "q")) {
            out->genericOperandForm = 1;
            out->componentMask = 1;
            checkedStringCopy(out->name, "Q");
            return true;
        }
        if (asciiCaseEqual(tok, "i")) {
            out->genericOperandForm = 1;
            out->componentMask = 1;
            checkedStringCopy(out->name, "I");
            return true;
        }
        out->componentMask = 15;
        out->genericOperandForm = 2;
        checkedStringCopy(out->name, tok);
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
            if (!parseBracketedComponentMask(tok, pmask) || countSetBits(mask) != 1) {
                reportError("oDecodeToken: [x/y/z/w] token for new syntax\n");
                return false;
            }
            out->componentMask = mask;
            char *nameSource = tok;
            char *nameDestination = out->name;
            while (*nameSource)
                *nameDestination++ = *nameSource++;
            *nameDestination = 0;
            do
                nameDestination--;
            while (*nameDestination != '[');
            *nameDestination = 0;
        } else {
            mask = readTrailingComponentMask(tok);
            if (countSetBits(mask) != 1) {
                reportError("oDecodeToken: bad FL1 bits\n");
                return false;
            }
            out->componentMask = mask;
            checkedStringCopy(out->name, tok);
            removeTrailingComponentSuffix(out->name);
        }
        return true;
    case 4:
        if (count != 1 || (*tok != 'i' && *tok != 'I')) {
            reportError("oDecode:: arg I\n");
            return false;
        }
        out->componentMask = 1;
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
        out->componentMask = 1;
        out->name[0] = 'P';
        out->name[1] = 0;
        return true;
    case 7:
        if (count != 1 || (*tok != 'q' && *tok != 'Q')) {
            reportError("oDecode:: arg Q\n");
            return false;
        }
        out->componentMask = 1;
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
        out->componentMask = 1;
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
        int *pm9 = &out->componentMask;
        if (!parseTrailingComponentMask(tok, pm9, &bad))
            out->componentMask = 15;
        if (out->componentMask == -1) {
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
        checkedStringCopy(out->name, "ACC");
        return true;
    }
    case 10:
        mask = readTrailingComponentMask(tok);
        if (mask && mask != 7) {
            reportError("bad mod bits for ACCxyz\n");
            return false;
        }
        out->componentMask = 7;
        checkedStringCopy(out->name, "ACC");
        return true;
    case 11:
        field = 7;
    fieldSel:
        if (newSyntax) {
            parsedComponentMask = 0;
            if (parseBracketedComponentMask(tok, &parsedComponentMask)) {
                if (parsedComponentMask != field) {
                    reportUserMessage("instruction and operand have mismatching field select\n");
                    return false;
                }
                char *nameSource = tok;
                char *nameDestination = out->name;
                while (*nameSource)
                    *nameDestination++ = *nameSource++;
                *nameDestination = 0;
                do
                    nameDestination--;
                while (*nameDestination != '[');
                *nameDestination = 0;
                out->componentMask = field;
                return true;
            }
            checkedStringCopy(out->name, tok);
            out->componentMask = field;
        } else {
            mask = readTrailingComponentMask(tok);
            if (mask && mask != field) {
                reportError("oDecode: bad FL?\n");
                return false;
            }
            out->componentMask = field;
            checkedStringCopy(out->name, tok);
            removeTrailingComponentSuffix(out->name);
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
        out->componentMask = 1;
        checkedStringCopy(out->name, "CLIP");
        return true;
    case 27:
        checkedStringCopy(out->immediateText, tok);
        return true;
    case 28: {
        out->componentMask = 1;
        savedTokenWord1 = (unsigned char)tok[1];
        if (count <= 2)
            return false;
        savedTokenCharacter = tok[savedTokenWord1];
        int z0 = (unsigned char)*tok;
        z0 >>= 4;
        z0 >>= 4;
        if (z0)
            return false;
        tok[savedTokenWord1] = savedTokenCharacter;
        char *closingDelimiter = tokenEnd - 1;
        if (*closingDelimiter != ')' || *tok != '(')
            return false;
        start = tok + 1;
        end = closingDelimiter;
        goto copyName;
    }
    case 36:
        destinationCursor = out->name;
        out->forcePhysicalBinding = 1;
        tokenEnd = tok;
        while (*tokenEnd && *tokenEnd != '(')
            *destinationCursor++ = *tokenEnd++;
        *destinationCursor = 0;
        if (*tokenEnd) {
            if (*tokenEnd != '(') {
                reportError("oDecodeToken: bad ints\n");
                return false;
            }
            tokenEnd++;
            destinationCursor = out->physicalRegisterName;
            while (*tokenEnd && *tokenEnd != ')')
                *destinationCursor++ = *tokenEnd++;
            *destinationCursor = 0;
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
        out->componentMask = 1;
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
        checkedStringCopy(out->name, tok);
        out->componentMask = 1;
        return true;
    case 30: {
        out->componentMask = 1;
        savedTokenWord2 = (unsigned char)tok[2];
        if (count <= 3)
            return false;
        savedTokenCharacter = tok[savedTokenWord2];
        int z1 = (unsigned char)*tok;
        z1 >>= 4;
        z1 >>= 4;
        if (z1)
            return false;
        tok[savedTokenWord2] = savedTokenCharacter;
        if (*tok != '(')
            return false;
        closingParenCursor = tokenEnd - 1;
        while (closingParenCursor != tok && *closingParenCursor != ')') {
            switch (*closingParenCursor) {
            case 'x': out->addressComponentMask |= 1; break;
            case 'y': out->addressComponentMask |= 2; break;
            case 'z': out->addressComponentMask |= 4; break;
            case 'w': out->addressComponentMask |= 8; break;
            default: return false;
            }
            closingParenCursor--;
        }
        if (*closingParenCursor != ')')
            return false;
        start = tok + 1;
        end = closingParenCursor;
        goto copyName;
    }
    case 31:
        out->componentMask = 1;
        if (count <= 4)
            return false;
        closingParenCursor = tokenEnd - 1;
        while (closingParenCursor != tok && *closingParenCursor != ')') {
            switch (*closingParenCursor) {
            case 'x': out->addressComponentMask |= 1; break;
            case 'y': out->addressComponentMask |= 2; break;
            case 'z': out->addressComponentMask |= 4; break;
            case 'w': out->addressComponentMask |= 8; break;
            default: return false;
            }
            closingParenCursor--;
        }
        if (*closingParenCursor != ')')
            return false;
        end = closingParenCursor;
        openingParenCursor = tokenEnd;
        while (openingParenCursor != tok && *openingParenCursor != '(')
            openingParenCursor--;
        if (*openingParenCursor != '(')
            return false;
        destinationCursor = out->immediateText;
        for (sourceCursor = tok; sourceCursor != openingParenCursor; )
            *destinationCursor++ = *sourceCursor++;
        *destinationCursor = 0;
        if (!out->immediateText[0]) {
            out->immediateText[0] = '0';
            out->immediateText[1] = 0;
        }
        start = openingParenCursor + 1;
    copyName:
        sourceCursor = start;
        for (destinationCursor = out->name; sourceCursor != end; )
            *destinationCursor++ = *sourceCursor++;
        *destinationCursor = 0;
        return true;
    case 32: {
        out->componentMask = 1;
        if (count <= 4)
            return false;
        if (tokenEnd[-1] != ')' || tokenEnd[-2] != '+')
            return false;
        char *closingDelimiter = tokenEnd - 3;
        if (*closingDelimiter != '+' || *tok != '(')
            return false;
        start = tok + 1;
        end = closingDelimiter;
        goto copyName;
    }
    case 33: {
        out->componentMask = 1;
        if (count <= 4)
            return false;
        char *closingDelimiter = tokenEnd - 1;
        if (*closingDelimiter != ')' || tok[0] != '(' || tok[1] != '-' || tok[2] != '-')
            return false;
        start = tok + 3;
        end = closingDelimiter;
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
        checkedStringCopy(out->physicalRegisterName, tok);
        checkedStringCopy(out->name, tok);
        out->componentMask = 1;
        return true;
    case 26:
        reportError("explicit mac register - but I'm writing ASTATUS here??\n");
        exitAfterCleanup(1);
        out->componentMask = 1;
        checkedStringCopy(out->name, "ASTATUS");
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
        out->componentMask = 1;
        checkedStringCopy(out->name, tok);
        return true;
    case 40:
        out->componentMask = 3;
        checkedStringCopy(out->name, tok);
        return true;
    case 41:
        out->componentMask = 15;
        checkedStringCopy(out->name, tok);
        return true;
    case 44:
        destinationCursor = out->name;
        out->forcePhysicalBinding = 1;
        sourceCursor = tok;
        while (*sourceCursor && *sourceCursor != '(')
            *destinationCursor++ = *sourceCursor++;
        *destinationCursor = 0;
        if (*sourceCursor) {
            if (*sourceCursor != '(') {
                reportError("oDecodeToken: bad fprs\n");
                return false;
            }
            sourceCursor++;
            destinationCursor = out->physicalRegisterName;
            while (*sourceCursor && *sourceCursor != ')')
                *destinationCursor++ = *sourceCursor++;
            *destinationCursor = 0;
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
        out->componentMask = 1;
        return true;
    case 29:
    case 46: {
        if (count <= 2)
            return false;
        out->componentMask = 1;
        closingParenCursor = tokenEnd - 1;
        if (*closingParenCursor != ')')
            return false;
        do
            closingParenCursor--;
        while (*closingParenCursor != '(');
        start = closingParenCursor + 1;
        closingParenCursor = tok;
        char *stop = start - 1;
        end = tokenEnd - 1;
        destinationCursor = out->immediateText;
        for (sourceCursor = tok; sourceCursor != stop; )
            *destinationCursor++ = *sourceCursor++;
        *destinationCursor = 0;
        if (!out->immediateText[0]) {
            out->immediateText[0] = '0';
            out->immediateText[1] = 0;
        }
        goto copyName;
    }
    default: {
        sourceCursor = tok + kind;
        int discardedUpperNibbleCheck = (unsigned char)tok[2];
        discardedUpperNibbleCheck >>= 4;
        discardedUpperNibbleCheck >>= 4;
        if (discardedUpperNibbleCheck) {
            destinationCursor = out->physicalRegisterName;
            while (*sourceCursor && *sourceCursor != ')')
                *destinationCursor++ = *sourceCursor++;
        }
        reportError("oDecode: general BAD '%s'\n", opKindName(kind));
        tokenEnd = 0;
        openingParenCursor = 0;
        break;
    }
    }
    return false;
}

const char *opKindName(int kind)
{
    const char *kindName = "BAD_OPERAND";
    switch (kind) {
    case 0: kindName = "OP_NULL"; break;
    case 1: kindName = "OP_V_FL4"; break;
    case 2: kindName = "OP_V_INT"; break;
    case 3: kindName = "OP_V_FL1"; break;
    case 4: kindName = "OP_V_I"; break;
    case 5: kindName = "OP_V_P"; break;
    case 6: kindName = "OP_V_nP"; break;
    case 7: kindName = "OP_V_Q"; break;
    case 8: kindName = "OP_V_R"; break;
    case 9: kindName = "OP_V_ACC"; break;
    case 10: kindName = "OP_V_ACCXYZ"; break;
    case 11: kindName = "OP_V_FLXYZ"; break;
    case 13: kindName = "OP_V_FLX"; break;
    case 14: kindName = "OP_V_FLY"; break;
    case 15: kindName = "OP_V_FLZ"; break;
    case 16: kindName = "OP_V_FLW"; break;
    case 17: kindName = "OP_V_CLIP"; break;
    case 18: case 19: case 20: case 21: kindName = "OP_V_STAT"; break;
    case 22: kindName = "OP_V_STAT_DIV"; break;
    case 23: kindName = "OP_V_STAT_ALU"; break;
    case 24: kindName = "OP_V_STAT_TST"; break;
    case 25: kindName = "OP_V_STAT_SET"; break;
    case 26: kindName = "OP_V_MAC"; break;
    case 27: kindName = "OP_IMM"; break;
    case 28: kindName = "OP_V_EA"; break;
    case 29: kindName = "OP_V_EA_IMM"; break;
    case 30: kindName = "OP_V_EA_REG"; break;
    case 31: kindName = "OP_V_EA_IMM_REG"; break;
    case 32: kindName = "OP_V_EA_INC"; break;
    case 33: kindName = "OP_V_EA_DEC"; break;
    case 34: kindName = "OP_V_INT1"; break;
    case 35: kindName = "OP_V_FL4S"; break;
    case 36: kindName = "OP_V_INTS"; break;
    case 37: kindName = "OP_V_MEMREG"; break;
    case 38: kindName = "OP_V_TBD"; break;
    case 39: kindName = "OP_C_GPR32"; break;
    case 40: kindName = "OP_C_GPR64"; break;
    case 41: kindName = "OP_C_GPR128"; break;
    case 42: kindName = "OP_C_GPRS"; break;
    case 43: kindName = "OP_C_FPR"; break;
    case 44: kindName = "OP_C_FPRS"; break;
    case 45: kindName = "OP_V_CLIPA"; break;
    case 46: kindName = "OP_C_EA_IMM"; break;
    case 47: kindName = "OP_C_HILO0"; break;
    case 48: kindName = "OP_C_HILO1"; break;
    case 49: kindName = "OP_C_FSTAT"; break;
    case 50: kindName = "OP_C_FCOND"; break;
    case 51: kindName = "OP_C_FACC"; break;
    case 52: kindName = "OP_C_FDIV"; break;
    case 53: kindName = "OP_C_SHFTA"; break;
    case 54: kindName = "OP_C_GPR64_31($ra)"; break;
    case 55: kindName = "OP_EXT_DEP"; break;
    }
    return kindName;
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
        Instr *containingInstruction = (Instr *)((char *)this - off);
        if (containingInstruction->operandCount < operandIndex) {
            reportError("operand: getOwner - bad instruction?\n");
            exitAfterCleanup(-1);
        }
        owner = containingInstruction;
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

char *Operand::emitExpectedOperandType(bool flag, char *outputBuffer)
{
    if (!outputBuffer)
        return 0;
    if (isImplicit)
        return outputBuffer;
    switch (operandKind) {
    case 38: outputBuffer += formatIntoBuffer(outputBuffer, "vec_reg"); break;
    case 1:
        if (flag)
            outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg");
        else
            outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg[xyzw]");
        break;
    case 2: outputBuffer += formatIntoBuffer(outputBuffer, "vi_reg"); break;
    case 3: outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg[x/y/z/w]"); break;
    case 4: outputBuffer += formatIntoBuffer(outputBuffer, "I"); break;
    case 5: outputBuffer += formatIntoBuffer(outputBuffer, "P"); break;
    case 7: outputBuffer += formatIntoBuffer(outputBuffer, "Q"); break;
    case 8: outputBuffer += formatIntoBuffer(outputBuffer, "R"); break;
    case 9: outputBuffer += formatIntoBuffer(outputBuffer, "ACC"); break;
    case 10: outputBuffer += formatIntoBuffer(outputBuffer, "ACCxyz"); break;
    case 11: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regxyz"); break;
    case 13: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regx"); break;
    case 14: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regy"); break;
    case 15: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regz"); break;
    case 16: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regw"); break;
    case 27: outputBuffer += formatIntoBuffer(outputBuffer, "IMMEDIATE(%s)", immediateTypeName(immediateDataType)); break;
    case 28: outputBuffer += formatIntoBuffer(outputBuffer, "(vi_reg)"); break;
    case 29: outputBuffer += formatIntoBuffer(outputBuffer, "OFFSET(vi_reg)"); break;
    case 30: outputBuffer += formatIntoBuffer(outputBuffer, "(vi_reg)[x/y/z/w]"); break;
    case 31: outputBuffer += formatIntoBuffer(outputBuffer, "OFFSET(vi_reg)[x/y/z/w]"); break;
    case 32: outputBuffer += formatIntoBuffer(outputBuffer, "(vi_reg++)"); break;
    case 33: outputBuffer += formatIntoBuffer(outputBuffer, "(--vi_reg)"); break;
    case 34: outputBuffer += formatIntoBuffer(outputBuffer, "vi01"); break;
    case 35: outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg[.xyzw][(base_reg)]"); break;
    case 36: outputBuffer += formatIntoBuffer(outputBuffer, "vi_reg[(base_reg)]"); break;
    case 39: case 40: case 41: outputBuffer += formatIntoBuffer(outputBuffer, "gpr"); break;
    case 42: outputBuffer += formatIntoBuffer(outputBuffer, "gpr(reg)"); break;
    case 43: outputBuffer += formatIntoBuffer(outputBuffer, "fpr"); break;
    case 44: outputBuffer += formatIntoBuffer(outputBuffer, "fpr(reg)"); break;
    case 46: outputBuffer += formatIntoBuffer(outputBuffer, "OFFSET(gpr)"); break;
    case 54: outputBuffer += formatIntoBuffer(outputBuffer, "$ra"); break;
    default: break;
    }
    return outputBuffer;
}

bool isScalarFloatingOperandKind(int kind)
{
    return kind == 43 || kind == 3 || kind == 13 || kind == 14 ||
           kind == 15 || kind == 16 || kind == 5 || kind == 7;
}

bool isVectorOrAccumulatorOperandKind(int kind)
{
    return kind == 1 || kind == 9;
}

int Operand::getRegFrom(Operand *sourceOperand)
{
    int targetRegisterClass = registerClassForOperandKind(operandKind);
    int sourceRegisterClass = registerClassForOperandKind(sourceOperand->operandKind);
    if (sourceRegisterClass != targetRegisterClass) {
        reportError("incompatible type request getting type for operand\n");
        return 0;
    }
    assignedRegister = sourceOperand->assignedRegister;
    originalRegister = sourceOperand->originalRegister;
    forcedBaseRegister = sourceOperand->forcedBaseRegister;
    if (!assignedRegister) {
        reportError("no register set in get reg from\n");
        return 0;
    }
    if (assignedRegister->registerClass != sourceRegisterClass) {
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
