extern const char iInfoLoadstoreRamtypeError[];
extern const char iInfoExplicitAfterImplicitError[];
extern const char iInfoPatchedAfterPatchError[];
#include "Instr.h"
#include "Block.h"
#include "os.h"

unsigned char iInfo::branchFlags()
{
    if (kind() == 3)
        return kindData;
    return 0;
}

unsigned char iInfo::ramType()
{
    if (kind() == 4) {
        unsigned char t = kindData;
        if ((unsigned char)(t - 1) <= 1)
            return t;
        reportError(iInfoLoadstoreRamtypeError);
        exitAfterCleanup(-1);
    }
    return 0;
}

int iInfo::syntheticKind()
{
    if (kind() == 2)
        return kindData;
    return 0;
}

bool iInfo::isRawOnly()
{
    return kind() == 0;
}

char iInfo::isNop()
{
    return kind() == 0 && kindData == 0;
}

void iInfo::scanOpcodeOperandDescriptors()
{
    int operandIndex;
    for (operandIndex = 0; operandIndex != operandDescriptors[operandIndex].operandKind; operandIndex++)
        ;
}

void iInfo::noopOpcodeDescriptorHook()
{
}

bool iInfo::check()
{
    bool noImplicitYet = true;
    iOpDesc *operandDescriptor = operandDescriptors;
    for (int i = 0; operandDescriptor->operandKind; i++, operandDescriptor++) {
        if (operandDescriptor->patchedOutputIndexPlusOne) {
            if (!operandDescriptor->isImplicit) {
                reportError("implicit web patch set on explicit operand\n");
                return false;
            }
            if (operandDescriptor->direction != 1) {
                reportError("iInfo: check - patch set on non-input (%d)\n", operandDescriptor->operandKind);
                return false;
            }
            if (operandDescriptors[operandDescriptor->patchedOutputIndexPlusOne - 1].direction != 2) {
                reportError("iInfo: check patch not point to output\n");
                return false;
            }
            if (operandDescriptors[operandDescriptor->patchedOutputIndexPlusOne - 1].isImplicit) {
                reportError("implicit web patch links to implicit operand\n");
                return false;
            }
            if (operandDescriptor->patchedOutputIndexPlusOne - 1 > i) {
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

bool checkTable(iInfo *tab, int count)
{
    if (!tab) {
        reportError("checkTable - passed a null table\n");
        return false;
    }
    int i;
    for (i = 0; tab->mnemonic; i++, tab++) {
        if (!tab->check()) {
            reportError("checkTable failed\n");
            return false;
        }
    }
    if (i != count) {
        reportError("instruction count mismatch %d!=%d\n", i, count);
        return false;
    }
    return true;
}

int iInfo::branchFlagBit1()
{
    unsigned int v = kind() == 3 ? kindData : 0;
    return (v >> 1) & 1;
}

int iInfo::branchFlagBit3()
{
    unsigned int v = kind() == 3 ? kindData : 0;
    return (v >> 3) & 1;
}

int iInfo::branchFlagBit2()
{
    unsigned int v = kind() == 3 ? kindData : 0;
    return (v >> 2) & 1;
}

int iInfo::canPruneOperands()
{
    int canPrune = 0;
    if (!(kindAndFlags & 0xf8)) {
        int k2 = kind() == 2 ? kindData : 0;
        if (!k2) {
            int ramTypeValue;
            if (kind() == 4) {
                unsigned char t = kindData;
                if ((unsigned char)(t - 1) <= 1)
                    ramTypeValue = t;
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
        unsigned int f = immediateFlags & 0x60;
        category = (f >> 5) + 1;
    }
    return category;
}

int defaultImmediateFlags(unsigned char)
{
    return 0;
}

unsigned char immediateFlagBit7(unsigned char immediateFlags)
{
    return immediateFlags >> 7;
}

Instr *getMove(int type, int);
