#include "VUStr.h"

// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include <string.h>
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

char *dupString(const char *s);

extern int errorOccurred;
extern bool quiet_080bbd98;

opPair_s opPairTable_080b32e0[] = {
    {109, {1, 1, 0, 2}, 0},
    {110, {1, 1, 0, 2}, 0},
    {112, {2, 1, 0, 2}, 0},
    {158, {1, 2, 1, -1}, 1},
    {163, {1, 2, 1, -1}, 1},
    {159, {2, 2, 1, -1}, 1},
    {164, {2, 2, 1, -1}, 1},
    {0, {0}, 0},
};

opCount_s opCountTable_080b3340[] = {
    {155, 1}, {160, 1}, {156, 1}, {161, 1}, {0, 0},
};

Instr::Instr(iInfo *info)
{
    if (!info) {
        reportError("constructor called with null iInfo -... later will make this a NOP, but , for now..halt!\n");
        exitAfterCleanup(-1);
    }
    operandCount = 0;
    unk_13d1 = 0;
    unk_1389 = 0;
    unk_138f = 0;
    unk_1434 = 0;
    unk_14 = 0;
    unk_18 = 0;
    unk_1a = 0;
    unk_1430 = 0;
    unk_1432 = 0;
    unk_142c = 0;
    unk_142e = 0;
    unk_1420 = -1;
    unk_139c = 0;
    unk_1398 = 0;
    unk_30 = 0;
    unk_1428 = 0;
    unk_13b0 = 0;
    unk_13b2 = 0;
    unk_13b4 = 0;
    unk_13b6 = 0;
    unk_13e4 = -1;
    unk_13e0 = -1;
    unk_1418 = 0;
    unk_141a = 0;
    opcodeInfo = info;
    unk_141e = 0;
    unk_08 = 0;
    unk_00 = 0;
    unk_04 = 0;
    unk_1391 = 0;
    unk_1392 = 0;
    unk_1394 = -1;
    unk_141c = 0;
    unk_13d2 = -1;
    unk_138e = -1;
    resetImmFixup();
    opcodeInfo = info;
    unk_13d7 = info->unk_0e;
    unk_138c = info->unk_0e;
    unk_138d = info->unk_10;
    unk_1388 = info->branchFlags();
    unk_13cf = info->unk_12;
    unk_13ce = info->ramType();
    unk_13d0 = info->kindAndFlags & ~7;
    unk_138a = info->isRawOnly();
    unk_13cd = info->syntheticKind();
    for (int i = 0; info->operandDescriptors[i].operandKind; i++) {
        operandCount++;
        Operand *op = &operands[i];
        iOpDesc *d = &info->operandDescriptors[i];
        op->unk_04 = d->operandKind;
        op->unk_26 = i;
        op->unk_0b = d->unk_04 != 0;
        op->unk_09 = d->direction;
        op->unk_08 = d->isImplicit != 0;
        op->unk_05 = d->unk_05;
        op->unk_0d = d->patchedOutputIndexPlusOne;
        if (op->unk_0d)
            operands[op->unk_0d - 1].unk_0d = i + 1;
        if (d->direction == 2)
            op->unk_0a = d->outputLatency;
        else
            op->unk_0a = 0;
    }
}

Instr::~Instr()
{
    if (unk_139c) {
        delete[] unk_139c;
        unk_139c = 0;
    }
    mValue *value = &unk_13d8;
    value->clear();
    unk_13a0.clear();
    unk_13a8.clear();
}

bool Instr::func_08053f4c()
{
    if (unk_139c) {
        delete[] unk_139c;
        unk_139c = 0;
    }
    unk_13d8.clear();
    unk_13a0.clear();
    unk_13a8.clear();
    return true;
}

bool Instr::func_08053fac()
{
    if (!opcodeInfo)
        return false;
    for (int i = 0; i != operandCount; i++)
        if (!operands[i].func_080563f8())
            return false;
    return true;
}

void Instr::func_08054008(int flags)
{
    if (isValid()) {
        int count = operandCount;
        for (int i = 0; i != count; i++) {
            operands[i].func_0805665c(0);
            count = operandCount;
        }
        if (flags & 1) {
            for (int j = 0; j != operandCount; j++) {
                Operand *op = &operands[j];
                RegPoolEntry *r = op->unk_18;
                if (r && r->isVirtual)
                    for (int k = 0; k != r->partCount; k++)
                        if ((op->unk_07 >> k) & 1)
                            op->unk_2c[k].unk_54.validateOperandLocations();
            }
        }
    }
}

