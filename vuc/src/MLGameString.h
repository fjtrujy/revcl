// Provisional VUC string layout, reconstructed from ELF32 function behavior.
// This is not yet an authenticated original 2004 header.
#ifndef VUC_ML_GAME_STRING_H
#define VUC_ML_GAME_STRING_H

struct MLGameStringData {
    unsigned hash;
    const char *text;
};

template <class T> class MLHashFun;

class MLGameString {
public:
    MLGameString();
    MLGameString(const MLGameString &other);
    const char *c_str() const;

private:
    MLGameStringData *data_;
    template <class T> friend class MLHashFun;
};

#endif
