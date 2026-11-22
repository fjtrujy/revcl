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

void iInfo::func_08070080()
{
    int i;
    for (i = 0; i != operandDescriptors[i].operandKind; i++)
        ;
}

void iInfo::func_080700a4()
{
}

bool iInfo::check()
{
    bool noImplicitYet = true;
    iOpDesc *d = operandDescriptors;
    for (int i = 0; d->operandKind; i++, d++) {
        if (d->patchedOutputIndexPlusOne) {
            if (!d->isImplicit) {
                reportError("implicit web patch set on explicit operand\n");
                return false;
            }
            if (d->direction != 1) {
                reportError("iInfo: check - patch set on non-input (%d)\n", d->operandKind);
                return false;
            }
            if (operandDescriptors[d->patchedOutputIndexPlusOne - 1].direction != 2) {
                reportError("iInfo: check patch not point to output\n");
                return false;
            }
            if (operandDescriptors[d->patchedOutputIndexPlusOne - 1].isImplicit) {
                reportError("implicit web patch links to implicit operand\n");
                return false;
            }
            if (d->patchedOutputIndexPlusOne - 1 > i) {
                reportError(iInfoPatchedAfterPatchError);
                return false;
            }
        }
        if (d->isImplicit)
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

int iInfo::func_080701f4()
{
    unsigned int v = kind() == 3 ? kindData : 0;
    return (v >> 1) & 1;
}

int iInfo::func_0807021c()
{
    unsigned int v = kind() == 3 ? kindData : 0;
    return (v >> 3) & 1;
}

int iInfo::func_08070244()
{
    unsigned int v = kind() == 3 ? kindData : 0;
    return (v >> 2) & 1;
}

int iInfo::canPruneOperands()
{
    int r = 0;
    if (!(kindAndFlags & 0xf8)) {
        int k2 = kind() == 2 ? kindData : 0;
        if (!k2) {
            int rt;
            if (kind() == 4) {
                unsigned char t = kindData;
                if ((unsigned char)(t - 1) <= 1)
                    rt = t;
                else {
                    reportError(iInfoLoadstoreRamtypeError);
                    exitAfterCleanup(-1);
                    goto none;
                }
            } else
none:
                rt = 0;
            if (rt != 2) {
                int k3 = kind() == 3 ? kindData : 0;
                if (!k3)
                    r = 1;
            }
        }
    }
    return r;
}

int func_08070300(unsigned char x)
{
    int r = 0;
    if (x) {
        unsigned int f = x & 0x60;
        r = (f >> 5) + 1;
    }
    return r;
}

int func_0807031c(unsigned char)
{
    return 0;
}

unsigned char func_08070324(unsigned char x)
{
    return x >> 7;
}

Instr *getMove(int type, int);
