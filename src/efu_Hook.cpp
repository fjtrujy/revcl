#include "os.h"
#include "instStream.h"
#include "decodedLine.h"
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

bool func_08087860(void *, Instr *in)
{
    Operand *op = &in->operands[1];
    op->unk_07 = (((op->unk_07 & 8) >> 3) | (op->unk_07 << 1)) & 0xf;
    return 1;
}

bool efu_Hook(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    Operand *op = &in->operands[1];
    switch (info->efuMaskType) {
    case 1: break;
    case 2: op->unk_07 = 3; break;
    case 3: op->unk_07 = 5; break;
    case 4: op->unk_07 = 7; break;
    case 5: op->unk_07 = 15; break;
    default:
        reportError("unknown EFU mod bits type during efu_Hook at build\n");
        return 0;
    }
    return 1;
}

bool asciiCaseEqual(const char *a, const char *b);

bool wr2w_Hook(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    iInfo *n;
    if (asciiCaseEqual(info->mnemonic, "ilwr"))
        n = func_0806bf8c("ilw", false);
    else if (asciiCaseEqual(info->mnemonic, "iswr"))
        n = func_0806bf8c("isw", false);
    else {
        reportError("wr2w_Hook - couldn't find instruction '%s'\n", info->mnemonic);
        return 0;
    }
    if (!n) {
        reportError("wr2w_Hook - failed to find extended instruction\n");
        return 0;
    }
    in->opcodeInfo = n;
    in->operands[1].unk_04 = 0x1f;
    in->operands[1].unk_1c.setInt(0);
    return 1;
}

char *emitClip(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    bool comma = false;
    int len = formatIntoBuffer(buf, "clipw%s.xyz", mods);
    char *p = buf + len;
    for (int n = p - buf; n < 14; n++)
        *p++ = ' ';
    for (int i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        if (op->unk_08)
            continue;
        if (comma)
            *p++ = ',';
        comma = true;
        p = op->Emit(p, upper, b);
        if (!p) {
            reportError("failed to emit operand\n");
            op->func_0805665c(0);
            reportError("from\n");
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
        reportError("baseMuid not set in vmove_Emit\n");
        return 0;
    }
    if (!in->unk_142e) {
        reportError("trying to emit an abstract move\n");
        return 0;
    }
    p = buf;
    switch (in->unk_142e) {
    case 0xb3:
        p += formatIntoBuffer(buf, "iaddiu");
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = in->operands[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        *p++ = '0';
        break;
    case 0xb5:
        p += formatIntoBuffer(buf, "%s%s", "move", mods);
        p = emitDestMask(p, 1, in->unk_138f);
        if (!p) {
            reportError("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = in->operands[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        break;
    case 0xb6:
        p += formatIntoBuffer(buf, "%s%s", "max", mods);
        p = emitDestMask(p, 1, in->unk_138f);
        if (!p) {
            reportError("failed to emit inst mods for lvfmove\n");
            return 0;
        }
        for (int n = p - buf; n < 14; n++)
            *p++ = ' ';
        p = in->operands[0].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        *p++ = ',';
        p = in->operands[1].Emit(p, upper, b);
        if (!p)
            goto fail;
        break;
    default:
        reportError("unknown vmove output requested\n");
        return 0;
    }
    *p = 0;
    goto done;
fail:
    reportError("failed to emit operand 0\n");
    return 0;
done:
    return p;
}

bool semSynth(void *p, Instr *in)
{
    iInfo *info = (iInfo *)p;
    char name[32];
    char *d = name;
    const char *s = info->mnemonic;
    while (*s)
        *d++ = *s++;
    if (in->operands[0].unk_04 == 9) {
        if (d[-1] != 'a' && d[-1] != 'A') {
            *d++ = 'a';
            in->operands[0].unk_0a = 1;
        }
    } else
        in->operands[0].unk_0a = 4;
    char k = in->operands[2].unk_04;
    if (k == 4) {
        in->operands[2].unk_0b = 1;
        *d++ = 'i';
    }
    if (k == 7) {
        *d++ = 'q';
        in->operands[2].unk_0b = 0;
    }
    if (k == 3) {
        unsigned char m = in->operands[2].unk_07;
        int n = 0;
        if (m == 1) { *d++ = 'x'; n++; }
        if (m == 2) { *d++ = 'y'; n++; }
        if (m == 4) { *d++ = 'z'; n++; }
        if (m == 8) { *d++ = 'w'; n++; }
        if (n != 1) {
            reportError("semSynth - bad FL1 bits\n");
            return 0;
        }
    }
    *d = 0;
    iInfo *ni = func_0806bf8c(name, false);
    if (!ni) {
        reportError("failed to find derived instruction type '%s'\n", name);
        return 0;
    }
    in->opcodeInfo = ni;
    return 1;
}
