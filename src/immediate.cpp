#include "Instr.h"
#include "xpNode.h"
#include "xpTree_inline.h"
#include "os.h"

const char *immediateValueTypeName(int type);
extern bool parseFloatImmediateMode;

extern const char parseImmediateError[];
extern const char evalImmediateError[];
extern const char parseImmediateWarn[];
extern const char maddImmediateInputError[];

const char *immediateTypeName(unsigned char immediateFlags)
{
    return immediateValueTypeName(decodeImmediateCategory(immediateFlags));
}

bool parseImmediate(const char *immediateText, unsigned char type, mValue *expressionValue)
{
    expressionValue->clear();
    parseFloatImmediateMode = (type == 0x40);
    int immediateCategory = decodeImmediateCategory(type);
    if (immediateCategory == 4) {
        parseFloatImmediateMode = false;
        return expressionValue->setString(immediateText);
    }
    xpTree tree;
    if (!tree.evalExp(immediateText)) {
        reportError(parseImmediateError, immediateText);
    } else {
        int unknownSymbolCount = tree.countUnknownExpressionSymbols();
        if (unknownSymbolCount < 0) {
            reportWarning(parseImmediateWarn, immediateText);
        } else {
            if (unknownSymbolCount != 0) {
                if (!expressionValue->setString(immediateText)) {
                    reportError("failed to set literal\n");
                    return false;
                }
            } else {
                expressionValue->dump();
                if (!tree.evaluate(expressionValue, immediateCategory)) {
                    reportError(evalImmediateError, immediateText, immediateValueTypeName(immediateCategory));
                    goto fail;
                }
            }
            parseFloatImmediateMode = false;
            return true;
        }
    }
fail:
    parseFloatImmediateMode = false;
    return false;
}

int maddImmediate(Operand *const operand, mValue *expressionValue, int immediateOffset)
{
    if (!operand->immediateValue.isValid()) {
        reportError("madd immediate- ximm invalid\n");
        return -1;
    }
    if (!expressionValue->isValid()) {
        reportError(maddImmediateInputError);
        return 0;
    }
    int arithmeticResult = 0;
    if (expressionValue->isInt() && operand->immediateValue.isInt()) {
        int operandValue = 0;
        int sourceValue = 0;
        if (!operand->immediateValue.getInt(&operandValue) || !expressionValue->getInt(&sourceValue)) {
            reportError("failed to get integer values\n");
            return -1;
        }
        operand->immediateValue.setInt(operandValue + sourceValue * immediateOffset);
    } else {
        char *operandImmediateText = operand->immediateValue.asChars();
        char *sourceImmediateText = expressionValue->asChars();
        if (!operandImmediateText || !sourceImmediateText) {
            reportError("failed to get chars\n");
            return -1;
        }
        char buf[1024];
        formatIntoBuffer(buf, "(%s)+((%s)*%d)", operandImmediateText, sourceImmediateText, immediateOffset);
        operand->immediateValue.setString(buf);
        arithmeticResult = 2;
    }
    return arithmeticResult;
}
