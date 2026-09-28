// Small typed value: int tag + owned string (class name guessed).
#ifndef MVALUE_H
#define MVALUE_H

class mValue {
public:
    enum { NONE, UINT, INT, FLOAT, STRING };
    unsigned int type;  // STRING owns a new[] buffer
    union {
        unsigned int u;
        int i;
        float f;
        char *str;
    };

    mValue();
    ~mValue();
    void clear();
    bool getFloat(float *out);
    bool getUInt(unsigned int *out);
    bool getInt(int *out);
    bool set(mValue *src);
    bool setInt(int v);
    bool setString(const char *s);
    bool setString(const char *s, int n);
    bool setUInt(unsigned int v);
    bool setFloat(float v);
    bool isValid();
    bool isNumeric();
    bool isInt();
    bool isFloat();
    bool negate(mValue *src);
    bool abs(mValue *src);
    bool logicalNot(mValue *src);
    bool bitNot(mValue *src);
};

#endif