bool Instr::func_08054138()
{
    if (!opcodeInfo)
        return true;
    return opcodeInfo->isNop();
}

instStream *Instr::func_08054168()
{
    return unk_30 ? unk_30->unk_10 : 0;
}

bool Instr::func_0805417c()
{
    unk_13a0.clear();
    unk_13a8.clear();
    unk_1394 = -1;
    for (int i = 0; i != operandCount; i++)
        if (!operands[i].func_080569cc())
            return false;
    unk_1434 = 0;
    return true;
}

bool Instr::func_080541fc()
{
    return !unk_13cd;
}

bool Instr::func_08054214()
{
    unk_1418 = 0;
    unk_141c = 0;
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        for (int k = 0; k != 4; k++)
            op->unk_2c[k].unk_50 = 0;
    }
    return true;
}

void Instr::leDump()
{
    leDump(-1, -1);
}

void Instr::leDump(int opnd)
{
    leDump(opnd, -1);
}

void Instr::leDump(int opnd, int mask)
{
    if (opnd >= operandCount) {
        reportError("Instruction::leDump - out of bounds operand!\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quiet_080bbd98;
    if (unk_14) {
        int idx = unk_18;
        if (opnd >= 0) {
            if (operands[opnd].unk_18 && operands[opnd].unk_18->partCount != 1 && mask >= 0)
                reportMessage("(field(s) : %s) ", func_08052064(mask));
            if (operands[opnd].unk_08) {
                if (operands[opnd].unk_18)
                    reportMessage("implicit operand '%s'\n", operands[opnd].unk_18->registerName());
                else
                    reportError("implicit operand with no register??\n");
            } else
                idx = operands[opnd].unk_24;
        }
        unk_14->dumpSourceLine(idx);
    } else {
        reportMessage("operand %d modbits %x..\n", opnd, mask);
        func_08054008(false);
    }
    errorOccurred = saved;
}

bool func_080543bc(Instr *a, Instr *b)
{
    if (!a || !b)
        return false;
    for (int i = 0; i != b->operandCount; i++) {
        Operand *op = &b->operands[i];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            int n = r->partCount;
            for (int k = 0; k != n; k++)
                if (((op->unk_07 & ~op->unk_27) >> k) & 1)
                    for (ol_s *p = op->unk_2c[k].unk_54.head; p; p = p->unk_00)
                        if (p->unk_04 == a)
                            return true;
        }
    }
    return false;
}

bool func_0805449c(Instr *a, Instr *b)
{
    if (!a || !b)
        return false;
    for (int i = 0; i != b->operandCount; i++) {
        Operand *op = &b->operands[i];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            int want = op->unk_07;
            int mask = 0;
            Instr *in = a;
            for (int j = 0; j != in->operandCount; j++)
                if (registerOperand(&in->operands[j], 2) == r)
                    mask |= in->operands[j].unk_07;
            if (mask & want)
                return true;
        }
    }
    return false;
}



