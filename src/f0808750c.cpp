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
            in->func_08054008(false);
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
            in->func_08054008(false);
            return 0;
        }
    }
    return p;
}

RegPoolEntry *getReg(unsigned type, const char *name);

int extHook(void *, Instr *in)
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

int func_08087860(void *, Instr *in)
{
    Operand *op = &in->unk_38[1];
    op->unk_07 = (((op->unk_07 & 8) >> 3) | (op->unk_07 << 1)) & 0xf;
    return 1;
}

int efu_Hook(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    Operand *op = &in->unk_38[1];
    switch (info->unk_1c) {
    case 1: break;
    case 2: op->unk_07 = 3; break;
    case 3: op->unk_07 = 5; break;
    case 4: op->unk_07 = 7; break;
    case 5: op->unk_07 = 15; break;
    default:
        func_0804ad38("unknown EFU mod bits type during efu_Hook at build\n");
        return 0;
    }
    return 1;
}

bool func_0804bfcc(const char *a, const char *b);

int wr2w_Hook(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    iInfo *n;
    if (func_0804bfcc(info->unk_04, "ilwr"))
        n = func_0806bf8c("ilw", false);
    else if (func_0804bfcc(info->unk_04, "iswr"))
        n = func_0806bf8c("isw", false);
    else {
        func_0804ad38("wr2w_Hook - couldn't find instruction '%s'\n", info->unk_04);
        return 0;
    }
    if (!n) {
        func_0804ad38("wr2w_Hook - failed to find extended instruction\n");
        return 0;
    }
    in->unk_34 = n;
    in->unk_38[1].unk_04 = 0x1f;
    in->unk_38[1].unk_1c.setInt(0);
    return 1;
}

static inline void dumpEmitInstruction(Instr *in)
{
    in->func_08054008(false);
}

char *emitClip(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    bool comma = false;
    int len = func_0804adbc(buf, "clipw%s.xyz", mods);
    char *p = buf + len;
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

char *vmove_Emit(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    char *p;
    if (!in)
        return 0;
    if (!in->unk_142c) {
        func_0804ad38("baseMuid not set in vmove_Emit\n");
        return 0;
    }
    if (!in->unk_142e) {
        func_0804ad38("trying to emit an abstract move\n");
        return 0;
    }
    p = buf;
    switch (in->unk_142e) {
    case 0xb3:
        p += func_0804adbc(buf, "iaddiu");
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = in->unk_38[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->unk_38[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        *p++ = '0';
        break;
    case 0xb5:
        p += func_0804adbc(buf, "%s%s", "move", mods);
        p = emitDestMask(p, 1, in->unk_138f);
        if (!p) {
            func_0804ad38("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = in->unk_38[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->unk_38[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        break;
    case 0xb6:
        p += func_0804adbc(buf, "%s%s", "max", mods);
        p = emitDestMask(p, 1, in->unk_138f);
        if (!p) {
            func_0804ad38("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = in->unk_38[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->unk_38[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->unk_38[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        break;
    default:
        func_0804ad38("unknown vmove output requested\n");
        return 0;
    }
    *p = 0;
    return p;
fail:
    func_0804ad38("failed to emit operand 0\n");
    return 0;
}

int semSynth(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    char name[32];
    char *d = name;
    const char *s = info->unk_04;
    while (*s)
        *d++ = *s++;
    if (in->unk_38[0].unk_04 == 9) {
        if (d[-1] != 'a' && d[-1] != 'A') {
            *d++ = 'a';
            in->unk_38[0].unk_0a = 1;
        }
    } else
        in->unk_38[0].unk_0a = 4;
    char k = in->unk_38[2].unk_04;
    if (k == 4) {
        in->unk_38[2].unk_0b = 1;
        *d++ = 'i';
    }
    if (k == 7) {
        *d++ = 'q';
        in->unk_38[2].unk_0b = 0;
    }
    if (k == 3) {
        unsigned char m = in->unk_38[2].unk_07;
        int n = 0;
        if (m == 1) { *d++ = 'x'; n++; }
        if (m == 2) { *d++ = 'y'; n++; }
        if (m == 4) { *d++ = 'z'; n++; }
        if (m == 8) { *d++ = 'w'; n++; }
        if (n != 1) {
            func_0804ad38("semSynth - bad FL1 bits\n");
            return 0;
        }
    }
    *d = 0;
    iInfo *ni = func_0806bf8c(name, false);
    if (!ni) {
        func_0804ad38("failed to find derived instruction type '%s'\n", name);
        return 0;
    }
    in->unk_34 = ni;
    return 1;
}

RegDesc regDescTable[] = {
#include "regDescTable.h"
};

iInfo vuITable_080b3b20[] = {
#include "vuITable.h"
};

PoolDef vuPools_080bbae0[] = {
    {1, "vf", "VU User floating point", 9, 4, "vf00", 0},
    {1, "vi", "VU User integer", 8, 1, "vi00", 0},
    {2, "hw", "VU hardware register", 10, 0, 0, "EXT"},
    {0, 0, 0, 0, 0, 0, 0},
};

extern const char *pipeNames[16];

CpuDef cpu_080bbb50 = {
    "Marigold",
    2,
    vuPools_080bbae0,
    pipeNames,
    vuITable_080b3b20,
    opPairTable_080b32e0,
    opCountTable_080b3340,
};
