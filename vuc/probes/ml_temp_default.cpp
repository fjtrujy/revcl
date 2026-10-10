// Experimental default MLTempString constructor, inline static singleton.
#include "../src/MLTempStringData.h"
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
