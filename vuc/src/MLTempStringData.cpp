// Only MLTempStringData::Get() is byte-exact in this provisional unit.
#include "MLTempStringData.h"

MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}
