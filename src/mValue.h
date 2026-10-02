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
    mValue(char *const &s)
    {
        str = 0;
        type = NONE;
        parse(s);
    }
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
    bool parse(const char *s);
    void dump();
    char *getString();
    bool makeNumeric();
    char *asChars();
};

bool func_0804ca94(mValue *dst, mValue *a, mValue *b);
bool func_0804cd68(mValue *dst, mValue *a, mValue *b);
bool func_0804d040(mValue *dst, mValue *a, mValue *b);
bool func_0804d314(mValue *dst, mValue *a, mValue *b);
bool func_0804d5ec(mValue *dst, mValue *a, mValue *b);
bool func_0804d7d4(mValue *dst, mValue *a, mValue *b);
bool func_0804d9c0(mValue *dst, mValue *a, mValue *b);
bool func_0804dbac(mValue *dst, mValue *a, mValue *b);
bool func_0804ddc0(mValue *dst, mValue *a, mValue *b);
bool func_0804dfd4(mValue *dst, mValue *a, mValue *b);
bool func_0804e050(mValue *dst, mValue *a, mValue *b);
bool func_0804e0cc(mValue *dst, mValue *a, mValue *b);
bool func_0804e5c0(mValue *a, mValue *b);

#endif
