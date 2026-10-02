#include "loadStalls_shared.h"

wtRow *workTable::func_080724f0(int i)
{
    if (!unk_2c)
        return 0;
    if (i < 0 || i >= unk_28)
        return 0;
    return &unk_2c[i];
}
