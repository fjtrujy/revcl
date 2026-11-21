#include "instStream.h"
#include "flowBlock.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);

static inline void dumpEmitInstruction(Instr *in)
{
    in->func_08054008(false);
}

char *func_0808750c(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    iInfo *info = in->unk_34;
    bool comma = false;
    if (!mods)
        mods = "";
    int len = func_0804adbc(buf, "%s%s", info->unk_04, mods);
    char *p = buf + len;
    p = emitDestMask(p, (char)info->unk_0c, in->unk_138f);
    if (!p) {
        func_0804ad38("failed to emit inst mods\n");
        return 0;
    }
    for (int n = p - buf; n < 14; n++)
        *p++ = ' ';
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        if (op->unk_08)
            continue;
        if (comma)
            *p++ = ',';
        comma = true;
        p = op->Emit(p, upper, b);
        if (!p) {
            func_0804ad38("failed to emit operand\n");
            op->func_0805665c(0);
            func_0804ad38("from\n");
            dumpEmitInstruction(in);
            return 0;
        }
    }
    return p;
}

char *emitReversibleBranch(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    iInfo *info = in->unk_34;
    bool comma = false;
    if (!mods)
        mods = "";
    if (in->unk_1391) {
        if ((in->unk_1388 >> 4) & 1) {
            func_0804ad38("can't reverse sense of a likely branch at the emit stage\n");
            return 0;
        }
        info = func_0806bf8c(info->unk_34, false);
        if (!info) {
            func_0804ad38("failed to find sense reversing instruction\n");
            return 0;
        }
    }
    int len = func_0804adbc(buf, "%s%s", info->unk_04, mods);
    char *p = buf + len;
    p = emitDestMask(p, (char)info->unk_0c, in->unk_138f);
    if (!p) {
        func_0804ad38("failed to generate instruction mod string\n");
        return 0;
    }
    for (int n = p - buf; n < 14; n++)
        *p++ = ' ';
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        if (op->unk_08)
            continue;
        if (comma)
            *p++ = ',';
        comma = true;
        p = op->Emit(p, upper, b);
        if (!p) {
            func_0804ad38("failed to emit operand\n");
            op->func_0805665c(0);
            func_0804ad38("from\n");
            dumpEmitInstruction(in);
            return 0;
        }
    }
    return p;
}

RegPoolEntry *getReg(unsigned type, const char *name);

bool extHook(void *, Instr *in)
{
    int done = 0;
    for (int i = 0; i != in->unk_138b; i++) {
        Operand *op = &in->unk_38[i];
        if (op->unk_04 != 55)
            continue;
        if (op->unk_18) {
            func_0804ad38("extHook - register already set\n");
            return 0;
        }
        RegPoolEntry *r = getReg((unsigned char)op->unk_05, "EXT");
        if (!r) {
            func_0804ad38("couldn't get ext register for pool\n");
            return 0;
        }
        op->unk_18 = r;
        op->unk_07 = (1 << r->unk_28) - 1;
        if (!op->unk_07) {
            func_0804ad38("no mod bits set for ext\n");
            return 0;
        }
        done++;
    }
    if (!done) {
        func_0804ad38("extHook - didn't do anything\n");
        return 0;
    }
    return 1;
}

