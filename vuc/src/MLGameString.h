// Provisional VUC string layout, reconstructed from ELF32 function behavior.
// This is not yet an authenticated original 2004 header.
#ifndef VUC_ML_GAME_STRING_H
#define VUC_ML_GAME_STRING_H

struct MLGameStringData {
    unsigned hash;
    const char *text;
    unsigned length;
    bool temporary;
    MLGameStringData(const char *value, bool temporary);
};

struct MLTempString {
    const char *text;
    MLTempString(const char *format, ...);
    void Set(const char *format, ...);
    MLTempString &operator+=(const MLTempString &other);
    MLTempString operator+(const MLTempString &other);
};

template <class T> class MLHashFun;

template <> class MLHashFun<const char *> {
public:
    unsigned Hash(const char * const &value);
    bool IsEqual(const char * const &lhs, const char * const &rhs);
};

class MLGameString {
public:
    MLGameString();
    MLGameString(const MLGameString &other);
    MLGameString(const char *text, bool temporary);
    MLGameString(const MLTempString &temporary);
    const char *c_str() const;
    bool operator==(const MLGameString &other) const {
        return data_ == other.data_;
    }

private:
    MLGameStringData *data_;
    template <class T> friend class MLHashFun;
};

#endif
