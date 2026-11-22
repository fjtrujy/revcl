

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

static inline bool initPrune_p(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!initPrune(stream)) {
            reportError("failed to init prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

static inline int tryPrune_p(Program *program)
{
    int n = 0;
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        int r = tryPrune(stream);
        if (r < 0) {
            reportError("flowMon: failed to  tryPrune %s\n", stream->streamName());
            return -1;
        }
        n += r;
    }
    return n;
}

static inline bool doPrune_p(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!doPrune(stream)) {
            reportError("failed to do prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

bool Prune(Program *program)
{
    if (!initPrune_p(program)) {
        reportError("failed to init prune!\n");
        return false;
    }
    int n;
    do {
        n = tryPrune_p(program);
        if (n < 0) {
            reportError("flowMon::Prune - an error occured\n");
            exitAfterCleanup(-1);
        }
    } while (n);
    if (!doPrune_p(program)) {
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
    for (Instr *in = t->linearInstructions.head; in; in = in->linearLink.nextInstruction)
        if (!initPrune(in)) {
            reportError("init prune failed\n");
            return false;
        }
    return true;
}

bool initPrune(Instr *in)
{
    in->pruningRestricted = 0;
    if (in->opcodeInfo) {
        char ok = in->opcodeInfo->canPruneOperands();
        if (!ok)
            in->pruningRestricted = 1;
        for (int i = 0;; i++) {
            Operand *op = in->getOp(i);
            if (!op)
                break;
            op->pruneMask = 0;
            if (op->isInput() && in->pruningRestricted)
                op->pruneMask = op->componentMask;
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

static inline void dumpPruneInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
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
    if (t->codeModeValue == 4 || t->getTableRawMode() == 4)
        raw = true;
    for (Instr *in = t->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
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
                (RegPoolEntry *&)r = r->assignedRegister;
            else
                (RegPoolEntry *&)r = 0;
            if (r)
                op->pruneMask = op->componentMask;
        }
    }
    Operand *out;
    for (int o = 0; (out = in->getOp(o)) != 0; o++) {
        int isOut = out->direction == 2;
        RegPoolEntry *reg = isOut ? out->assignedRegister : 0;
        if (!reg)
            continue;
        int cnt = 0;
        for (int s = 0; s != reg->partCount; s++) {
            if ((out->componentMask >> s) & 1)
            for (ol_s *ol = out->parts[s].operandLocations.head; ol; ol = ol->next) {
                Operand *src = operandAtLocation(ol);
                if (!src) {
                    reportError("failed to get an operand from an oploc\n");
                    return -1;
                }
                int sw = ol->componentIndex;
                if (src->assignedRegister != reg) {
                    reportError("there was a reg mismatch\n");
                    return -1;
                }
                if (sw != s) {
                    reportError("swizzle detected\n");
                    return -1;
                }
                if (!((src->componentMask >> s) & 1)) {
                    reportError("there was a mop bit mismatch\n");
                    return -1;
                }
                if (((src->pruneMask >> s) & 1) && ((~out->pruneMask >> s) & 1)) {
                    cnt++;
                    n++;
                    out->pruneMask |= 1 << s;
                }
            }
        }
        if (!cnt)
            continue;
        if (out->operandKind == 10 || out->operandKind == 11)
            out->pruneMask = out->componentMask;
        Operand *ip;
        for (int k = 0; (ip = in->getOp(k)) != 0; k++) {
            int isIn = ip->direction == 1;
            RegPoolEntry *r = isIn ? ip->assignedRegister : 0;
            if (!r)
                continue;
            int bits = hackIOmask(in, o, out->pruneMask, k);
            if (bits < 0) {
                reportError("lprune: failed to deduce which inputs affected which outputs\n");
                return -1;
            }
            if ((ip->componentMask & bits) != bits) {
                reportError("bits were used.. but outside of mod bit mask!\n");
                reportError("OUTPUT(%d) INPUT(%d) inst : \n", o, k);
                reportError("A_BITS=%x .. INPUT_MODBITS=%x : \n", bits, ip->componentMask);
                reportError("otype is opt->m_oType=%d\n", ip->operandKind);
                dumpPruneInstruction(in);
                return -1;
            }
            ip->pruneMask |= bits;
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
extern bool quietDiagnostics;

bool doPrune(instStream *s)
{
    workTable *t = s->getTable();
    if (!t) {
        reportError("failed to get work table\n");
        return false;
    }
    if (t->getTableRawMode() == 4 || t->getTableRawMode() == 3)
        return true;
    Instr *first = t->linearInstructions.head;
    Instr *in = first;
    Instr *next;
    if (first) do {
        next = in->linearLink.nextInstruction;
        int r = doPrune(in);
        if (r < 0) {
            reportError("do  prune failed for instruction\n");
            dumpPruneInstruction(in);
            return false;
        }
        if (r == 0) {
            if (!in->isNop()) {
                reportVerboseInfo("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = quietDiagnostics;
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

bool rotateInputComponentMask(void *, Instr *in);

int doPrune(Instr *in)
{
    int i;
    Operand *op;
    unsigned char any = 0;
    if (in->pruningRestricted)
        return 1;
    for (i = 0; (op = in->getOp(i)) != 0; i++) {
        any |= op->pruneMask;
    }
    if (!any)
        return 0;
    iInfo *info = in->opcodeInfo;
    if (!info) {
        reportError("Instruction::doPrune() - had no iInfo\n");
        return -1;
    }
    if (in->abstractMoveOpcode && in->operands[0].assignedRegister == in->operands[1].assignedRegister)
        return 0;
    if (info->destinationMaskMode == 1) {
        int m, nIn = 0, inOR = 0, outOR = 0, nOut = 0;
        for (i = 0; (op = in->getOp(i)) != 0; i++) {
            if (op->operandKind != 1 && op->operandKind != 9)
                continue;
            if (!op->assignedRegister) {
                reportError("couldn't get a register for V_FL4 or V_ACC operand\n");
                return -1;
            }
            if (op->isInput()) {
                inOR |= op->pruneMask;
                nIn++;
            }
            if (op->isOutput()) {
                if (nOut && op->pruneMask != outOR) {
                    reportError("prune - found two inputs with different pBits state for FL4/ACC\n");
                    return -1;
                }
                outOR |= op->pruneMask;
                nOut++;
            }
        }
        if (!nOut && !nIn) {
            reportError("MD_ANY set - but couldn't find any fl4 or acc operands for %s\n", info->mnemonic);
            return -1;
        }
        if (nOut > 1) {
            reportError("nOut >2! for fl4/acc\n ");
            in->leDump();
            return -1;
        }
        if (info->decodeHook == rotateInputComponentMask)
            inOR = (inOR >> 1) | ((inOR & 1) << 3);
        if (nIn) {
            if (outOR & ~inOR) {
                reportError("doPrune:: output had larger extent than input.. out=%x vs in=%x\n", outOR, inOR);
                in->leDump();
                return -1;
            }
            m = inOR;
        } else if (!outOR)
            m = in->destinationMask;
        else
            m = outOR;
        if (m & ~in->destinationMask) {
            reportError("inOR was out of range of instruction mod bits nb=%x,mb=%x!\n", m, in->destinationMask);
            dumpPruneInstruction(in);
            return -1;
        }
        if (m != in->destinationMask) {
            reportVerboseInfo("PRUNE FIELD: [%s->%s]  ", formatComponentMaskLetters(in->destinationMask), formatComponentMaskLetters(m));
            int saved = errorOccurred;
            errorOccurred = quietDiagnostics;
            in->leDump();
            errorOccurred = saved;
        }
        in->destinationMask = m;
        for (i = 0; (op = in->getOp(i)) != 0; i++) {
            if (op->operandKind == 1 || op->operandKind == 9)
                op->componentMask = m;
        }
        if (info->decodeHook == rotateInputComponentMask)
            in->operands[1].componentMask = (((m & 8) >> 3) | (m << 1)) & 0xf;
    }
    for (i = 0; (op = in->getOp(i)) != 0; i++) {
        RegPoolEntry *r = registerOperand(op, 2);
        if (!r || op->pruneMask || !r->isVirtual)
            continue;
        RegPool *pool = r->ownerPool;
        if (!pool) {
            reportError("register had no regpool\n");
            return -1;
        }
        if (pool->zeroRegister && !op->pairedOperandIndexPlusOne) {
            reportVerboseInfo("ZERO FIX   : ");
            int saved = errorOccurred;
            errorOccurred = quietDiagnostics;
            in->leDump(i);
            errorOccurred = saved;
            op->assignedRegister = pool->zeroRegister;
        }
    }
    return 1;
}

