#include "os.h"
extern const char regUseNewFailedError[];
#include "RegPool_shared.h"

regUse::regUse(RegPoolEntry *reg)
{
    unk_08 = reg;
    unk_0c = 0;
    unk_0d = 0;
    unk_0e = 0;
    unk_0f = 0;
    unk_10 = 0;
    unk_11 = 0;
    unk_12 = 0;
    unk_13 = 0;
    unk_14 = 0;
    unk_15 = -1;
    unk_00 = 0;
    unk_04 = 0;
    unk_18 = reg->unk_36;
    unk_1c = reg->throughputWriter;
}

regUse::~regUse()
{
}

regUseList::regUseList()
{
    unk_08 = 0;
    unk_04 = 0;
    unk_00 = 0;
}

regUseList::~regUseList()
{
    regUse *ru = unk_04;
    while (ru) {
        regUse *next = ru->unk_04;
        if (ru->unk_04)
            ru->unk_04->unk_00 = ru->unk_00;
        else
            unk_08 = ru->unk_00;
        if (ru->unk_00)
            ru->unk_00->unk_04 = ru->unk_04;
        else
            unk_04 = ru->unk_04;
        unk_00--;
        ru->unk_00 = 0;
        ru->unk_04 = 0;
        delete ru;
        ru = next;
    }
}

bool regUseList::clear()
{
    regUse *ru = unk_04;
    while (ru) {
        regUse *next = ru->unk_04;
        if (ru->unk_04)
            ru->unk_04->unk_00 = ru->unk_00;
        else
            unk_08 = ru->unk_00;
        if (ru->unk_00)
            ru->unk_00->unk_04 = ru->unk_04;
        else
            unk_04 = ru->unk_04;
        unk_00--;
        ru->unk_00 = 0;
        ru->unk_04 = 0;
        delete ru;
        ru = next;
    }
    return true;
}

regUse *regUseList::AddNewRegRef(RegPoolEntry *reg)
{
    if (find(reg)) {
        reportError("addnewregref - already have rg\n");
        exitAfterCleanup(-1);
        return 0;
    }
    regUse *ru = new regUse(reg);
    if (!ru) {
        reportError(regUseNewFailedError);
        exitAfterCleanup(-1);
        return 0;
    }
    ru->unk_00 = 0;
    ru->unk_04 = 0;
    if (!unk_08) {
        unk_04 = ru;
    } else {
        unk_08->unk_04 = ru;
        ru->unk_00 = unk_08;
    }
    unk_08 = ru;
    unk_00++;
    return ru;
}

bool regUseList::append(regUse *ru)
{
    ru->unk_00 = 0;
    ru->unk_04 = 0;
    if (!unk_08) {
        unk_04 = ru;
    } else {
        unk_08->unk_04 = ru;
        ru->unk_00 = unk_08;
    }
    unk_08 = ru;
    unk_00++;
    return true;
}

bool regUseList::remove(regUse *ru)
{
    if (ru->unk_04)
        ru->unk_04->unk_00 = ru->unk_00;
    else
        unk_08 = ru->unk_00;
    if (ru->unk_00)
        ru->unk_00->unk_04 = ru->unk_04;
    else
        unk_04 = ru->unk_04;
    unk_00--;
    ru->unk_00 = 0;
    ru->unk_04 = 0;
    return true;
}

bool regUseList::func_0805b0cc()
{
    int n = 0;
    for (regUse *ru = unk_04; ru; ru = ru->unk_04) {
        if (!ru->unk_08)
            return false;
        n++;
    }
    return n == unk_00;
}

bool regUseList::func_0805b0f8()
{
    return unk_08 != 0;
}

regUseSlot::regUseSlot()
{
    unk_34[1] = 0;
    unk_34[0] = 0;
    unk_34[3] = 0;
    unk_34[2] = 0;
    unk_3c[0] = -1;
    unk_3c[2] = -1;
    unk_3c[1] = -1;
    unk_3c[3] = -1;
    unk_3c[4] = -1;
    unk_3c[5] = -1;
    unk_3c[6] = -1;
    unk_3c[7] = -1;
    unk_3c[8] = -1;
    unk_3c[9] = -1;
    unk_3c[10] = -1;
    unk_3c[11] = -1;
    unk_3c[12] = -1;
    unk_30 = -1;
    unk_32 = -1;
    unk_58 = 0;
    unk_5c.func_0805bcc4();
}