Instr *Instr::clone()
{
    if (!this)
        return 0;
    iInfo *info = opcodeInfo;
    if (!info)
        return 0;
    Instr *c = new Instr(info);
    if (!c) {
        reportError("failed to get a new instruction during clone!\n");
        exitAfterCleanup(-1);
    }
    if (!c->func_08054b84()) {
        reportError("failed to opt reset cloned instruction\n");
        exitAfterCleanup(-1);
    }
    c->unk_138f = unk_138f;
    c->unk_138a = unk_138a;
    c->unk_1428 = 0;
    c->unk_1389 = unk_1389;
    c->unk_142c = unk_142c;
    c->unk_142e = unk_142e;
    c->unk_13d7 = unk_13d7;
    c->unk_13f0.func_0805e37c(&unk_13f0);
    c->unk_13e4 = unk_13e4;
    c->unk_13e0 = unk_13e0;
    c->unk_141a = unk_141a;
    c->unk_1420 = unk_1420;
    c->unk_1392 = unk_1392;
    c->unk_1391 = unk_1391;
    c->unk_14 = unk_14;
    c->unk_18 = unk_18;
    c->unk_1a = unk_1a;
    c->unk_13d4 = unk_13d4;
    c->unk_13d5 = unk_13d5;
    c->unk_13d6 = unk_13d6;
    if (unk_13d8.isValid())
        c->unk_13d8.set(&unk_13d8);
    c->unk_13d2 = unk_13d2;
    if (unk_08) {
        reportError("during instruction clone - tmpClone was already set!\n");
        delete c;
        return 0;
    }
    unk_08 = c;
    c->unk_08 = this;
    if (unk_139c)
        c->unk_139c = dupString(unk_139c);
    c->operandCount = operandCount;
    Operand *s;
    for (int i = 0; (s = getOp(i)) != 0; i++) {
        Operand *d = c->getOp(i);
        if (d == 0) {
            reportError("fatal error, couldn't get operand during clone\n");
            exitAfterCleanup(-1);
        }
        if (s->unk_26 != i) {
            reportError("bad index in operand for clone\n");
            exitAfterCleanup(-1);
        }
        d->unk_26 = s->unk_26;
        d->unk_04 = s->unk_04;
        d->unk_09 = s->unk_09;
        d->unk_0a = s->unk_0a;
        d->unk_05 = s->unk_05;
        d->unk_0b = s->unk_0b;
        d->unk_18 = s->unk_18;
        d->unk_07 = s->unk_07;
        d->unk_0c = s->unk_0c;
        d->unk_0d = s->unk_0d;
        d->unk_08 = s->unk_08;
        d->unk_14 = s->unk_14;
        d->unk_10 = s->unk_10;
        if (s->unk_1c.isValid()) {
            if (!d->unk_1c.set(&s->unk_1c)) {
                reportError("failed to clone immediate\n");
                exitAfterCleanup(-1);
            }
        }
        d->unk_24 = s->unk_24;
        d->unk_25 = s->unk_25;
        d->unk_29 = s->unk_29;
        d->unk_27 = s->unk_27;
        d->unk_2b = 0;
        d->unk_06 = s->unk_06;
        d->unk_28 = s->unk_28;
    }
    return c;
}




int Instr::getRamBits()
{
    int r;
    if (!unk_13ce) {
        memset(&r, 0, sizeof(r));
        return r;
    }
    r = 0;
    if (opcodeInfo) {
        switch ((char)opcodeInfo->destinationMaskMode) {
        case 1:
            r = unk_138f;
            break;
        default:
            reportError("got here! - end of get rambits?\n");
            func_08054008(false);
            exitAfterCleanup(-1);
            memset(&r, 0, sizeof(r));
            return r;
        case 3: {
            Operand *op = getOp(1);
            if (!op) {
                reportError("failed to get op1 for mod_1 ram test\n");
                exitAfterCleanup(-1);
            }
            if (!op->unk_0c)
                reportWarning("mb was 0 failed to get xmod bits for mod 1 inst\n");
            r = op->unk_0c;
            break;
        }
        case 0:
        case 4:
        case 5:
            r = 15;
            break;
        }
    }
    return r;
}

bool Instr::func_08054b84()
{
    unk_13e8.func_080586e0();
    unk_2c = 0;
    unk_24.unk_04 = 0;
    unk_24.unk_00 = 0;
    unk_1418 = 0;
    unk_1398 = 0;
    unk_141c = 0;
    for (int i = 0; i != operandCount; i++) {
        if (!operands[i].func_08056a24()) {
            reportError("failed to optReset operand  %d\n", i);
            return false;
        }
    }
    return true;
}

int Instr::func_08054c24(RegPoolEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *op;
    for (int i = 0; (op = getOp(i)) != 0; i++) {
        RegPoolEntry *r;
        if (op->isOutput())
            r = op->unk_18;
        else
            r = 0;
        if (reg == r) {
            *paired = *paired || op->unk_0d;
            mask |= op->unk_07;
        }
    }
    return mask;
}

int Instr::func_08054cc4(RegPoolEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *op;
    for (int i = 0; (op = getOp(i)) != 0; i++) {
        RegPoolEntry *r;
        if (op->isInput())
            r = op->unk_18;
        else
            r = 0;
        if (reg == r) {
            *paired = *paired || op->unk_0d;
            mask |= op->unk_07;
        }
    }
    return mask;
}


bool Instr::func_08054d64(int n, int *out)
{
    Operand *op = getOp(n);
    if (!op) {
        reportError("failed to get operand %d\n", n);
        return false;
    }
    return op->unk_1c.getInt(out);
}







