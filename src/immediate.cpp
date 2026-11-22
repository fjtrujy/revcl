#include "Instr.h"
#include "xpNode.h"
#include "xpTree_inline.h"
#include "os.h"

const char *func_0804ca58(int type);
extern bool g_080b3088;

extern const char parseImmediateError[];
extern const char evalImmediateError[];
extern const char parseImmediateWarn[];
extern const char maddImmediateInputError[];

const char *func_0806a33c(unsigned char t)
{
    return func_0804ca58(func_08070300(t));
}

bool parseImmediate(const char *s, unsigned char type, mValue *v)
{
    v->clear();
    g_080b3088 = (type == 0x40);
    int t = func_08070300(type);
    if (t == 4) {
        g_080b3088 = false;
        return v->setString(s);
    }
    xpTree tree;
    if (!tree.evalExp(s)) {
        reportError(parseImmediateError, s);
    } else {
        int n = tree.func_0804f8f0();
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
                    reportError(evalImmediateError, s, func_0804ca58(t));
                    goto fail;
                }
            }
            g_080b3088 = false;
            return true;
        }
    }
fail:
    g_080b3088 = false;
    return false;
}

int maddImmediate(Operand *const op, mValue *v, int n)
{
    if (!op->unk_1c.isValid()) {
        reportError("madd immediate- ximm invalid\n");
        return -1;
    }
    if (!v->isValid()) {
        reportError(maddImmediateInputError);
        return 0;
    }
    int r = 0;
    if (v->isInt() && op->unk_1c.isInt()) {
        int a = 0;
        int b = 0;
        if (!op->unk_1c.getInt(&a) || !v->getInt(&b)) {
            reportError("failed to get integer values\n");
            return -1;
        }
        op->unk_1c.setInt(a + b * n);
    } else {
        char *x = op->unk_1c.asChars();
        char *y = v->asChars();
        if (!x || !y) {
            reportError("failed to get chars\n");
            return -1;
        }
        char buf[1024];
        formatIntoBuffer(buf, "(%s)+((%s)*%d)", x, y, n);
        op->unk_1c.setString(buf);
        r = 2;
    }
    return r;
}
