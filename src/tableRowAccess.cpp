#include "loadStalls_shared.h"

wtRow *workTable::getRowByIndex(int rowIndex)
{
    if (!rows)
        return 0;
    if (rowIndex < 0 || rowIndex >= rowCapacity)
        return 0;
    return &rows[rowIndex];
}
