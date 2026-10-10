// Provisional MLTempStringData layout from original ELF. Not original header.
#ifndef VUC_ML_TEMP_STRING_DATA_H
#define VUC_ML_TEMP_STRING_DATA_H

#include <cstring>
class MLTempStringData {
public:
    static MLTempStringData *Get();
    MLTempStringData() {
        memset(bytes_, 'X', sizeof bytes_);
        bytes_[0x1fbf] = 0;
        used_ = 0;
    }
    char *FormatString(const char *format, ...);
    char *VFormatString(const char *format, char *arguments);
private:
    char bytes_[0x2fc0];
    unsigned used_;
};
#endif
