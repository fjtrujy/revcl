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
    mValue(unsigned int initialType)
    {
        str = 0;
        type = initialType;
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

bool addValues(mValue *dst, mValue *a, mValue *b);
bool subtractValues(mValue *dst, mValue *a, mValue *b);
bool multiplyValues(mValue *dst, mValue *a, mValue *b);
bool divideValues(mValue *dst, mValue *a, mValue *b);
bool moduloValues(mValue *dst, mValue *a, mValue *b);
bool shiftValuesLeft(mValue *dst, mValue *a, mValue *b);
bool shiftValuesRight(mValue *dst, mValue *a, mValue *b);
bool logicalOrValues(mValue *dst, mValue *a, mValue *b);
bool logicalAndValues(mValue *dst, mValue *a, mValue *b);
bool bitwiseAndValues(mValue *dst, mValue *a, mValue *b);
bool bitwiseOrValues(mValue *dst, mValue *a, mValue *b);
bool bitwiseXorValues(mValue *dst, mValue *a, mValue *b);
bool sameImmediateValue(mValue *a, mValue *b);

#endif
