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
    bool setInt(int newValue);
    bool setString(const char *s);
    bool setString(const char *s, int n);
    bool setUInt(unsigned int newValue);
    bool setFloat(float newValue);
    bool isValid();
    bool isNumeric();
    bool isInt();
    bool isFloat();
    bool negate(mValue *src);
    bool abs(mValue *src);
    bool logicalNot(mValue *src);
    bool bitNot(mValue *src);
    bool parse(const char *literalText);
    void dump();
    char *getString();
    bool makeNumeric();
    char *asChars();
};

bool addValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool subtractValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool multiplyValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool divideValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool moduloValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool shiftValuesLeft(mValue *dst, mValue *leftValue, mValue *rightValue);
bool shiftValuesRight(mValue *dst, mValue *leftValue, mValue *rightValue);
bool logicalOrValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool logicalAndValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool bitwiseAndValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool bitwiseOrValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool bitwiseXorValues(mValue *dst, mValue *leftValue, mValue *rightValue);
bool sameImmediateValue(mValue *leftValue, mValue *rightValue);

#endif
