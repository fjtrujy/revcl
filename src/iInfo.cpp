extern const char iInfoLoadstoreRamtypeError[];
extern const char iInfoExplicitAfterImplicitError[];
extern const char iInfoPatchedAfterPatchError[];
#include "Instr.h"
#include "Block.h"
#include "os.h"

unsigned char iInfo::func_0806ffb4()
{
    if (kind() == 3)
        return unk_19;
    return 0;
}

unsigned char iInfo::ramType()
{
    if (kind() == 4) {
        unsigned char t = unk_19;
        if ((unsigned char)(t - 1) <= 1)
            return t;
        reportError(iInfoLoadstoreRamtypeError);
        exitAfterCleanup(-1);
    }
    return 0;
}

int iInfo::func_08070020()
{
    if (kind() == 2)
        return unk_19;
    return 0;
}

bool iInfo::func_08070044()
{
    return kind() == 0;
}

char iInfo::func_0807005c()
{
    return kind() == 0 && unk_19 == 0;
}

void iInfo::func_08070080()
{
    int i;
    for (i = 0; i != unk_44[i].unk_00; i++)
        ;
}

void iInfo::func_080700a4()
{
}

bool iInfo::check()
{
    bool noImplicitYet = true;
    iOpDesc *d = unk_44;
    for (int i = 0; d->unk_00; i++, d++) {
        if (d->unk_06) {
            if (!d->unk_02) {
                reportError("implicit web patch set on explicit operand\n");
                return false;
            }
            if (d->unk_01 != 1) {
                reportError("iInfo: check - patch set on non-input (%d)\n", d->unk_00);
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_01 != 2) {
                reportError("iInfo: check patch not point to output\n");
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_02) {
                reportError("implicit web patch links to implicit operand\n");
                return false;
            }
            if (d->unk_06 - 1 > i) {
                reportError(iInfoPatchedAfterPatchError);
                return false;
            }
        }
        if (d->unk_02)
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
    for (i = 0; tab->unk_04; i++, tab++) {
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
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 1) & 1;
}

int iInfo::func_0807021c()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 3) & 1;
}

int iInfo::func_08070244()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 2) & 1;
}

int iInfo::func_0807026c()
{
    int r = 0;
    if (!(unk_18 & 0xf8)) {
        int k2 = kind() == 2 ? unk_19 : 0;
        if (!k2) {
            int rt;
            if (kind() == 4) {
                unsigned char t = unk_19;
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
                int k3 = kind() == 3 ? unk_19 : 0;
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
