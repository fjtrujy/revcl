#include "Instr.h"
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
        func_0804ad38("loadstore instruction, but bad ramtype\n");
        func_0804adf0(-1);
    }
    return 0;
}

unsigned char iInfo::func_08070020()
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
                func_0804ad38("implicit web patch set on explicit operand\n");
                return false;
            }
            if (d->unk_01 != 1) {
                func_0804ad38("iInfo: check - patch set on non-input (%d)\n", d->unk_00);
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_01 != 2) {
                func_0804ad38("iInfo: check patch not point to output\n");
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_02) {
                func_0804ad38("implicit web patch links to implicit operand\n");
                return false;
            }
            if (d->unk_06 - 1 > i) {
                func_0804ad38("patched operand is after patch\n");
                return false;
            }
        }
        if (d->unk_02)
            noImplicitYet = false;
        else if (!noImplicitYet) {
            func_0804ad38("explicit operand after an implicit\n");
            return false;
        }
    }
    return true;
}
