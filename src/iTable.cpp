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
