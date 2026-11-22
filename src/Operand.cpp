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
    operandKind = VCL_OP_NULL;
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
    case VCL_OP_V_FL4:
    case VCL_OP_V_FL4S:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FL4 or V_FL4S/ MEM!\n");
            return false;
        }
        if ((componentMask & VCL_COMPONENT_ALL) == componentMask)
            return true;
        reportError("not all!\n");
        return false;
    case VCL_OP_V_FL1:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FL1!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_X || componentMask == VCL_COMPONENT_Y || componentMask == VCL_COMPONENT_Z || componentMask == VCL_COMPONENT_W)
            return true;
        reportError("bad mod bits for FL1..%d\n", componentMask);
        return false;
    case VCL_OP_V_I:
    case VCL_OP_V_P:
    case VCL_OP_V_NP:
    case VCL_OP_V_Q:
    case VCL_OP_V_R:
        if (!assignedRegister) {
            reportError("reg no reg set - V_P/Q/R/I/nQ!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_X)
            return true;
        reportError("bad mod bits for V_P/Q/R/I\n");
        return false;
    case VCL_OP_V_ACC:
        if (!assignedRegister) {
            reportError("reg no reg set - V_ACC!\n");
            return false;
        }
        if ((componentMask & VCL_COMPONENT_ALL) == componentMask)
            return true;
        reportError("bad mod bits..hmm..out of range for acc\n");
        return false;
    case VCL_OP_V_ACCXYZ:
        if (!assignedRegister) {
            reportError("reg no reg set - V_ACCYZ!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_XYZ)
            return true;
        reportError("xyz not set to ACCXYZ!\n");
        return false;
    case VCL_OP_V_FLXYZ:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLXYZ!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_XYZ)
            return true;
        reportError("xyz not set to XYZ!\n");
        return false;
    case VCL_OP_V_FLX:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLX!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_X)
            return true;
        reportError("bad mod bits for FLX\n");
        return false;
    case VCL_OP_V_FLY:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLY!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_Y)
            return true;
        reportError("bad mod bits for FLY\n");
        return false;
    case VCL_OP_V_FLZ:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLZ!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_Z)
            return true;
        reportError("bad mod bits for FLZ\n");
        return false;
    case VCL_OP_V_FLW:
        if (!assignedRegister) {
            reportError("reg no reg set - V_FLW!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_W)
            return true;
        reportError("bad mod bits for FLW\n");
        return false;
    case VCL_OP_V_CLIP:
    case VCL_OP_V_STAT_ALU_SHADOW:
    case VCL_OP_V_STAT_DIV_SHADOW:
    case VCL_OP_V_STAT_DIV_REGISTER:
    case VCL_OP_V_STAT_ALU_REGISTER:
    case VCL_OP_V_STAT_DIV:
    case VCL_OP_V_STAT_ALU:
    case VCL_OP_V_STAT_TST:
    case VCL_OP_V_STAT_SET:
    case VCL_OP_V_MAC:
    case VCL_OP_V_INT1:
    case VCL_OP_V_CLIPA:
    case VCL_OP_EXT_DEP:
        return true;
    case VCL_OP_IMM:
        return immediateValue.isValid() != 0;
    case VCL_OP_V_EA:
        if (componentMask != VCL_COMPONENT_X)
            return false;
    case VCL_OP_V_EA_IMM:
    case VCL_OP_V_EA_REG:
    case VCL_OP_V_EA_IMM_REG:
    case VCL_OP_V_EA_INC:
    case VCL_OP_V_EA_DEC:
    case VCL_OP_C_EA_IMM:
        return assignedRegister != 0;
    case VCL_OP_V_INT:
    case VCL_OP_V_INTS:
        if (!assignedRegister) {
            reportError("reg no reg set - V_INT/INTS!\n");
            return false;
        }
        if (componentMask == VCL_COMPONENT_X)
            return true;
        reportError("bad mod bits for int/ints!\n");
        return false;
    case VCL_OP_V_MEMREG:
        if (assignedRegister)
            return true;
        reportError("ramreg.. not ok.. no m_Reg\n");
        return false;
    case VCL_OP_C_GPR32:
    case VCL_OP_C_GPR64:
    case VCL_OP_C_GPR128:
    case VCL_OP_C_GPRS:
    case VCL_OP_C_FPR:
    case VCL_OP_C_FPRS:
    case VCL_OP_C_HILO0:
    case VCL_OP_C_HILO1:
    case VCL_OP_C_FSTAT:
    case VCL_OP_C_FCOND:
    case VCL_OP_C_FACC:
    case VCL_OP_C_FDIV:
    case VCL_OP_C_SHFTA:
    case VCL_OP_C_GPR64_31_RA:
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
        case VCL_OP_NULL:
            break;
        case VCL_OP_V_EA:
        case VCL_OP_V_EA_IMM:
        case VCL_OP_V_EA_REG:
        case VCL_OP_V_EA_INC:
        case VCL_OP_V_EA_DEC:
        case VCL_OP_C_EA_IMM:
            operandValueValid = immediateValue.isValid();
            break;
        case VCL_OP_IMM:
            immediateValue.dump();
            break;
        case VCL_OP_EXT_DEP:
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
    case VCL_OP_V_FLX:
    emit_x:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sx", registerNameText);
        break;
    case VCL_OP_V_FL1:
        if (componentMask & 1)
            goto emit_x;
        if (componentMask & 2)
            goto emit_y;
        if (componentMask & 4)
            goto emit_z;
        if (componentMask & 8)
            goto emit_w;
        return 0;
    case VCL_OP_V_FLY:
    emit_y:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sy", registerNameText);
        break;
    case VCL_OP_V_I:
        outputBuffer += formatIntoBuffer(outputBuffer, "I");
        break;
    case VCL_OP_V_P:
        outputBuffer += formatIntoBuffer(outputBuffer, "P");
        break;
    case VCL_OP_V_NP:
        outputBuffer += formatIntoBuffer(outputBuffer, "nP");
        break;
    case VCL_OP_V_Q:
        outputBuffer += formatIntoBuffer(outputBuffer, "Q");
        break;
    case VCL_OP_V_FLZ:
    emit_z:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sz", registerNameText);
        break;
    case VCL_OP_V_R:
        outputBuffer += formatIntoBuffer(outputBuffer, "R");
        break;
    case VCL_OP_V_ACC:
        outputBuffer += formatIntoBuffer(outputBuffer, "ACC");
        break;
    case VCL_OP_V_ACCXYZ:
        outputBuffer += formatIntoBuffer(outputBuffer, "ACCxyz");
        break;
    case VCL_OP_V_FLW:
    emit_w:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sw", registerNameText);
        break;
    case VCL_OP_V_FLXYZ:
        outputBuffer += formatIntoBuffer(outputBuffer, "%sxyz", registerNameText);
        break;
    case VCL_OP_V_FL4:
    case VCL_OP_V_INT:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case VCL_OP_C_GPR32:
    case VCL_OP_C_GPR64:
    case VCL_OP_C_GPR128:
    case VCL_OP_C_GPRS:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case VCL_OP_C_FPR:
    case VCL_OP_C_FPRS:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case VCL_OP_C_GPR64_31_RA:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case VCL_OP_V_INT1:
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", registerNameText);
        break;
    case VCL_OP_IMM: {
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
    case VCL_OP_V_EA_IMM:
    case VCL_OP_C_EA_IMM: {
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
    case VCL_OP_V_EA_IMM_REG: {
        char *immediateLiteralText = immediateValue.asChars();
        if (!immediateLiteralText)
            reportError("failed to get chars from ximm for v_ea_imm\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "%s", immediateLiteralText);
    }
    case VCL_OP_V_EA_REG:
        if (!registerNameText) {
            reportError("null rn - abort _V_EA_REG\n");
            return 0;
        }
    case VCL_OP_V_EA:
        outputBuffer += formatIntoBuffer(outputBuffer, "(%s)", registerNameText);
        break;
    case VCL_OP_V_EA_INC:
        if (!registerNameText) {
            reportError("null rn - abort _V_EA_INC\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer(outputBuffer, "(%s++)", registerNameText);
        break;
    case VCL_OP_V_EA_DEC:
        if (!registerNameText) {
            reportError("null rn - abort _V_EA_DEC\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer(outputBuffer, "(--%s)", registerNameText);
        break;
    case VCL_OP_V_MEMREG:
        if (!registerNameText) {
            reportError("emit ramreg - no rn\n");
            return 0;
        }
        outputBuffer += formatIntoBuffer("ramreg-[%s]", registerNameText);
        break;
    case VCL_OP_EXT_DEP:
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
    case VCL_OP_V_ACCXYZ: checkedStringCopy(out->name, "ACC");   out->componentMask = VCL_COMPONENT_XYZ; return true;
    case VCL_OP_V_I:  checkedStringCopy(out->name, "I");     out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_P:  checkedStringCopy(out->name, "P");     out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_NP:  checkedStringCopy(out->name, "tt_nP"); out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_Q:  checkedStringCopy(out->name, "Q");     out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_R:  checkedStringCopy(out->name, "R");     out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_ACC:  checkedStringCopy(out->name, "ACC");   out->componentMask = VCL_COMPONENT_ALL; return true;
    case VCL_OP_V_CLIP: checkedStringCopy(out->name, "CLIP");  out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_STAT_ALU_SHADOW: case VCL_OP_V_STAT_DIV_SHADOW: case VCL_OP_V_STAT_DIV_REGISTER: case VCL_OP_V_STAT_ALU_REGISTER:
        reportError("odecode for a V_STAT\n");
        return false;
    case VCL_OP_EXT_DEP:
        out->name[0] = 0;
        return true;
    case VCL_OP_V_STAT_DIV: case VCL_OP_V_STAT_ALU: case VCL_OP_V_STAT_TST: case VCL_OP_V_STAT_SET:
        out->name[0] = 0;
        return true;
    case VCL_OP_V_MAC: checkedStringCopy(out->name, "MAC");   out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_EA: out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_V_MEMREG: checkedStringCopy(out->name, "ramreg"); return true;
    case VCL_OP_C_GPR64_31_RA:
        checkedStringCopy(out->physicalRegisterName, "$ra");
        checkedStringCopy(out->name, "$ra");
        out->componentMask = 3;
        return true;
    case VCL_OP_C_GPR64: out->componentMask = 3; return true;
    case VCL_OP_V_CLIPA: checkedStringCopy(out->name, "CLIPA"); out->componentMask = VCL_COMPONENT_X; return true;
    default:
        reportError("fell through building an implicit operand of type %s\n", opKindName(kind));
        out->name[0] = 0;
        out->physicalRegisterName[0] = 0;
        return false;
    case VCL_OP_C_HILO0: checkedStringCopy(out->name, "HILO0"); out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_C_HILO1: checkedStringCopy(out->name, "HILO1"); out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_C_FSTAT: checkedStringCopy(out->name, "FSTAT"); out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_C_FCOND: checkedStringCopy(out->name, "FCOND"); out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_C_FACC: checkedStringCopy(out->name, "FACC");  out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_C_FDIV: checkedStringCopy(out->name, "FDIV");  out->componentMask = VCL_COMPONENT_X; return true;
    case VCL_OP_C_SHFTA: checkedStringCopy(out->name, "SHFTA"); out->componentMask = VCL_COMPONENT_X; return true;
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
    case VCL_OP_C_GPR64_31_RA: {
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
    case VCL_OP_V_FL4S: {
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
            out->componentMask = VCL_COMPONENT_ALL;
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
                out->componentMask = VCL_COMPONENT_ALL;
            }
            if (out->componentMask == -1) {
                reportError("oDecodeToken: bad FL4S bits\n");
                return false;
            }
            removeTrailingComponentSuffix(out->name);
        }
        return true;
    }
    case VCL_OP_C_GPRS:
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
        out->componentMask = VCL_COMPONENT_ALL;
        return true;
    case VCL_OP_V_FL4:
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
            out->componentMask = VCL_COMPONENT_ALL;
            checkedStringCopy(out->name, tok);
        } else {
            savedTokenWord2 = readTrailingComponentMask(tok);
            out->componentMask = savedTokenWord2;
            if (!savedTokenWord2)
                out->componentMask = VCL_COMPONENT_ALL;
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
    case VCL_OP_V_INT:
        checkedStringCopy(out->name, tok);
        out->componentMask = VCL_COMPONENT_X;
        return true;
    case VCL_OP_V_TBD:
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
            out->componentMask = VCL_COMPONENT_X;
            checkedStringCopy(out->name, "Q");
            return true;
        }
        if (asciiCaseEqual(tok, "i")) {
            out->genericOperandForm = 1;
            out->componentMask = VCL_COMPONENT_X;
            checkedStringCopy(out->name, "I");
            return true;
        }
        out->componentMask = VCL_COMPONENT_ALL;
        out->genericOperandForm = 2;
        checkedStringCopy(out->name, tok);
        return true;
    case VCL_OP_V_FL1:
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
    case VCL_OP_V_I:
        if (count != 1 || (*tok != 'i' && *tok != 'I')) {
            reportError("oDecode:: arg I\n");
            return false;
        }
        out->componentMask = VCL_COMPONENT_X;
        out->name[0] = 'I';
        out->name[1] = 0;
        return true;
    case VCL_OP_V_P:
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
        out->componentMask = VCL_COMPONENT_X;
        out->name[0] = 'P';
        out->name[1] = 0;
        return true;
    case VCL_OP_V_Q:
        if (count != 1 || (*tok != 'q' && *tok != 'Q')) {
            reportError("oDecode:: arg Q\n");
            return false;
        }
        out->componentMask = VCL_COMPONENT_X;
        out->name[0] = 'Q';
        out->name[1] = 0;
        return true;
    case VCL_OP_V_R:
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
        out->componentMask = VCL_COMPONENT_X;
        out->name[0] = 'R';
        out->name[1] = 0;
        return true;
    case VCL_OP_V_ACC: {
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
            out->componentMask = VCL_COMPONENT_ALL;
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
    case VCL_OP_V_ACCXYZ:
        mask = readTrailingComponentMask(tok);
        if (mask && mask != 7) {
            reportError("bad mod bits for ACCxyz\n");
            return false;
        }
        out->componentMask = VCL_COMPONENT_XYZ;
        checkedStringCopy(out->name, "ACC");
        return true;
    case VCL_OP_V_FLXYZ:
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
    case VCL_OP_V_FLX:
        field = 1;
        goto fieldSel;
    case VCL_OP_V_FLY:
        field = 2;
        goto fieldSel;
    case VCL_OP_V_FLZ:
        field = 4;
        goto fieldSel;
    case VCL_OP_V_FLW:
        field = 8;
        goto fieldSel;
    case VCL_OP_V_CLIP:
        out->componentMask = VCL_COMPONENT_X;
        checkedStringCopy(out->name, "CLIP");
        return true;
    case VCL_OP_IMM:
        checkedStringCopy(out->immediateText, tok);
        return true;
    case VCL_OP_V_EA: {
        out->componentMask = VCL_COMPONENT_X;
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
    case VCL_OP_V_INTS:
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
        out->componentMask = VCL_COMPONENT_X;
        return true;
    case VCL_OP_C_FPR:
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
        out->componentMask = VCL_COMPONENT_X;
        return true;
    case VCL_OP_V_EA_REG: {
        out->componentMask = VCL_COMPONENT_X;
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
    case VCL_OP_V_EA_IMM_REG:
        out->componentMask = VCL_COMPONENT_X;
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
    case VCL_OP_V_EA_INC: {
        out->componentMask = VCL_COMPONENT_X;
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
    case VCL_OP_V_EA_DEC: {
        out->componentMask = VCL_COMPONENT_X;
        if (count <= 4)
            return false;
        char *closingDelimiter = tokenEnd - 1;
        if (*closingDelimiter != ')' || tok[0] != '(' || tok[1] != '-' || tok[2] != '-')
            return false;
        start = tok + 3;
        end = closingDelimiter;
        goto copyName;
    }
    case VCL_OP_V_INT1:
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
        out->componentMask = VCL_COMPONENT_X;
        return true;
    case VCL_OP_V_MAC:
        reportError("explicit mac register - but I'm writing ASTATUS here??\n");
        exitAfterCleanup(1);
        out->componentMask = VCL_COMPONENT_X;
        checkedStringCopy(out->name, "ASTATUS");
        return true;
    case VCL_OP_C_GPR32:
        {
            int svk_c39 = ((int *)tok)[kind];
            int zsk_c39 = (unsigned char)tok[2];
            zsk_c39 >>= 4;
            zsk_c39 >>= 4;
            if (zsk_c39)
                return false;
            ((int *)tok)[kind] = svk_c39;
        }
        out->componentMask = VCL_COMPONENT_X;
        checkedStringCopy(out->name, tok);
        return true;
    case VCL_OP_C_GPR64:
        out->componentMask = 3;
        checkedStringCopy(out->name, tok);
        return true;
    case VCL_OP_C_GPR128:
        out->componentMask = VCL_COMPONENT_ALL;
        checkedStringCopy(out->name, tok);
        return true;
    case VCL_OP_C_FPRS:
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
        out->componentMask = VCL_COMPONENT_X;
        return true;
    case VCL_OP_V_EA_IMM:
    case VCL_OP_C_EA_IMM: {
        if (count <= 2)
            return false;
        out->componentMask = VCL_COMPONENT_X;
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
    case VCL_OP_NULL: kindName = "OP_NULL"; break;
    case VCL_OP_V_FL4: kindName = "OP_V_FL4"; break;
    case VCL_OP_V_INT: kindName = "OP_V_INT"; break;
    case VCL_OP_V_FL1: kindName = "OP_V_FL1"; break;
    case VCL_OP_V_I: kindName = "OP_V_I"; break;
    case VCL_OP_V_P: kindName = "OP_V_P"; break;
    case VCL_OP_V_NP: kindName = "OP_V_nP"; break;
    case VCL_OP_V_Q: kindName = "OP_V_Q"; break;
    case VCL_OP_V_R: kindName = "OP_V_R"; break;
    case VCL_OP_V_ACC: kindName = "OP_V_ACC"; break;
    case VCL_OP_V_ACCXYZ: kindName = "OP_V_ACCXYZ"; break;
    case VCL_OP_V_FLXYZ: kindName = "OP_V_FLXYZ"; break;
    case VCL_OP_V_FLX: kindName = "OP_V_FLX"; break;
    case VCL_OP_V_FLY: kindName = "OP_V_FLY"; break;
    case VCL_OP_V_FLZ: kindName = "OP_V_FLZ"; break;
    case VCL_OP_V_FLW: kindName = "OP_V_FLW"; break;
    case VCL_OP_V_CLIP: kindName = "OP_V_CLIP"; break;
    case VCL_OP_V_STAT_ALU_SHADOW: case VCL_OP_V_STAT_DIV_SHADOW: case VCL_OP_V_STAT_DIV_REGISTER: case VCL_OP_V_STAT_ALU_REGISTER: kindName = "OP_V_STAT"; break;
    case VCL_OP_V_STAT_DIV: kindName = "OP_V_STAT_DIV"; break;
    case VCL_OP_V_STAT_ALU: kindName = "OP_V_STAT_ALU"; break;
    case VCL_OP_V_STAT_TST: kindName = "OP_V_STAT_TST"; break;
    case VCL_OP_V_STAT_SET: kindName = "OP_V_STAT_SET"; break;
    case VCL_OP_V_MAC: kindName = "OP_V_MAC"; break;
    case VCL_OP_IMM: kindName = "OP_IMM"; break;
    case VCL_OP_V_EA: kindName = "OP_V_EA"; break;
    case VCL_OP_V_EA_IMM: kindName = "OP_V_EA_IMM"; break;
    case VCL_OP_V_EA_REG: kindName = "OP_V_EA_REG"; break;
    case VCL_OP_V_EA_IMM_REG: kindName = "OP_V_EA_IMM_REG"; break;
    case VCL_OP_V_EA_INC: kindName = "OP_V_EA_INC"; break;
    case VCL_OP_V_EA_DEC: kindName = "OP_V_EA_DEC"; break;
    case VCL_OP_V_INT1: kindName = "OP_V_INT1"; break;
    case VCL_OP_V_FL4S: kindName = "OP_V_FL4S"; break;
    case VCL_OP_V_INTS: kindName = "OP_V_INTS"; break;
    case VCL_OP_V_MEMREG: kindName = "OP_V_MEMREG"; break;
    case VCL_OP_V_TBD: kindName = "OP_V_TBD"; break;
    case VCL_OP_C_GPR32: kindName = "OP_C_GPR32"; break;
    case VCL_OP_C_GPR64: kindName = "OP_C_GPR64"; break;
    case VCL_OP_C_GPR128: kindName = "OP_C_GPR128"; break;
    case VCL_OP_C_GPRS: kindName = "OP_C_GPRS"; break;
    case VCL_OP_C_FPR: kindName = "OP_C_FPR"; break;
    case VCL_OP_C_FPRS: kindName = "OP_C_FPRS"; break;
    case VCL_OP_V_CLIPA: kindName = "OP_V_CLIPA"; break;
    case VCL_OP_C_EA_IMM: kindName = "OP_C_EA_IMM"; break;
    case VCL_OP_C_HILO0: kindName = "OP_C_HILO0"; break;
    case VCL_OP_C_HILO1: kindName = "OP_C_HILO1"; break;
    case VCL_OP_C_FSTAT: kindName = "OP_C_FSTAT"; break;
    case VCL_OP_C_FCOND: kindName = "OP_C_FCOND"; break;
    case VCL_OP_C_FACC: kindName = "OP_C_FACC"; break;
    case VCL_OP_C_FDIV: kindName = "OP_C_FDIV"; break;
    case VCL_OP_C_SHFTA: kindName = "OP_C_SHFTA"; break;
    case VCL_OP_C_GPR64_31_RA: kindName = "OP_C_GPR64_31($ra)"; break;
    case VCL_OP_EXT_DEP: kindName = "OP_EXT_DEP"; break;
    }
    return kindName;
}

int registerClassForOperandKind(int kind)
{
    switch (kind) {
    case VCL_OP_V_FL4:
        return 9;
    case VCL_OP_V_FL1:
        return 9;
    case VCL_OP_V_FLXYZ:
        return 9;
    case VCL_OP_V_FLX:
        return 9;
    case VCL_OP_V_FLY:
        return 9;
    case VCL_OP_V_FLZ:
        return 9;
    case VCL_OP_V_FLW:
        return 9;
    case VCL_OP_V_FL4S:
        return 9;
    default:
    case VCL_OP_EXT_DEP:
        return 0;
    case VCL_OP_NULL:
        return 0;
    case VCL_OP_V_INT:
        return 8;
    case VCL_OP_V_EA:
        return 8;
    case VCL_OP_V_EA_IMM:
        return 8;
    case VCL_OP_V_EA_REG:
        return 8;
    case VCL_OP_V_EA_IMM_REG:
        return 8;
    case VCL_OP_V_EA_INC:
        return 8;
    case VCL_OP_V_EA_DEC:
        return 8;
    case VCL_OP_V_INT1:
        return 8;
    case VCL_OP_V_INTS:
        return 8;
    case VCL_OP_V_MEMREG:
        return 11;
    case VCL_OP_C_FPR: case VCL_OP_C_FPRS:
        return 2;
    case VCL_OP_V_I: case VCL_OP_V_P: case VCL_OP_V_NP: case VCL_OP_V_Q: case VCL_OP_V_R: case VCL_OP_V_ACC: case VCL_OP_V_ACCXYZ: case VCL_OP_V_CLIP:
    case VCL_OP_V_STAT_ALU_SHADOW: case VCL_OP_V_STAT_DIV_SHADOW: case VCL_OP_V_STAT_DIV_REGISTER: case VCL_OP_V_STAT_ALU_REGISTER: case VCL_OP_V_MAC: case VCL_OP_V_CLIPA:
        return 10;
    case VCL_OP_C_HILO0: case VCL_OP_C_HILO1: case VCL_OP_C_FSTAT: case VCL_OP_C_FCOND: case VCL_OP_C_FACC: case VCL_OP_C_FDIV: case VCL_OP_C_SHFTA:
        return 6;
    case VCL_OP_C_GPR32: case VCL_OP_C_GPR64: case VCL_OP_C_GPR128: case VCL_OP_C_GPRS: case VCL_OP_C_EA_IMM: case VCL_OP_C_GPR64_31_RA:
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
    case VCL_OP_V_TBD: outputBuffer += formatIntoBuffer(outputBuffer, "vec_reg"); break;
    case VCL_OP_V_FL4:
        if (flag)
            outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg");
        else
            outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg[xyzw]");
        break;
    case VCL_OP_V_INT: outputBuffer += formatIntoBuffer(outputBuffer, "vi_reg"); break;
    case VCL_OP_V_FL1: outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg[x/y/z/w]"); break;
    case VCL_OP_V_I: outputBuffer += formatIntoBuffer(outputBuffer, "I"); break;
    case VCL_OP_V_P: outputBuffer += formatIntoBuffer(outputBuffer, "P"); break;
    case VCL_OP_V_Q: outputBuffer += formatIntoBuffer(outputBuffer, "Q"); break;
    case VCL_OP_V_R: outputBuffer += formatIntoBuffer(outputBuffer, "R"); break;
    case VCL_OP_V_ACC: outputBuffer += formatIntoBuffer(outputBuffer, "ACC"); break;
    case VCL_OP_V_ACCXYZ: outputBuffer += formatIntoBuffer(outputBuffer, "ACCxyz"); break;
    case VCL_OP_V_FLXYZ: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regxyz"); break;
    case VCL_OP_V_FLX: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regx"); break;
    case VCL_OP_V_FLY: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regy"); break;
    case VCL_OP_V_FLZ: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regz"); break;
    case VCL_OP_V_FLW: outputBuffer += formatIntoBuffer(outputBuffer, "vf_regw"); break;
    case VCL_OP_IMM: outputBuffer += formatIntoBuffer(outputBuffer, "IMMEDIATE(%s)", immediateTypeName(immediateDataType)); break;
    case VCL_OP_V_EA: outputBuffer += formatIntoBuffer(outputBuffer, "(vi_reg)"); break;
    case VCL_OP_V_EA_IMM: outputBuffer += formatIntoBuffer(outputBuffer, "OFFSET(vi_reg)"); break;
    case VCL_OP_V_EA_REG: outputBuffer += formatIntoBuffer(outputBuffer, "(vi_reg)[x/y/z/w]"); break;
    case VCL_OP_V_EA_IMM_REG: outputBuffer += formatIntoBuffer(outputBuffer, "OFFSET(vi_reg)[x/y/z/w]"); break;
    case VCL_OP_V_EA_INC: outputBuffer += formatIntoBuffer(outputBuffer, "(vi_reg++)"); break;
    case VCL_OP_V_EA_DEC: outputBuffer += formatIntoBuffer(outputBuffer, "(--vi_reg)"); break;
    case VCL_OP_V_INT1: outputBuffer += formatIntoBuffer(outputBuffer, "vi01"); break;
    case VCL_OP_V_FL4S: outputBuffer += formatIntoBuffer(outputBuffer, "vf_reg[.xyzw][(base_reg)]"); break;
    case VCL_OP_V_INTS: outputBuffer += formatIntoBuffer(outputBuffer, "vi_reg[(base_reg)]"); break;
    case VCL_OP_C_GPR32: case VCL_OP_C_GPR64: case VCL_OP_C_GPR128: outputBuffer += formatIntoBuffer(outputBuffer, "gpr"); break;
    case VCL_OP_C_GPRS: outputBuffer += formatIntoBuffer(outputBuffer, "gpr(reg)"); break;
    case VCL_OP_C_FPR: outputBuffer += formatIntoBuffer(outputBuffer, "fpr"); break;
    case VCL_OP_C_FPRS: outputBuffer += formatIntoBuffer(outputBuffer, "fpr(reg)"); break;
    case VCL_OP_C_EA_IMM: outputBuffer += formatIntoBuffer(outputBuffer, "OFFSET(gpr)"); break;
    case VCL_OP_C_GPR64_31_RA: outputBuffer += formatIntoBuffer(outputBuffer, "$ra"); break;
    default: break;
    }
    return outputBuffer;
}

bool isScalarFloatingOperandKind(int kind)
{
    return kind == VCL_OP_C_FPR || kind == VCL_OP_V_FL1 || kind == VCL_OP_V_FLX || kind == VCL_OP_V_FLY ||
           kind == VCL_OP_V_FLZ || kind == VCL_OP_V_FLW || kind == VCL_OP_V_P || kind == VCL_OP_V_Q;
}

bool isVectorOrAccumulatorOperandKind(int kind)
{
    return kind == VCL_OP_V_FL4 || kind == VCL_OP_V_ACC;
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
