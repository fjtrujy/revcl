// Tagged numeric/string value, with ownership of STRING storage.
#ifndef MVALUE_H
#define MVALUE_H

class TypedValue {
public:
    enum { NONE, UINT, INT, FLOAT, STRING };
    unsigned int type;  // STRING owns a new[] buffer
    union {
        unsigned int u;
        int i;
        float f;
        char *str;
    };

    TypedValue();
    TypedValue(unsigned int initialType)
    {
        str = 0;
        type = initialType;
    }
    ~TypedValue();
    void clear();
    bool getFloat(float *out);
    bool getUInt(unsigned int *out);
    bool getInt(int *out);
    bool set(TypedValue *src);
    bool setInt(int newValue);
    bool setString(const char *s);
    bool setString(const char *s, int n);
    bool setUInt(unsigned int newValue);
    bool setFloat(float newValue);
    bool isValid();
    bool isNumeric();
    bool isInt();
    bool isFloat();
    bool negate(TypedValue *src);
    bool abs(TypedValue *src);
    bool logicalNot(TypedValue *src);
    bool bitNot(TypedValue *src);
    bool parse(const char *literalText);
    void dump();
    char *getString();
    bool makeNumeric();
    char *asChars();
};

bool addValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool subtractValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool multiplyValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool divideValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool moduloValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool shiftValuesLeft(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool shiftValuesRight(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool logicalOrValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool logicalAndValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool bitwiseAndValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool bitwiseOrValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool bitwiseXorValues(TypedValue *dst, TypedValue *leftValue, TypedValue *rightValue);
bool sameImmediateValue(TypedValue *leftValue, TypedValue *rightValue);

#endif
