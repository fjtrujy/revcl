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

char *func_0808750c(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    iInfo *info = in->opcodeInfo;
    bool comma = false;
    if (!mods)
        mods = "";
    int len = formatIntoBuffer(buf, "%s%s", info->mnemonic, mods);
    char *p = buf + len;
    p = emitDestMask(p, (char)info->destinationMaskMode, in->destinationMask);
    if (!p) {
        reportError("failed to emit inst mods\n");
        return 0;
    }
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
            op->diagnoseOperand(0);
            reportError("from\n");
            dumpEmitInstruction(in);
            return 0;
        }
    }
    return p;
}

char *emitReversibleBranch(char *buf, Instr *in, bool upper, char *mods, bool b, int)
{
    iInfo *info = in->opcodeInfo;
    bool comma = false;
    if (!mods)
        mods = "";
    if (in->unk_1391) {
        if ((in->branchFlags >> 4) & 1) {
            reportError("can't reverse sense of a likely branch at the emit stage\n");
            return 0;
        }
        info = findOpcodeInfoByMnemonic(info->reverseSenseMnemonic, false);
        if (!info) {
            reportError("failed to find sense reversing instruction\n");
            return 0;
        }
    }
    int len = formatIntoBuffer(buf, "%s%s", info->mnemonic, mods);
    char *p = buf + len;
    p = emitDestMask(p, (char)info->destinationMaskMode, in->destinationMask);
    if (!p) {
        reportError("failed to generate instruction mod string\n");
        return 0;
    }
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
            op->diagnoseOperand(0);
            reportError("from\n");
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
    for (int i = 0; i != in->operandCount; i++) {
        Operand *op = &in->operands[i];
        if (op->unk_04 != 55)
            continue;
        if (op->unk_18) {
            reportError("extHook - register already set\n");
            return 0;
        }
        RegPoolEntry *r = getReg((unsigned char)op->unk_05, "EXT");
        if (!r) {
            reportError("couldn't get ext register for pool\n");
            return 0;
        }
        op->unk_18 = r;
        op->unk_07 = (1 << r->partCount) - 1;
        if (!op->unk_07) {
            reportError("no mod bits set for ext\n");
            return 0;
        }
        done++;
    }
    if (!done) {
        reportError("extHook - didn't do anything\n");
        return 0;
    }
    return 1;
}

