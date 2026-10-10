// Byte-exact C1 constructor ABI; C2 emitted in this object remains experimental.
#include "MLTempStringData.h"
inline MLTempStringData *MLTempStringData::Get()
{
    static MLTempStringData tsd;
    return &tsd;
}
struct MLTempString {
    const char *text;
    MLTempString();
};
MLTempString::MLTempString()
{
    text = MLTempStringData::Get()->FormatString("");
}
