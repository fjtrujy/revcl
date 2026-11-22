extern const char iInfoLoadstoreRamtypeError[];
extern const char iInfoExplicitAfterImplicitError[];
extern const char iInfoPatchedAfterPatchError[];
#include "Instr.h"
#include "Block.h"
#include "os.h"

unsigned char OpcodeDescriptor::branchFlags()
{
    if (kind() == 3)
        return kindData;
    return 0;
}

unsigned char OpcodeDescriptor::ramType()
{
    if (kind() == 4) {
        unsigned char ramAccessType = kindData;
        if ((unsigned char)(ramAccessType - 1) <= 1)
            return ramAccessType;
        reportError(iInfoLoadstoreRamtypeError);
        exitAfterCleanup(-1);
    }
    return 0;
}

int OpcodeDescriptor::syntheticKind()
{
    if (kind() == 2)
        return kindData;
    return 0;
}

bool OpcodeDescriptor::isRawOnly()
{
    return kind() == 0;
}

char OpcodeDescriptor::isNop()
{
    return kind() == 0 && kindData == 0;
}

void OpcodeDescriptor::scanOpcodeOperandDescriptors()
{
    int operandIndex;
    for (operandIndex = 0; operandIndex != operandDescriptors[operandIndex].operandKind; operandIndex++)
        ;
}

void OpcodeDescriptor::noopOpcodeDescriptorHook()
{
}

bool OpcodeDescriptor::check()
{
    bool noImplicitYet = true;
    iOpDesc *operandDescriptor = operandDescriptors;
    for (int operandIndex = 0; operandDescriptor->operandKind; operandIndex++, operandDescriptor++) {
        if (operandDescriptor->patchedOutputIndexPlusOne) {
            if (!operandDescriptor->isImplicit) {
                reportError("implicit web patch set on explicit operand\n");
                return false;
            }
            if (operandDescriptor->direction != VCL_OPERAND_READ) {
                reportError("iInfo: check - patch set on non-input (%d)\n", operandDescriptor->operandKind);
                return false;
            }
            if (operandDescriptors[operandDescriptor->patchedOutputIndexPlusOne - 1].direction != VCL_OPERAND_WRITE) {
                reportError("iInfo: check patch not point to output\n");
                return false;
            }
            if (operandDescriptors[operandDescriptor->patchedOutputIndexPlusOne - 1].isImplicit) {
                reportError("implicit web patch links to implicit operand\n");
                return false;
            }
            if (operandDescriptor->patchedOutputIndexPlusOne - 1 > operandIndex) {
                reportError(iInfoPatchedAfterPatchError);
                return false;
            }
        }
        if (operandDescriptor->isImplicit)
            noImplicitYet = false;
        else if (!noImplicitYet) {
            reportError(iInfoExplicitAfterImplicitError);
            return false;
        }
    }
    return true;
}

bool checkTable(OpcodeDescriptor *tab, int count)
{
    if (!tab) {
        reportError("checkTable - passed a null table\n");
        return false;
    }
    int opcodeIndex;
    for (opcodeIndex = 0; tab->mnemonic; opcodeIndex++, tab++) {
        if (!tab->check()) {
            reportError("checkTable failed\n");
            return false;
        }
    }
    if (opcodeIndex != count) {
        reportError("instruction count mismatch %d!=%d\n", opcodeIndex, count);
        return false;
    }
    return true;
}

int OpcodeDescriptor::branchFlagBit1()
{
    unsigned int branchKindBits = kind() == 3 ? kindData : 0;
    return (branchKindBits >> 1) & 1;
}

int OpcodeDescriptor::branchFlagBit3()
{
    unsigned int branchKindBits = kind() == 3 ? kindData : 0;
    return (branchKindBits >> 3) & 1;
}

int OpcodeDescriptor::branchFlagBit2()
{
    unsigned int branchKindBits = kind() == 3 ? kindData : 0;
    return (branchKindBits >> 2) & 1;
}

int OpcodeDescriptor::canPruneOperands()
{
    int canPrune = 0;
    if (!(kindAndFlags & 0xf8)) {
        int k2 = kind() == 2 ? kindData : 0;
        if (!k2) {
            int ramTypeValue;
            if (kind() == 4) {
                unsigned char ramType = kindData;
                if ((unsigned char)(ramType - 1) <= 1)
                    ramTypeValue = ramType;
                else {
                    reportError(iInfoLoadstoreRamtypeError);
                    exitAfterCleanup(-1);
                    goto none;
                }
            } else
none:
                ramTypeValue = 0;
            if (ramTypeValue != 2) {
                int k3 = kind() == 3 ? kindData : 0;
                if (!k3)
                    canPrune = 1;
            }
        }
    }
    return canPrune;
}

int decodeImmediateCategory(unsigned char immediateFlags)
{
    int category = 0;
    if (immediateFlags) {
        unsigned int immediateClassBits = immediateFlags & 0x60;
        category = (immediateClassBits >> 5) + 1;
    }
    return category;
}

int defaultImmediateFlags(unsigned char unusedImmediateKind)
{
    return 0;
}

unsigned char immediateFlagBit7(unsigned char immediateFlags)
{
    return immediateFlags >> 7;
}

Instr *getMove(int type, int);