bool Instr::changeBranchImmediate(const char *s)
{
    if (!unk_1388 || !opcodeInfo) {
        reportError("changeBranchImmediate: not a branch\n");
        return false;
    }
    if (unk_1388 & 2) {
        reportError("changeBranchImmediate: target is register!\n");
        return false;
    }
    int n = opcodeInfo->unk_1a;
    if (!n) {
        reportError("getBranchTargetImmediate - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *op = &operands[n - 1];
    if (op->unk_04 != 0x1b) {
        reportError("changeBranchImmediate::target operand isn't immediate!\n");
        exitAfterCleanup(-1);
    }
    return op->unk_1c.setString(s);
}

bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot)
{
    if (!a || !b) {
        reportError("link_oParts, null arg pointer!\n");
        return false;
    }
    if (bSlot != slot) {
        reportError("swizzle detected for ps2 ?\n");
        return false;
    }
    OpSlot *sa = &a->operands[aOp].unk_2c[slot];
    OpSlot *sb = &b->operands[bOp].unk_2c[slot];
    ol_s *oa = sa->unk_54.addOperandLocation(b, bOp, slot, false);
    if (oa) {
        olList *lb = &sb->unk_54;
        ol_s *ob = lb->addOperandLocation(a, aOp, slot, false);
        if (!ob) {
            reportError("failed to add ol\n");
            return false;
        }
        return true;
    }
    reportError("failed to add ol\n");
    return false;
}

static inline void dumpLinkOperand(Operand *op)
{
    op->func_0805665c(0);
}

bool link_oParts(ol_s *o, ol_s *i, int x, int y)
{
    if (!o) {
        reportError("link_oParts, null arg pointer2!\n");
        return false;
    }
    if (!i) {
        reportError("link_oParts, null arg pointer2!\n");
        return false;
    }
    Operand *out = func_0805802c(o);
    if (!out) {
        reportError("link_oParts(2) - output not set \n");
        return false;
    }
    if (!isRegisterOperand(out, 2)) {
        reportError("link_oParts - output was not output\n");
        dumpLinkOperand(out);
        return false;
    }
    Operand *in = func_0805802c(i);
    if (!in) {
        reportError("link_oParts(2) - input not set \n");
        return false;
    }
    if (!isRegisterOperand(in, 1)) {
        reportError("link_oParts - input was not input\n");
        return false;
    }
    int to = o->unk_11;
    int ti = i->unk_11;
    if (!((out->unk_07 >> to) & 1)) {
        reportError("link_oParts - mod bit not set for output\n");
        return false;
    }
    if (!((in->unk_07 >> ti) & 1)) {
        reportError("link_oParts - mod bit not set for input\n");
        return false;
    }
    if (!((in->unk_27 >> ti) & 1)) {
        reportError("link_oParts_ext - external bit not set for input\n");
        return false;
    }
    out->unk_27 |= 1 << to;
    if (func_08058618(o) != func_08058618(i)) {
        out->unk_06 |= 1 << o->unk_11;
        in->unk_06 |= 1 << i->unk_11;
    }
    if (to != ti) {
        reportError("link_oParts - not a bug, but should not happen for this cpu, t_o!=t_i (%d!=%d)\n", to, ti);
        return false;
    }
    OpSlot *so = out->unk_2c + ti;
    OpSlot *si = in->unk_2c + ti;
    olList *lo = &so->unk_54;
    ol_s *a = lo->copyOperandLocation(i, true);
    if (!a) {
        reportError("failed to add ol_input to output\n");
        return false;
    }
    olList *li = &si->unk_54;
    ol_s *b = li->copyOperandLocation(o, true);
    if (!b) {
        reportError("failed to add ol_ouput to input\n");
        return false;
    }
    int s = o->unk_12;
    if (s > 0xff)
        s = 0xff;
    a->unk_12 = s;
    a->unk_08 = x;
    a->unk_0c = y;
    b->unk_12 = 0;
    b->unk_08 = x;
    b->unk_0c = y;
    return true;
}

bool Instr::setOwner(cls_unk30 *s)
{
    if (unk_30 && unk_30 != s) {
        reportError("in->setOwner - already owned..but by wrong stream!\n");
        exitAfterCleanup(-1);
    }
    unk_30 = s;
    return true;
}

const char *func_08055254(int t)
{
    switch (t) {
    case 0:
        return "BTYPE_NULL";
    case 1:
        return "BTYPE_ABS";
    case 3:
        return "BTYPE_REG";
    case 5:
        return "BTYPE_COND_ABS";
    case 7:
        return "BTYPE_COND_REG";
    case 9:
        return "BTYPE_ABS_LINK";
    case 11:
        return "BTYPE_REG_LINK";
    default:
        return "BTYPE_TYPE_THIS_IN!";
    case 13:
        return "BTYPE_COND_ABS_LINK";
    }
}

char *Instr::func_080552a8(bool flag, char *buf, int)
{
    if (!buf)
        return 0;
    iInfo *info = opcodeInfo;
    if (info) {
        buf += formatIntoBuffer(buf, info->mnemonic);
        switch ((char)info->destinationMaskMode) {
        case 1:
            buf += formatIntoBuffer(buf, ".[xyzw]");
            break;
        case 2:
            buf += formatIntoBuffer(buf, ".xyz");
            break;
        case 3:
            buf += formatIntoBuffer(buf, ".[x/y/z/w]");
            break;
        }
    } else {
        buf += formatIntoBuffer(buf, "nop");
    }
    buf += formatIntoBuffer(buf, "\t");
    for (int n = 0; n != operandCount; n++) {
        if (operands[n].unk_08)
            break;
        if (n)
            buf += formatIntoBuffer(buf, ",");
        buf = operands[n].func_08057b90(flag, buf);
        if (!buf)
            break;
    }
    return buf;
}

int Instr::func_080553b4(RegPoolEntry *reg, int mask)
{
    int result = 0;
    if (reg) {
        for (int i = 0; i != operandCount; i++) {
            Operand *op = &operands[i];
            if (op->outRegZ() == reg)
                result |= op->unk_07;
        }
    }
    return result & mask;
}

int Instr::func_0805544c(RegPoolEntry *reg, int mask)
{
    int result = 0;
    if (reg) {
        for (int i = 0; i != operandCount; i++) {
            Operand *op = &operands[i];
            if (op->inRegOp2() == reg)
                result |= op->unk_07;
        }
    }
    return result & mask;
}

const char *func_080554e4(int t)
{
    const char *s = "SYNTH_UNKNOWN";
    switch (t) {
    case 0: s = "SYNTH_NULL"; break;
    case 1: s = "SYNTH_ENTER"; break;
    case 2: s = "SYNTH_ENDENTER"; break;
    case 3: s = "SYNTH_EXIT"; break;
    case 4: s = "SYNTH_ENDEXIT"; break;
    case 5: s = "SYNTH_CONT"; break;
    case 6: s = "SYNTH_XINPUT"; break;
    case 7: s = "SYNTH_XOUTPUT"; break;
    case 8: s = "SYNTH_LOOP_MINSAFE"; break;
    case 9: s = "SYNTH_LOOP_EXTRA"; break;
    case 10: s = "SYNTH_BARRIER"; break;
    }
    return s;
}

bool Instr::func_08055548()
{
    if (unk_00)
        return true;
    if (unk_30)
        return unk_30->unk_09 == 3;
    return false;
}

char *Instr::getBranchTargetImmediate()
{
    iInfo *ii = opcodeInfo;
    if (!unk_1388)
        return 0;
    if (!ii) {
        reportError("branch type set - but no ii\n");
        exitAfterCleanup(-1);
    }
    int n = ii->unk_1a;
    if (!n) {
        reportError("getBranchTargetImmediate - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *op = &operands[n - 1];
    if (!op->isInput()) {
        reportError("alleged branch target immediate wasn't an input\n");
        exitAfterCleanup(-1);
    }
    if (op->unk_04 == 0x1b)
        return op->unk_1c.getString();
    return 0;
}

RegPoolEntry *Instr::getBranchTargetRegister()
{
    iInfo *ii = opcodeInfo;
    if (!(unk_1388 & 2))
        return 0;
    if (!ii) {
        reportError("branch type set - but no ii\n");
        exitAfterCleanup(-1);
    }
    int n = ii->unk_1a;
    if (!n) {
        reportError("getBranchTargetRegister - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *op = &operands[n - 1];
    if (!op->isInput()) {
        reportError("alleged branch target register wasn't an input\n");
        exitAfterCleanup(-1);
    }
    if (op->unk_04 == 0x1b)
        return 0;
    RegPoolEntry *r = op->unk_18;
    if (!r) {
        reportError("branch target register wasn't set\n");
        exitAfterCleanup(-1);
    }
    return r;
}

int Instr::func_08055718()
{
    return 0;
}

Operand *Instr::func_08055720(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->inRegOp2() == reg && (op->unk_07 & mask))
            return op;
    }
    return 0;
}

Operand *Instr::func_080557a4(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->outRegZ() == reg && (op->unk_07 & mask))
            return op;
    }
    return 0;
}

bool Instr::func_08055828(unsigned int n)
{
    if (!opcodeInfo)
        return false;
    for (; n < operandCount; n++) {
        Operand *op = &operands[n];
        RegPoolEntry *r = registerOperand(op, 2);
        if (r) {
            if (op->unk_27)
                return true;
            for (int j = 0; j != r->partCount; j++)
                if (((op->unk_07 >> j) & 1) && op->unk_2c[j].unk_54.count)
                    return true;
        }
    }
    return false;
}

bool Instr::func_080558f4()
{
    if (unk_13ce == 2) {
        unk_138e = 1;
        return true;
    }
    if (unk_13d0 & 0x40) {
        unk_138e = 2;
        return true;
    }
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (isRegisterOperand(op, 2) && op->unk_06) {
            unk_138e = 2;
            return true;
        }
    }
    unk_138e = 0;
    return true;
}

