// mValue: typed value (int / uint / float / string) and string helpers.

#include <stdio.h>
#include <stdlib.h>
#include "Block.h"
#include "Instr.h"
#include "mValue.h"
#include "os.h"

bool parseFloatImmediateMode = false;
const char *immediateLiteralCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_@.[]";
const char *immediateLiteralDigitsByBase[3] = { "0123456789", "0123456789abcdef", "01" };

bool asciiCaseEqual(const char *a, const char *b);
char *dupString(const char *s);
int mstrlen(const char *s);

mValue::mValue()
{
    str = 0;
    type = NONE;
}

mValue::~mValue()
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
}

void mValue::clear()
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
}

int maximumValueType(int leftType, int rightType)
{
    if (rightType > leftType)
        leftType = rightType;
    return leftType;
}

int promoteIntegralValueType(int leftType, int rightType)
{
    int promotedType = leftType;
    if (rightType > leftType)
        promotedType = rightType;
    if (promotedType == mValue::FLOAT)
        return mValue::NONE;
    return promotedType;
}

bool mValue::getFloat(float *out)
{
    switch (type) {
    case UINT: *out = (float)u; break;
    case INT: *out = (float)i; break;
    case FLOAT: *out = f; break;
    default: return false;
    }
    return true;
}

bool mValue::getUInt(unsigned int *out)
{
    switch (type) {
    case UINT: *out = u; break;
    case INT: *out = i; break;
    case FLOAT: *out = (unsigned int)f; break;
    default: return false;
    }
    return true;
}

bool mValue::getInt(int *out)
{
    switch (type) {
    case UINT: *out = u; break;
    case INT: *out = i; break;
    case FLOAT: *out = (int)f; break;
    default: return false;
    }
    return true;
}

bool mValue::set(mValue *src)
{
    if (!src) {
        reportError("xpVal::set() - source is null ptr\n");
        return false;
    }
    if (src->type < UINT || src->type > STRING) {
        reportError("xpVal::set() - source is invalid\n");
        return false;
    }
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    type = src->type;
    if (type == STRING) {
        str = dupString(src->str);
        if (!str) {
            reportError("failed to dup literal\n");
            if (type == STRING) {
                if (str)
                    delete[] str;
                str = 0;
            }
            type = NONE;
            return false;
        }
    } else
        u = src->u;
    return true;
}

bool mValue::setInt(int newValue)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    i = newValue;
    type = INT;
    return true;
}

bool mValue::setString(const char *s)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    type = STRING;
    str = dupString(s);
    if (!str) {
        reportError("failed to dup string for set str\n");
        type = NONE;
        return false;
    }
    return true;
}

bool mValue::setString(const char *s, int n)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    if (!s)
        return false;
    type = STRING;
    str = new char[n + 1];
    if (!str) {
        reportError("failed to new chars for set str\n");
        type = NONE;
        return false;
    }
    const char *p = s;
    char *d = str;
    while (n && *p) {
        *d++ = *p++;
        n--;
    }
    *d = 0;
    return true;
}

bool mValue::setUInt(unsigned int newValue)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    u = newValue;
    type = UINT;
    return true;
}

bool mValue::setFloat(float newValue)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    type = FLOAT;
    f = newValue;
    return true;
}

bool mValue::isValid()
{
    return type >= UINT && type <= STRING;
}

bool mValue::isNumeric()
{
    return type >= UINT && type <= FLOAT;
}

bool mValue::isInt()
{
    return type >= UINT && type <= INT;
}

bool mValue::isFloat()
{
    return type == FLOAT;
}

bool mValue::negate(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: u = -u; break;
    case INT: i = -i; break;
    case FLOAT: f = -f; break;
    }
    return true;
}

bool mValue::abs(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: break;
    case INT: if (i < 0) i = -i; break;
    case FLOAT: if (f < 0) f = -f; break;
    }
    return true;
}

bool mValue::logicalNot(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: u = !u; break;
    case INT: i = !i; break;
    case FLOAT: return false;
    }
    return true;
}

bool mValue::bitNot(mValue *src)
{
    if (!set(src))
        return false;
    switch (type) {
    case UINT: u = ~u; break;
    case INT: i = ~i; break;
    case FLOAT: return false;
    }
    return true;
}

