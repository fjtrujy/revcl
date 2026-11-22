

#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "VUStr.h"

bool initPrune(instStream *s);
bool initPrune(Instr *in);
int tryPrune(instStream *s);
int tryPrune(Instr *in, bool raw);
bool doPrune(instStream *s);
int doPrune(Instr *in);
int hackIOmask(Instr *in, int outOp, unsigned char mask, int inOp);

static inline bool initPrune_p(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream)
        if (!initPrune(s)) {
            reportError("failed to init prune for %s\n", s->streamName());
            return false;
        }
    return true;
}

static inline int tryPrune_p(Program *p)
{
    int n = 0;
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        int r = tryPrune(s);
        if (r < 0) {
            reportError("flowMon: failed to  tryPrune %s\n", s->streamName());
            return -1;
        }
        n += r;
    }
    return n;
}

static inline bool doPrune_p(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream)
        if (!doPrune(s)) {
            reportError("failed to do prune for %s\n", s->streamName());
            return false;
        }
    return true;
}

bool Prune(Program *p)
{
    if (!initPrune_p(p)) {
        reportError("failed to init prune!\n");
        return false;
    }
    int n;
    do {
        n = tryPrune_p(p);
        if (n < 0) {
            reportError("flowMon::Prune - an error occured\n");
            exitAfterCleanup(-1);
        }
    } while (n);
    if (!doPrune_p(p)) {
        reportError("failed to do prune!\n");
        return false;
    }
    return true;
}

bool initPrune(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream)
        if (!initPrune(s)) {
            reportError("failed to init prune for %s\n", s->streamName());
            return false;
        }
    return true;
}

bool initPrune(instStream *s)
{
    workTable *t = s->getTable();
    if (!t) {
        reportError("failed to get work table\n");
        return false;
    }
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        if (!initPrune(in)) {
            reportError("init prune failed\n");
            return false;
        }
    return true;
}

bool initPrune(Instr *in)
{
    in->unk_13cc = 0;
    if (in->opcodeInfo) {
        char ok = in->opcodeInfo->func_0807026c();
        if (!ok)
            in->unk_13cc = 1;
        for (int i = 0;; i++) {
            Operand *op = in->getOp(i);
            if (!op)
                break;
            op->unk_2a = 0;
            if (op->isInput() && in->unk_13cc)
                op->unk_2a = op->unk_07;
        }
        return true;
    }
    reportError("found inst with no m_Inst\n");
    return false;
}

int tryPrune(Program *p)
{
    int n = 0;
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        int r = tryPrune(s);
        if (r < 0) {
            reportError("flowMon: failed to  tryPrune %s\n", s->streamName());
            return -1;
        }
        n += r;
    }
    return n;
}

static inline void dumpPruneInstruction(Instr *in)
{
    in->func_08054008(false);
}

int tryPrune(instStream *s)
{
    int n = 0;
    workTable *t = s->getTable();
    if (!t) {
        reportError("failed to get work table\n");
        return 0;
    }
    bool raw = false;
    if (t->unk_09 == 4 || t->func_08073754() == 4)
        raw = true;
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        int r = tryPrune(in, raw);
        if (r < 0) {
            reportError("rec  prune failed for instruction\n");
            dumpPruneInstruction(in);
            return -1;
        }
        n += r;
    }
    return n;
}

int tryPrune(Instr *in, bool raw)
{
    int n = 0;
    if (!in->opcodeInfo)
        return 0;
    if (raw) {
        Operand *r, *op, *lookup;
        for (int i = 0;
             ((i < 0 ? (lookup = in->getOp(i)) :
                       (lookup = i < in->operandCount ? &in->operands[i] : 0)),
              r = lookup, op = lookup, lookup = 0, op) != 0; i++) {
            if (r->isInput())
                (RegPoolEntry *&)r = r->unk_18;
            else
                (RegPoolEntry *&)r = 0;
            if (r)
                op->unk_2a = op->unk_07;
        }
    }
    Operand *out;
    for (int o = 0; (out = in->getOp(o)) != 0; o++) {
        int isOut = out->unk_09 == 2;
        RegPoolEntry *reg = isOut ? out->unk_18 : 0;
        if (!reg)
            continue;
        int cnt = 0;
        for (int s = 0; s != reg->partCount; s++) {
            if ((out->unk_07 >> s) & 1)
            for (ol_s *ol = out->unk_2c[s].unk_54.unk_00; ol; ol = ol->unk_00) {
                Operand *src = func_0805802c(ol);
                if (!src) {
                    reportError("failed to get an operand from an oploc\n");
                    return -1;
                }
                int sw = ol->unk_11;
                if (src->unk_18 != reg) {
                    reportError("there was a reg mismatch\n");
                    return -1;
                }
                if (sw != s) {
                    reportError("swizzle detected\n");
                    return -1;
                }
                if (!((src->unk_07 >> s) & 1)) {
                    reportError("there was a mop bit mismatch\n");
                    return -1;
                }
                if (((src->unk_2a >> s) & 1) && ((~out->unk_2a >> s) & 1)) {
                    cnt++;
                    n++;
                    out->unk_2a |= 1 << s;
                }
            }
        }
        if (!cnt)
            continue;
        if (out->unk_04 == 10 || out->unk_04 == 11)
            out->unk_2a = out->unk_07;
        Operand *ip;
        for (int k = 0; (ip = in->getOp(k)) != 0; k++) {
            int isIn = ip->unk_09 == 1;
            RegPoolEntry *r = isIn ? ip->unk_18 : 0;
            if (!r)
                continue;
            int bits = hackIOmask(in, o, out->unk_2a, k);
            if (bits < 0) {
                reportError("lprune: failed to deduce which inputs affected which outputs\n");
                return -1;
            }
            if ((ip->unk_07 & bits) != bits) {
                reportError("bits were used.. but outside of mod bit mask!\n");
                reportError("OUTPUT(%d) INPUT(%d) inst : \n", o, k);
                reportError("A_BITS=%x .. INPUT_MODBITS=%x : \n", bits, ip->unk_07);
                reportError("otype is opt->m_oType=%d\n", ip->unk_04);
                dumpPruneInstruction(in);
                return -1;
            }
            ip->unk_2a |= bits;
        }
    }
    return n;
}

