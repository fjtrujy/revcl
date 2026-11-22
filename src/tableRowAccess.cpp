#include "loadStalls_shared.h"

wtRow *workTable::getRowByIndex(int rowIndex)
{
    if (!unk_2c)
        return 0;
    if (rowIndex < 0 || rowIndex >= unk_28)
        return 0;
    return &unk_2c[rowIndex];
}