Instr **Instr::getIE()
{
    cls_unk30 *table = unk_30;
    if (!table) {
        reportError("failed to get itable from instruction\n");
        return 0;
    }
    InstructionPosition *pos = &unk_13e8;
    Instr **ie = table->getIE_ck(pos->unk_00);
    if (!ie) {
        reportError("failed to get ie from table for in::getIE()\n");
        return 0;
    }
    if (ie[pos->unk_02] != this) {
        reportError("we aren't where we thought we were for in::getIE\n");
        return 0;
    }
    return ie;
}

int Instr::func_08055a0c()
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->outRegZ() && !op->unk_0b)
            return op->unk_0a;
    }
    return 0;
}

bool Instr::func_08055a88()
{
    bool r = !unk_0e_1;
    unk_0e_1 = 1;
    return r;
}

bool Instr::func_08055aa8()
{
    bool r = !unk_0e_0;
    unk_0e_0 = 1;
    return r;
}

int Instr::func_08055ac4()
{
    if (!opcodeInfo)
        return -1;
    return func_0806c72c(opcodeInfo);
}

static inline bool isLiteral(mValue *v)
{
    return v->type == mValue::STRING;
}

bool Instr::func_08055ae4()
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->unk_04 < 22 || op->unk_04 > 25)
            continue;
        if (i != operandCount - 1) {
            reportError("virtual stat operand not final\n");
            return false;
        }
        operandCount--;
        switch (op->unk_04) {
        case 23:
            op->unk_08 = 1;
            op->unk_04 = 21;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(55);
            op->unk_07 = 1;
            op->unk_0a = 4;
            op->unk_0b = 0;
            op->unk_26 = operandCount++;
            op = &operands[i + 1];
            op->unk_08 = 1;
            op->unk_04 = 18;
            op->unk_09 = 1;
            op->unk_14 = op->unk_18 = func_0806c224(57);
            op->unk_07 = 1;
            op->unk_0a = -1;
            op->unk_0b = 1;
            op->unk_26 = operandCount++;
            break;
        case 22:
            op->unk_08 = 1;
            op->unk_04 = 20;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(56);
            op->unk_07 = 1;
            op->unk_0a = operands[0].unk_0a;
            op->unk_0b = 0;
            op->unk_26 = operandCount;
            op = &operands[i + 1];
            operandCount++;
            op->unk_08 = 1;
            op->unk_04 = 19;
            op->unk_09 = 1;
            op->unk_14 = op->unk_18 = func_0806c224(58);
            op->unk_07 = 1;
            op->unk_0a = -1;
            op->unk_0b = 1;
            op->unk_26 = operandCount++;
            break;
        case 24: {
            if (operands[i - 1].unk_04 != 27) {
                reportError("not immediate predecessor in fs test\n");
                return false;
            }
            bool a = true;
            bool b = true;
            bool c = true;
            bool d = true;
            mValue *v = &operands[i - 1].unk_1c;
            unsigned int imm;
            if (isLiteral(v)) {
                reportWarning("fsop has literals - so assuming the worst - if it were numerical vcl could figure out dependencies better\n");
            } else if (v->getUInt(&imm)) {
                unsigned int bits;
                int id = -1;
                if (opcodeInfo)
                    id = func_0806c72c(opcodeInfo);
                switch (id) {
                case 147:
                    break;
                case 148:
                    bits = ~imm;
                    goto test;
                case 146:
                    bits = imm;
                test:
                    a = (bits & 0xf) != 0;
                    b = (bits & 0x30) != 0;
                    c = (bits & 0xc00) != 0;
                    d = (bits & 0x3c0) != 0;
                    break;
                default:
                    reportError("bad fsop \n");
                    return false;
                }
            } else {
                reportError("failed to get immediate value\n");
                return false;
            }
            a = a | c;
            b = b | d;
            if (a) {
                op->unk_08 = 1;
                op->unk_04 = 21;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(55);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 0;
                op->unk_26 = operandCount;
                op++;
                operandCount++;
            }
            if (b) {
                op->unk_08 = 1;
                op->unk_04 = 20;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(56);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 0;
                op->unk_26 = operandCount;
                op++;
                operandCount++;
            }
            if (c) {
                op->unk_08 = 1;
                op->unk_04 = 18;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(57);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 1;
                op->unk_26 = operandCount;
                op++;
                operandCount++;
                op->unk_08 = 1;
                op->unk_04 = 18;
                op->unk_09 = 2;
                op->unk_14 = op->unk_18 = func_0806c224(57);
                op->unk_07 = 1;
                op->unk_0a = 1;
                op->unk_0b = 1;
                op->unk_26 = operandCount;
                op++;
                operandCount++;
            }
            if (d) {
                op->unk_08 = 1;
                op->unk_04 = 19;
                op->unk_09 = 1;
                op->unk_14 = op->unk_18 = func_0806c224(58);
                op->unk_07 = 1;
                op->unk_0a = -1;
                op->unk_0b = 1;
                op->unk_26 = operandCount;
                op++;
                operandCount++;
                op->unk_08 = 1;
                op->unk_04 = 19;
                op->unk_09 = 2;
                op->unk_14 = op->unk_18 = func_0806c224(58);
                op->unk_07 = 1;
                op->unk_0a = 1;
                op->unk_0b = 1;
                op->unk_26 = operandCount++;
            }
            break;
        }
        case 25:
            op->unk_08 = 1;
            op->unk_04 = 18;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(57);
            op->unk_07 = 1;
            op->unk_0a = 1;
            op->unk_0b = 1;
            op->unk_26 = operandCount;
            op = &operands[i + 1];
            operandCount++;
            op->unk_08 = 1;
            op->unk_04 = 19;
            op->unk_09 = 2;
            op->unk_14 = op->unk_18 = func_0806c224(58);
            op->unk_07 = 1;
            op->unk_0a = 1;
            op->unk_0b = 1;
            op->unk_26 = operandCount++;
            break;
        }
    }
    return true;
}