const char *immediateValueTypeName(int type)
{
    switch (type) {
    case 3: return "EXP_DATATYPE_FLOAT";
    case 0: return "EXP_DATATYPE_NULL";
    case 1: return "EXP_DATATYPE_UINT";
    case 2: return "EXP_DATATYPE_SINT";
    case 4: return "EXP_DATATTYPE_LITERAL";
    }
    return "EXP_DATATYPE_UNKNOWN";
}

static inline bool getUIntInl(mValue *sourceValue, unsigned int *out)
{
    switch (sourceValue->type) {
    case mValue::UINT: *out = sourceValue->u; break;
    case mValue::INT: *out = sourceValue->i; break;
    case mValue::FLOAT: *out = (unsigned int)sourceValue->f; break;
    default: return false;
    }
    return true;
}

static inline bool getIntInl(mValue *sourceValue, int *out)
{
    switch (sourceValue->type) {
    case mValue::UINT: *out = sourceValue->u; break;
    case mValue::INT: *out = sourceValue->i; break;
    case mValue::FLOAT: *out = (int)sourceValue->f; break;
    default: return false;
    }
    return true;
}

static inline bool getFloatInl(mValue *sourceValue, float *out)
{
    switch (sourceValue->type) {
    case mValue::UINT: *out = (float)sourceValue->u; break;
    case mValue::INT: *out = (float)sourceValue->i; break;
    case mValue::FLOAT: *out = sourceValue->f; break;
    default: return false;
    }
    return true;
}

