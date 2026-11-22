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

bool parseImmediate(const char *s, unsigned char type, mValue *v)
{
    v->clear();
    parseFloatImmediateMode = (type == 0x40);
    int t = decodeImmediateCategory(type);
    if (t == 4) {
        parseFloatImmediateMode = false;
        return v->setString(s);
    }
    xpTree tree;
    if (!tree.evalExp(s)) {
        reportError(parseImmediateError, s);
    } else {
        int n = tree.countUnknownExpressionSymbols();
        if (n < 0) {
            reportWarning(parseImmediateWarn, s);
        } else {
            if (n != 0) {
                if (!v->setString(s)) {
                    reportError("failed to set literal\n");
                    return false;
                }
            } else {
                v->dump();
                if (!tree.evaluate(v, t)) {
                    reportError(evalImmediateError, s, immediateValueTypeName(t));
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

int maddImmediate(Operand *const op, mValue *v, int n)
{
    if (!op->immediateValue.isValid()) {
        reportError("madd immediate- ximm invalid\n");
        return -1;
    }
    if (!v->isValid()) {
        reportError(maddImmediateInputError);
        return 0;
    }
    int r = 0;
    if (v->isInt() && op->immediateValue.isInt()) {
        int a = 0;
        int b = 0;
        if (!op->immediateValue.getInt(&a) || !v->getInt(&b)) {
            reportError("failed to get integer values\n");
            return -1;
        }
        op->immediateValue.setInt(a + b * n);
    } else {
        char *x = op->immediateValue.asChars();
        char *y = v->asChars();
        if (!x || !y) {
            reportError("failed to get chars\n");
            return -1;
        }
        char buf[1024];
        formatIntoBuffer(buf, "(%s)+((%s)*%d)", x, y, n);
        op->immediateValue.setString(buf);
        r = 2;
    }
    return r;
}
