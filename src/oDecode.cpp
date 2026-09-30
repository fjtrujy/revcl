// Instruction line decoding ("oDecode", CIE group 38).
#include "os.h"
#include "Parser.h"
#include "mList.h"
#include "Processor.h"
#include "VUStr.h"
#include "Instr.h"
#include "workTable.h"

char gatheredToken[1024];

static inline void copyToken(char *d, const char *s)
{
    int i;
    for (i = 0; s[i] && i < 0x800; i++)
        d[i] = s[i];
    d[i] = 0;
}

bool Processor::decodeLE(LocTag *t, int *pos, mList *labels, Instr **out, int *nOut)
{
    char buf[0x800];

    if (!t) {
        func_0804ad38("decodeLE: passed a null pointer\n");
        return false;
    }
    if (*pos == 0) {
        char *tok = t->func_08050be4(0, 0);
        if (func_080506e0(tok) && !t->func_08050f18(0)) {
            if (*nOut != 0) {
                func_0804ad84("found a label on a continued line\n");
                return false;
            }
            copyToken(buf, tok);
            int mods = func_080521f0(buf);
            char *p = buf;
            while (*p && *p != ':')
                p++;
            *p = 0;
            mNode *n = labels->func_0804b444(buf);
            if (!n) {
                func_0804ad38("Processor::decodeLE() - couldn't create tail for label list\n");
                return false;
            }
            n->unk_18 = mods;
            (*pos)++;
            if (func_0805085c(t) > 1) {
                char *tok2 = t->func_08050be4(1, 0);
                if (*tok2 == '[') {
                    copyToken(buf + 1, tok2);
                    n->unk_18 |= func_080521f0(buf);
                    (*pos)++;
                }
            }
            if (!func_080522c4(n->unk_18, unk_3d, true)) {
                func_0804ad84("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(unk_3d));
                t->func_08050c40(-1);
                return false;
            }
        }
    }
    if (t->func_08050f48())
        (*pos)++;
    while (*pos < func_0805085c(t)) {
        if (t->func_08050f18(*pos)) {
            (*pos)++;
            continue;
        }
        if (t->func_08050f7c(*pos)) {
            (*pos)++;
            return true;
        }
        if (*nOut > 1) {
            func_0804ad84("too many instructions on a line!\n");
            t->func_08050c40(-1);
            return false;
        }
        Instr *ins = getNextInstruction(t, pos);
        if (!ins)
            return false;
        out[(*nOut)++] = ins;
    }
    return true;
}

int gatherTokens(LocTag *t, int i, bool stopAtOpcode)
{
    char *d = gatheredToken;
    int n = 0;
    char sep;

    for (; i < func_0805085c(t); i++) {
        char *tok = t->func_08050be4(i, &sep);
        if (!tok) {
            func_0804ad38("gatherTokens() - failed to get token\n");
            return -1;
        }
        if (!t->func_08050f18(i)) {
            if (t->func_08050f7c(i)) {
                *d = 0;
                return i;
            }
            if (n && stopAtOpcode) {
                for (char *p = tok; *p; p++)
                    if (p[0] == '.' && p[1] == '.') {
                        *d = 0;
                        return i;
                    }
            }
            if (n && stopAtOpcode && func_0806bf8c(tok, false)) {
                *d = 0;
                return i;
            }
            n++;
            while (*tok) {
                if ((unsigned)(d - gatheredToken) > 0x3fe) {
                    func_0804ad84("gatherTokens: the gathered token became too long!\n");
                    return -1;
                }
                *d++ = *tok++;
                *d = 0;
            }
        }
        if (sep)
            break;
    }
    *d = 0;
    return i + 1;
}
