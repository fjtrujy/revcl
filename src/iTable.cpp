// iTable: work table construction and management (CIE group 24).
#include "workTable.h"

workTable::workTable()
{
    unk_2c = 0;
    unk_4c = 0;
    unk_24 = 0;
    unk_20 = 0;
    unk_0b = 0;
    unk_10 = 0;
    dispose();
}

workTable::~workTable()
{
    dispose();
}

bool workTable::init(unsigned char mode, int size)
{
    dispose();
    switch (mode) {
    case 2:
    case 3:
    case 5:
        size += 4;
        if (size < 4)
            size = 4;
        unk_2c = new wtRow[size];
        if (!unk_2c)
            return false;
        unk_28 = size;
        break;
    case 1:
    case 4:
        if (size) {
            func_0804ad38("can't ask for a table based itable (%d) for a synth/uta table...like.. perleaze make me an abstract base class and leave me alone!\n", size);
            return false;
        }
        break;
    default:
        func_0804ad38("iTable::Init() -%s for Init\n", codeModeStr(mode));
        return false;
    }
    unk_30 = 0;
    unk_34 = 0;
    unk_38 = 0;
    unk_ac = 0;
    unk_40 = 0;
    unk_ad = 0;
    unk_0c = 0;
    unk_7a = 0;
    unk_09 = mode;
    return true;
}

bool workTable::dispose()
{
    Instr *p = unk_94.unk_00;
    while (p) {
        Instr *next = p->unk_24.unk_00;
        if (p->unk_13e8.unk_04 < 0)
            delete p;
        p = next;
    }
    unk_0b = 0;
    if (!freeLockLists(unk_24)) {
        func_0804ad38("failed to dispose rll list\n");
        func_0804aeb0();
    }
    unk_24 = 0;
    unk_20 = 0;
    unk_14.clear();
    delete[] unk_2c;
    unk_2c = 0;
    unk_28 = 0;
    unk_38 = 0;
    unk_30 = 0;
    unk_34 = 0;
    unk_3c = 0;
    unk_00 = -1;
    unk_09 = 0;
    unk_08 = 0;
    unk_0a = 0;
    unk_c8 = 0;
    unk_bc = 0;
    unk_b0 = 0;
    unk_b4 = 0;
    unk_c0 = 0;
    unk_b8 = 0;
    unk_c4 = 0;
    unk_4c = 0;
    unk_88.reset();
    unk_94.reset();
    unk_a0.reset();
    unk_50 = 0;
    unk_40 = 0;
    unk_0c = -1;
    unk_10 = 0;
    return true;
}