bool addValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    float leftFloat, rightFloat;
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    int promotedType;
    if (!leftValue)
        goto fail;
    if (!rightValue)
        goto fail;
    if (leftValue->type < mValue::UINT || leftValue->type > mValue::FLOAT)
        goto fail;
    if (rightValue->type < mValue::UINT || rightValue->type > mValue::FLOAT)
        goto fail;
    promotedType = leftValue->type;
    if ((int)rightValue->type > promotedType)
        promotedType = rightValue->type;
    dst->type = promotedType;
    if (promotedType == mValue::NONE)
        return false;
    switch ((unsigned)promotedType) {
    case mValue::FLOAT:
        if (getFloatInl(leftValue, &leftFloat) && getFloatInl(rightValue, &rightFloat)) {
            dst->f = leftFloat + rightFloat;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(leftValue, &leftInt) || !getIntInl(rightValue, &rightInt)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = leftInt + rightInt;
        return true;
    case mValue::UINT:
        if (!getUIntInl(leftValue, &leftUnsigned) || !getUIntInl(rightValue, &rightUnsigned)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = leftUnsigned + rightUnsigned;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

bool subtractValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    float leftFloat, rightFloat;
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    int promotedType;
    if (!leftValue)
        goto fail;
    if (!rightValue)
        goto fail;
    if (leftValue->type < mValue::UINT || leftValue->type > mValue::FLOAT)
        goto fail;
    if (rightValue->type < mValue::UINT || rightValue->type > mValue::FLOAT)
        goto fail;
    promotedType = leftValue->type;
    if ((int)rightValue->type > promotedType)
        promotedType = rightValue->type;
    dst->type = promotedType;
    if (promotedType == mValue::NONE)
        return false;
    switch ((unsigned)promotedType) {
    case mValue::FLOAT:
        if (getFloatInl(leftValue, &leftFloat) && getFloatInl(rightValue, &rightFloat)) {
            dst->f = leftFloat - rightFloat;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(leftValue, &leftInt) || !getIntInl(rightValue, &rightInt)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = leftInt - rightInt;
        return true;
    case mValue::UINT:
        if (!getUIntInl(leftValue, &leftUnsigned) || !getUIntInl(rightValue, &rightUnsigned)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = leftUnsigned - rightUnsigned;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

bool multiplyValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    float leftFloat, rightFloat;
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    int promotedType;
    if (!leftValue)
        goto fail;
    if (!rightValue)
        goto fail;
    if (leftValue->type < mValue::UINT || leftValue->type > mValue::FLOAT)
        goto fail;
    if (rightValue->type < mValue::UINT || rightValue->type > mValue::FLOAT)
        goto fail;
    promotedType = leftValue->type;
    if ((int)rightValue->type > promotedType)
        promotedType = rightValue->type;
    dst->type = promotedType;
    if (promotedType == mValue::NONE)
        return false;
    switch ((unsigned)promotedType) {
    case mValue::FLOAT:
        if (getFloatInl(leftValue, &leftFloat) && getFloatInl(rightValue, &rightFloat)) {
            dst->f = leftFloat * rightFloat;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(leftValue, &leftInt) || !getIntInl(rightValue, &rightInt)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = leftInt * rightInt;
        return true;
    case mValue::UINT:
        if (!getUIntInl(leftValue, &leftUnsigned) || !getUIntInl(rightValue, &rightUnsigned)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = leftUnsigned * rightUnsigned;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

bool divideValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    float leftFloat, rightFloat;
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    int promotedType;
    if (!leftValue)
        goto fail;
    if (!rightValue)
        goto fail;
    if (leftValue->type < mValue::UINT || leftValue->type > mValue::FLOAT)
        goto fail;
    if (rightValue->type < mValue::UINT || rightValue->type > mValue::FLOAT)
        goto fail;
    promotedType = leftValue->type;
    if ((int)rightValue->type > promotedType)
        promotedType = rightValue->type;
    dst->type = promotedType;
    if (promotedType == mValue::NONE)
        return false;
    switch ((unsigned)promotedType) {
    case mValue::FLOAT:
        if (getFloatInl(leftValue, &leftFloat) && getFloatInl(rightValue, &rightFloat)) {
            dst->f = leftFloat / rightFloat;
            return true;
        }
        reportError("failed to get args\n");
        return false;
    case mValue::INT:
        if (!getIntInl(leftValue, &leftInt) || !getIntInl(rightValue, &rightInt)) {
            reportError("failed to get args\n");
            return false;
        }
        dst->i = leftInt / rightInt;
        return true;
    case mValue::UINT:
        if (!getUIntInl(leftValue, &leftUnsigned) || !getUIntInl(rightValue, &rightUnsigned)) {
            printf("failed to get args\n");
            return false;
        }
        dst->u = leftUnsigned / rightUnsigned;
        return true;
    }
    printf("bad type to expr\n");
    return false;
fail:
    return false;
}

static inline bool isNumType(unsigned int valueType)
{
    return valueType >= mValue::UINT && valueType <= mValue::FLOAT;
}

static inline int promoteIntType(int leftType, int rightType)
{
    int promotedType = leftType;
    if (rightType > leftType)
        promotedType = rightType;
    if (promotedType == mValue::FLOAT)
        return mValue::NONE;
    return promotedType;
}

bool moduloValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    if (!leftValue || !rightValue) {
        printf("xpVal::mod null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::mod invalid arg\n");
        return false;
    }
    int promotedType = promoteIntType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::mod type promote failed\n");
        return false;
    }
    if (promotedType != mValue::UINT) {
        if (promotedType == mValue::INT) {
            if (getIntInl(leftValue, &leftInt) && getIntInl(rightValue, &rightInt)) {
                dst->i = leftInt % rightInt;
                goto success;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(leftValue, &leftUnsigned) && getUIntInl(rightValue, &rightUnsigned)) {
        dst->u = leftUnsigned % rightUnsigned;
        goto success;
    }
    printf("failed to get args\n");
    return false;
success:
    return true;
}

bool shiftValuesLeft(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    if (!leftValue || !rightValue) {
        printf("xpVal::shfl null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::shfl invalid arg\n");
        return false;
    }
    int promotedType = promoteIntType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::shfl type promote failed\n");
        return false;
    }
    if (promotedType != mValue::UINT) {
        if (promotedType == mValue::INT) {
            if (getIntInl(leftValue, &leftInt) && getIntInl(rightValue, &rightInt)) {
                dst->i = leftInt << rightInt;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(leftValue, &leftUnsigned) && getUIntInl(rightValue, &rightUnsigned)) {
        dst->u = leftUnsigned << rightUnsigned;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

bool shiftValuesRight(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    if (!leftValue || !rightValue) {
        printf("xpVal::shfr null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::shfr invalid arg\n");
        return false;
    }
    int promotedType = promoteIntType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::shfr type promote failed\n");
        return false;
    }
    if (promotedType != mValue::UINT) {
        if (promotedType == mValue::INT) {
            if (getIntInl(leftValue, &leftInt) && getIntInl(rightValue, &rightInt)) {
                dst->i = leftInt >> rightInt;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(leftValue, &leftUnsigned) && getUIntInl(rightValue, &rightUnsigned)) {
        dst->u = leftUnsigned >> rightUnsigned;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

bool logicalOrValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    if (!leftValue || !rightValue) {
        printf("xpVal::logor null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::logor invalid arg\n");
        return false;
    }
    int promotedType = promoteIntType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::logor type promote failed\n");
        return false;
    }
    if (promotedType != mValue::UINT) {
        if (promotedType == mValue::INT) {
            if (getIntInl(leftValue, &leftInt) && getIntInl(rightValue, &rightInt)) {
                dst->i = leftInt || rightInt;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(leftValue, &leftUnsigned) && getUIntInl(rightValue, &rightUnsigned)) {
        dst->u = leftUnsigned || rightUnsigned;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

bool logicalAndValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    int leftInt, rightInt;
    unsigned int leftUnsigned, rightUnsigned;
    if (!leftValue || !rightValue) {
        printf("xpVal::logand null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::logand invalid arg\n");
        return false;
    }
    int promotedType = promoteIntType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::logand type promote failed\n");
        return false;
    }
    if (promotedType != mValue::UINT) {
        if (promotedType == mValue::INT) {
            if (getIntInl(leftValue, &leftInt) && getIntInl(rightValue, &rightInt)) {
                dst->i = leftInt && rightInt;
                return true;
            }
            printf("failed to get args\n");
            return false;
        }
        printf("bad type to expr\n");
        return false;
    }
    if (getUIntInl(leftValue, &leftUnsigned) && getUIntInl(rightValue, &rightUnsigned)) {
        dst->u = leftUnsigned && rightUnsigned;
        return true;
    }
    printf("failed to get args\n");
    return false;
}

static inline int promoteType(int leftType, int rightType)
{
    int promotedType = leftType;
    if (rightType > leftType)
        promotedType = rightType;
    return promotedType;
}

bool bitwiseAndValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    if (!leftValue || !rightValue) {
        printf("xpVal::bitand null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::bitand invalid arg\n");
        return false;
    }
    int promotedType = promoteType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::bitand type promote failed\n");
        return false;
    }
    dst->u = leftValue->u & rightValue->u;
    return true;
}

bool bitwiseOrValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    if (!leftValue || !rightValue) {
        printf("xpVal::bitor null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::bitor invalid arg\n");
        return false;
    }
    int promotedType = promoteType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::bitor type promote failed\n");
        return false;
    }
    dst->u = leftValue->u | rightValue->u;
    return true;
}

bool bitwiseXorValues(mValue *dst, mValue *leftValue, mValue *rightValue)
{
    if (!leftValue || !rightValue) {
        printf("xpVal::bitxor null arg passed\n");
        return false;
    }
    if (!isNumType(leftValue->type) || !isNumType(rightValue->type)) {
        printf("xpVal::bitxor invalid arg\n");
        return false;
    }
    int promotedType = promoteType(leftValue->type, rightValue->type);
    dst->type = promotedType;
    if (promotedType == mValue::NONE) {
        printf("xpVal::bitxor type promote failed\n");
        return false;
    }
    dst->u = leftValue->u ^ rightValue->u;
    return true;
}

bool hasOnlyImmediateCharacters(const char *literalText)
{
    if (!literalText)
        return false;
    for (; *literalText; literalText++) {
        const char *allowedCharacterCursor = immediateLiteralCharacters;
        while (*allowedCharacterCursor) {
            if (*allowedCharacterCursor == *literalText)
                break;
            allowedCharacterCursor++;
        }
        if (!*allowedCharacterCursor)
            return false;
    }
    return true;
}

bool mValue::parse(const char *literalText)
{
    if (type == STRING) {
        if (str)
            delete[] str;
        str = 0;
    }
    type = NONE;
    if (!literalText)
        return false;
    int dots = 0;
    int digits = 0;
    int rtype = INT;
    int base = 0;
    int value = 0;
    int radix = 10;
    bool isFlt = false;
    bool hasExp = false;
    int expSign = 0;
    const char *start = literalText;
    if (literalText[0] == '0' && literalText[1]) {
        switch (literalText[1]) {
        case 'x':
        case 'X':
            base = 1;
            radix = 16;
            rtype = parseFloatImmediateMode ? FLOAT : UINT;
            start = literalText + 2;
            break;
        case 'b':
        case 'B':
            radix = 2;
            base = 2;
            start = literalText + 2;
            break;
        case 'z':
        case 'Z':
            base = 1;
            radix = 16;
            rtype = FLOAT;
            start = literalText + 2;
            break;
        }
    }
    int skip = 0;
    if (!*start) {
        printf("badly formed literal %s\n", literalText);
        return false;
    }
    for (const char *p = start; *p; p++) {
        char c = *p;
        if (c >= 'A' && c <= 'Z')
            c += 32;
        if (base == 0) {
            if (c == '.') {
                dots++;
                if (isFlt) {
                    printf("error in floating point expression '%s'\n", literalText);
                    return false;
                }
                if (dots > 1) {
                    printf("unexpected '.' in number\n");
                    return false;
                }
                rtype = FLOAT;
                isFlt = true;
                value = 0;
                digits = 0;
                skip = 1;
            }
            if (c == 'e' && (p[1] == '+' || p[1] == '-')) {
                rtype = FLOAT;
                if (expSign) {
                    printf("already found exponent in %s\n", literalText);
                    return false;
                }
                if (hasExp) {
                    printf("error in float expression %s\n", literalText);
                    return false;
                }
                if (isFlt)
                    hasExp = true;
                else {
                    isFlt = true;
                    hasExp = true;
                }
                value = 0;
                digits = 0;
                expSign = (p[1] == '+') ? 1 : -1;
                skip = 2;
            }
        }
        if (c == '_')
            skip = 1;
        if (!skip) {
            const char *d = immediateLiteralDigitsByBase[base];
            int idx;
            char ch;
            for (idx = 0; (ch = *d) != 0; d++, idx++)
                if (c == ch)
                    break;
            if (!ch)
                return false;
            value = value * radix + idx;
        }
        if (skip)
            skip--;
        else
            digits++;
    }
    if (skip || !digits) {
        reportError("error parsing literal %s\n", literalText);
        return false;
    }
    if (isFlt)
        f = strtod(literalText, 0);
    else
        i = value;
    type = rtype;
    return true;
}

void mValue::dump()
{
}

char *mValue::getString()
{
    if (type == STRING)
        return str;
    return 0;
}

bool mValue::makeNumeric()
{
    if (type == STRING) {
        mValue parsedValue(NONE);
        parsedValue.parse(str);
        if (isNumType(parsedValue.type))
            set(&parsedValue);
    }
    return isNumType(type);
}

char immediateFormattingBuffer[4096];
int immediateValueStringOffset;

char *mValue::asChars()
{
    if (immediateValueStringOffset > 2048)
        immediateValueStringOffset = 0;
    char *outputCursor = immediateFormattingBuffer + immediateValueStringOffset;
    switch (type) {
    case INT:
        formatIntoBuffer(outputCursor, "%d", i);
        break;
    case UINT:
        formatIntoBuffer(outputCursor, "%u", u);
        break;
    case FLOAT:
        formatIntoBuffer(outputCursor, "%1.9e", f);
        break;
    case STRING:
        if (!str) {
            reportError("asChars - but no str for a literal\n");
            return 0;
        }
        formatIntoBuffer(outputCursor, "%s", str);
        break;
    }
    immediateValueStringOffset += mstrlen(outputCursor) + 1;
    return outputCursor;
}

bool sameImmediateValue(mValue *leftValue, mValue *rightValue)
{
    unsigned int leftDefaultWord = 0;
    unsigned int rightDefaultWord = 0;
    unsigned int *leftWordPointer = &leftDefaultWord;
    unsigned int *rightWordPointer = &rightDefaultWord;
    if (leftValue && rightValue) {
        if (leftValue->type != rightValue->type)
            return false;
        if (leftValue->type == mValue::STRING)
            return asciiCaseEqual(leftValue->str, rightValue->str);
    }
    if (leftValue && leftValue->type == mValue::STRING)
        return false;
    if (rightValue && rightValue->type == mValue::STRING)
        return false;
    if (leftValue && isNumType(leftValue->type))
        leftWordPointer = &leftValue->u;
    if (rightValue && isNumType(rightValue->type))
        rightWordPointer = &rightValue->u;
    return *leftWordPointer == *rightWordPointer;
}
