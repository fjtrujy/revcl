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

const char *immediateTypeName(unsigned char t)
{
    return immediateValueTypeName(decodeImmediateCategory(t));
}

bool parseImmediate(const char *immediateText, unsigned char type, mValue *expressionValue)
{
    expressionValue->clear();
    parseFloatImmediateMode = (type == 0x40);
    int t = decodeImmediateCategory(type);
    if (t == 4) {
        parseFloatImmediateMode = false;
        return expressionValue->setString(immediateText);
    }
    xpTree tree;
    if (!tree.evalExp(immediateText)) {
        reportError(parseImmediateError, immediateText);
    } else {
        int n = tree.countUnknownExpressionSymbols();
        if (n < 0) {
            reportWarning(parseImmediateWarn, immediateText);
        } else {
            if (n != 0) {
                if (!expressionValue->setString(immediateText)) {
                    reportError("failed to set literal\n");
                    return false;
                }
            } else {
                expressionValue->dump();
                if (!tree.evaluate(expressionValue, t)) {
                    reportError(evalImmediateError, immediateText, immediateValueTypeName(t));
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

int maddImmediate(Operand *const op, mValue *expressionValue, int n)
{
    if (!op->immediateValue.isValid()) {
        reportError("madd immediate- ximm invalid\n");
        return -1;
    }
    if (!expressionValue->isValid()) {
        reportError(maddImmediateInputError);
        return 0;
    }
    int r = 0;
    if (expressionValue->isInt() && op->immediateValue.isInt()) {
        int a = 0;
        int b = 0;
        if (!op->immediateValue.getInt(&a) || !expressionValue->getInt(&b)) {
            reportError("failed to get integer values\n");
            return -1;
        }
        op->immediateValue.setInt(a + b * n);
    } else {
        char *operandImmediateText = op->immediateValue.asChars();
        char *sourceImmediateText = expressionValue->asChars();
        if (!operandImmediateText || !sourceImmediateText) {
            reportError("failed to get chars\n");
            return -1;
        }
        char buf[1024];
        formatIntoBuffer(buf, "(%s)+((%s)*%d)", operandImmediateText, sourceImmediateText, n);
        op->immediateValue.setString(buf);
        r = 2;
    }
    return r;
}