void Instr::func_08055f4c()
{
    unk_13b0 = 0;
    unk_13b2 = 0;
    unk_13b4 = 0;
    unk_13b6 = 0;
}

void Instr::func_08055f78(Instr *src)
{
    unk_13b0 = src->unk_13b0;
    unk_13b2 = src->unk_13b2;
    unk_13b4 = src->unk_13b4;
    unk_13b6 = src->unk_13b6;
}

void Instr::func_08055fbc(short v)
{
    unk_13b0 = unk_13b0 < v ? v : unk_13b0;
}

void Instr::func_08055fdc(short v)
{
    unk_13b2 = unk_13b2 < v ? v : unk_13b2;
}

void Instr::func_08055ffc(short v)
{
    unk_13b4 = unk_13b4 < v ? v : unk_13b4;
}

void Instr::func_0805601c(short v)
{
    unk_13b6 = unk_13b6 < v ? v : unk_13b6;
}

OpSlot::OpSlot()
{
}

OpSlot::~OpSlot()
{
}

void OpSlot::func_080560bc()
{
}

bool OpSlot::func_080560c4()
{
    func_0805bcc4();
    unk_50 = 0;
    unk_52[0] = 0;
    return true;
}


int OpSlot::func_080560e8()
{
    return unk_54.clear();
}