bool doPrune(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream)
        if (!doPrune(s)) {
            reportError("failed to do prune for %s\n", s->streamName());
            return false;
        }
    return true;
}

extern int errorOccurred;
extern bool quiet_080bbd98;

bool doPrune(instStream *s)
{
    workTable *t = s->getTable();
    if (!t) {
        reportError("failed to get work table\n");
        return false;
    }
    if (t->func_08073754() == 4 || t->func_08073754() == 3)
        return true;
    Instr *first = t->unk_88.unk_00;
    Instr *in = first;
    Instr *next;
    if (first) do {
        next = in->unk_1c.unk_00;
        int r = doPrune(in);
        if (r < 0) {
            reportError("do  prune failed for instruction\n");
            dumpPruneInstruction(in);
            return false;
        }
        if (r == 0) {
            if (!in->func_08054138()) {
                reportVerboseInfo("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = quiet_080bbd98;
                in->leDump();
                errorOccurred = saved;
            }
            if (!t->destroyInstruction(in)) {
                reportError("failed to destroy instruction\n");
                if (in)
                    dumpPruneInstruction(in);
                return false;
            }
        }
        in = next;
    } while (in);
    return true;
}

bool func_08087860(void *, Instr *in);

int doPrune(Instr *in)
{
    int i;
    Operand *op;
    unsigned char any = 0;
    if (in->unk_13cc)
        return 1;
    for (i = 0; (op = in->getOp(i)) != 0; i++) {
        any |= op->unk_2a;
    }
    if (!any)
        return 0;
    iInfo *info = in->opcodeInfo;
    if (!info) {
        reportError("Instruction::doPrune() - had no iInfo\n");
        return -1;
    }
    if (in->unk_142c && in->operands[0].unk_18 == in->operands[1].unk_18)
        return 0;
    if (info->unk_0c == 1) {
        int m, nIn = 0, inOR = 0, outOR = 0, nOut = 0;
        for (i = 0; (op = in->getOp(i)) != 0; i++) {
            if (op->unk_04 != 1 && op->unk_04 != 9)
                continue;
            if (!op->unk_18) {
                reportError("couldn't get a register for V_FL4 or V_ACC operand\n");
                return -1;
            }
            if (op->isInput()) {
                inOR |= op->unk_2a;
                nIn++;
            }
            if (op->isOutput()) {
                if (nOut && op->unk_2a != outOR) {
                    reportError("prune - found two inputs with different pBits state for FL4/ACC\n");
                    return -1;
                }
                outOR |= op->unk_2a;
                nOut++;
            }
        }
        if (!nOut && !nIn) {
            reportError("MD_ANY set - but couldn't find any fl4 or acc operands for %s\n", info->unk_04);
            return -1;
        }
        if (nOut > 1) {
            reportError("nOut >2! for fl4/acc\n ");
            in->leDump();
            return -1;
        }
        if (info->unk_20 == func_08087860)
            inOR = (inOR >> 1) | ((inOR & 1) << 3);
        if (nIn) {
            if (outOR & ~inOR) {
                reportError("doPrune:: output had larger extent than input.. out=%x vs in=%x\n", outOR, inOR);
                in->leDump();
                return -1;
            }
            m = inOR;
        } else if (!outOR)
            m = in->unk_138f;
        else
            m = outOR;
        if (m & ~in->unk_138f) {
            reportError("inOR was out of range of instruction mod bits nb=%x,mb=%x!\n", m, in->unk_138f);
            dumpPruneInstruction(in);
            return -1;
        }
        if (m != in->unk_138f) {
            reportVerboseInfo("PRUNE FIELD: [%s->%s]  ", func_08052064(in->unk_138f), func_08052064(m));
            int saved = errorOccurred;
            errorOccurred = quiet_080bbd98;
            in->leDump();
            errorOccurred = saved;
        }
        in->unk_138f = m;
        for (i = 0; (op = in->getOp(i)) != 0; i++) {
            if (op->unk_04 == 1 || op->unk_04 == 9)
                op->unk_07 = m;
        }
        if (info->unk_20 == func_08087860)
            in->operands[1].unk_07 = (((m & 8) >> 3) | (m << 1)) & 0xf;
    }
    for (i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *r = registerOperand(op, 2);
        if (!r || op->unk_2a || !r->isVirtual)
            continue;
        RegPool *pool = r->ownerPool;
        if (!pool) {
            reportError("register had no regpool\n");
            return -1;
        }
        if (pool->zeroRegister && !op->unk_0d) {
            reportVerboseInfo("ZERO FIX   : ");
            int saved = errorOccurred;
            errorOccurred = quiet_080bbd98;
            in->leDump(i);
            errorOccurred = saved;
            op->unk_18 = pool->zeroRegister;
        }
    }
    return 1;
}

