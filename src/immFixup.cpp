#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"
#include "os.h"

bool aaFixup(instStream *s);
bool resolveInstructionImmediateFixup(Instr *in);
int maddImmediate(Operand *op, mValue *v, int n);

static inline void initImm(Instr *in)
{
    in->addressAliasMode = 0;
    in->addressAliasDirection = 0;
    in->addressAliasOperandIndex = -1;
    in->addressImmediateValue.clear();
    in->expectedAliasLoopId = -1;
}

static inline bool initStreamImm(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        reportError("Init_immFixup - failed to get work table\n");
        return false;
    }
    s->streamSerial = 0;
    for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
        initImm(in);
    return true;
}

static inline void fixInst(Instr *in)
{
    if (!resolveInstructionImmediateFixup(in)) {
        reportError("do_immFixup - failed for an inst\n");
        exitAfterCleanup(-1);
    }
}

static inline bool do_immFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt)
        return true;
    for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
        fixInst(in);
    return true;
}

static inline bool dumpFailed(instStream *s)
{
    s->dump(false);
    return false;
}

bool resolveProgramImmediateFixups(Program *p, bool init)
{
    instStream *s;
    if (init) {
        for (s = p->streamHead; s; s = s->nextStream)
            if (!initStreamImm(s))
                return false;
        for (s = p->streamHead; s; s = s->nextStream)
            if (!aaFixup(s))
                return dumpFailed(s);
    } else {
        for (s = p->streamHead; s; s = s->nextStream)
            if (!do_immFixup(s))
                return false;
        for (s = p->streamHead; s; s = s->nextStream)
            if (!initStreamImm(s))
                return false;
    }
    return true;
}


static inline bool bit0(unsigned char m)
{
    return m & 1;
}

static inline bool bit0Clear(unsigned char m)
{
    m ^= 1;
    return m & 1;
}

static inline bool hasAaData(Instr *in)
{
    if (in->addressAliasMode != 0 || in->addressAliasDirection != 0 ||
        in->addressAliasOperandIndex != -1 || in->addressImmediateValue.isValid())
        return true;
    return false;
}

static inline bool immValid(mValue *v)
{
    return v->isValid();
}

static inline regUse *firstRegUse(workTable *wt) { return wt->registerUses.head; }
static inline RegPoolEntry *regUseReg(regUse *ru) { return ru->registerEntry; }
static inline Instr *regUseInstr(regUse *ru) { return ru->definingInstruction; }
static inline olList *regUseVisList0(regUse *ru) { return &ru->slots[0].inputLocations; }
static inline unsigned char regUseExtIn(regUse *ru) { return ru->externalInputMask; }
static inline unsigned char regUseExtOut(regUse *ru) { return ru->externalOutputMask; }
static inline Operand *instrRawOp(Instr *in, unsigned char idx) { return &in->operands[idx]; }
static inline Operand *instrRawOpS(Instr *in, signed char idx) { return &in->operands[idx]; }
static inline RegPoolEntry *opRawReg(Operand *op) { return op->assignedRegister; }
static inline unsigned char opSlotMask(Operand *op) { return op->componentMask; }
static inline mValue *opImmVal(Operand *op) { return &op->immediateValue; }
static inline olList *opSlotUses0(Operand *op) { return &op->parts[0].operandLocations; }
static inline Instr *olInstr(ol_s *ol) { return ol->instruction; }
static inline char olOpIndex(ol_s *ol) { return ol->operandIndex; }
static inline mValue *instrAaImm(Instr *in) { return &in->addressImmediateValue; }
static inline unsigned char aaOutOp(aaDesc *aa) { return aa->unk_06; }
static inline unsigned char aaInOp(aaDesc *aa) { return aa->unk_05; }
static inline signed char aaImmOp(aaDesc *aa) { return aa->unk_07; }
static inline int aaImmLit(aaDesc *aa) { return aa->unk_08; }
static inline unsigned char aaUseOp(aaUse *u) { return u->unk_04; }

bool aaFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        reportError("Init_immFixup - failed to get work table\n");
        return false;
    }
    for (regUse *ru = firstRegUse(wt); ru; ru = ru->next) {
        RegPoolEntry *reg = regUseReg(ru);
        if (!reg) {
            reportError("aaFixup - regref had no register\n");
            exitAfterCleanup(-1);
        }
        Instr *in = regUseInstr(ru);
        olList *vis = regUseVisList0(ru);
        aaDesc *aa = findAddressAliasDescriptor(in);
        if (!aa || !bit0(regUseExtIn(ru)) || !bit0(regUseExtOut(ru)) || ru->unk_18 != 1)
            continue;
        RegPoolEntry *outReg = in->operands[aa->unk_06].assignedRegister;
        if (outReg != reg || opRawReg(instrRawOp(in, aaInOp(aa))) != outReg)
            continue;
        ol_s *ol;
        for (ol = vis->firstInternalLocation(); ol; ol = ol->nextInternalLocation()) {
            Instr *vi = olInstr(ol);
            Operand *vop = operandAtLocation(ol);
            if (!vi)
                goto bad1;
            if (!vop)
                goto bad2;
        cont:
            {
            aaUse *u = findAddressAliasUse(vi);
            if (u && olOpIndex(ol) == aaUseOp(u) && opSlotMask(vop) == 1) {
                if (hasAaData(vi)) {
                    reportError("instruction already has aa data loaded .. mode before\n");
                    vi->dumpOperandDiagnostics(false);
                    immValid(instrAaImm(vi));
                    return false;
                }
                vi->addressAliasDirection = 1;
                vi->addressAliasMode = aa->unk_04;
                vi->addressAliasOperandIndex = u->unk_04;
                mValue *v = instrAaImm(vi);
                if (aa->unk_07 >= 0) {
                    if (!v->set(opImmVal(instrRawOpS(in, aaImmOp(aa))))) {
                        reportError("failed to set xaaImm for before\n");
                        return false;
                    }
                } else
                    v->setInt(aaImmLit(aa));
            }
            continue;
            }
        bad1:
            reportError("failed to get instruction for visible input\n");
            exitAfterCleanup(-1);
            goto cont;
        bad2:
            reportError("failed to get instruction for visible input\n");
            exitAfterCleanup(-1);
            goto cont;
        }
        Operand *out = in->getOp(aaOutOp(aa));
        if (!out || !out->isOutput() || bit0Clear(opSlotMask(out))) {
            reportError("failed to get output operand for aa\n");
            exitAfterCleanup(-1);
        }
        for (ol = opSlotUses0(out)->firstInternalLocation(); ol; ol = ol->nextInternalLocation()) {
            Instr *vi = olInstr(ol);
            Operand *vop = operandAtLocation(ol);
            if (!vi || !vop) {
                reportError("failed to get instruction for visible input.. mode after\n");
                exitAfterCleanup(-1);
            }
            aaUse *u = findAddressAliasUse(vi);
            if (u && olOpIndex(ol) == aaUseOp(u) && opSlotMask(vop) == 1) {
                if (hasAaData(vi)) {
                    reportError("instruction already has aa data loaded.. mode after\n");
                    vi->dumpOperandDiagnostics(false);
                    immValid(instrAaImm(vi));
                    return false;
                }
                mValue *v = instrAaImm(vi);
                vi->addressAliasDirection = 2;
                vi->addressAliasMode = aa->unk_04;
                vi->addressAliasOperandIndex = u->unk_04;
                if (aa->unk_07 >= 0) {
                    if (!v->set(opImmVal(instrRawOpS(in, aaImmOp(aa))))) {
                        reportError("failed to set xaaImm after\n");
                        return false;
                    }
                } else
                    v->setInt(aaImmLit(aa));
            }
        }
    }
    return true;
}

bool tidyStream(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt)
        return true;
    for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
        fixInst(in);
    return true;
}

bool tidyTable(workTable *wt)
{
    for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
        fixInst(in);
    return true;
}

bool tidyProgram(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream)
        if (!tidyStream(s)) {
            reportError("failed to tidy a stream\n");
            exitAfterCleanup(-1);
        }
    return true;
}

struct epSrc {
    short lid;
    short type;
};

bool getSrcEpLid(olList *oll, int luid, int epType, int *lid)
{
    if (luid < 0) {
        reportError("entered getSrcEpLid with -ve luid\n");
        return false;
    }
    if (!oll->count) {
        reportError("oll has no entries for getSrcEpLid\n");
        return false;
    }
    int nLocal = 0;
    ol_s *ol;
    for (ol = oll->head; ol; ol = ol->next) {
        Instr *in = ol->instruction;
        if (!in) {
            reportError("failed to get in from ol\n");
            exitAfterCleanup(-1);
        }
        instStream *s = in->getOwningStream();
        if (!s) {
            reportError("getSrcEpLid- failed to get owner\n");
            return false;
        }
        if (s->streamSerial != luid) {
            *lid = -1;
            return true;
        }
    }
    epSrc src[2];
    for (ol = oll->head; ol; ol = ol->next) {
        Instr *in = ol->instruction;
        if (!in) {
            reportError("failed to get in from ol\n");
            exitAfterCleanup(-1);
        }
        instStream *s = in->getOwningStream();
        if (!s) {
            reportError("getSrcEpLid- failed to get owner\n");
            return false;
        }
        int type = s->loopRegionRole;
        if (s->streamSerial != luid) {
            reportError("getSrcEpLid() - rather unlikely\n");
            return false;
        }
        if (nLocal > 1) {
            reportError("nLocal got >1 in getSrcEpLid()\n");
            oll->validateOperandLocations();
            return false;
        }
        src[nLocal].lid = in->expectedAliasLoopId;
        src[nLocal].type = type;
        nLocal++;
    }
    if (nLocal == 0) {
        reportError("no valid eplids found for getSrc\n");
        return false;
    }
    if (nLocal == 2) {
        if (src[0].type == src[1].type) {
            reportError("nLocal is 2 , but both setters from same type of construct\n");
            return false;
        }
        if (epType == 2) {
            reportError("nLocal is 2, but we are a prologue!\n");
            return false;
        }
        for (int i = 0; i != 2; i++) {
            switch (epType) {
            case 3:
                switch (src[i].type) {
                case 2:
                    break;
                case 5:
                    src[i].lid--;
                    break;
                default:
                    reportError("epSrc: we are EPI, but not MAIN/PRO setter for n=2\n");
                    return false;
                }
                break;
            case 5:
                switch (src[i].type) {
                case 2:
                    break;
                case 5:
                    src[i].lid--;
                    break;
                default:
                    reportError("epSrc: we are MAIN, but not MAIN/PRO setter\n");
                    return false;
                }
                break;
            default:
                reportError("bad loop type found for n=2\n");
                return false;
            }
        }
        if (src[0].lid != src[1].lid) {
            reportError("lid mismatch for n=2\n");
            return false;
        }
    } else {
    switch (epType) {
    case 2:
        break;
    case 3:
        switch (src[0].type) {
        case 2:
        case 3:
            break;
        case 5:
            src[0].lid--;
            break;
        default:
            reportError("bad ep type for n=1\n");
            return false;
        }
        break;
    case 5:
        if (src[0].type != 2 && src[0].type != 5) {
            reportError("bad ep type for n=1  %d\n", src[0].type);
            return false;
        }
        break;
    default:
        reportError("mylm is bad for n=1\n");
        return false;
    }
    }
    *lid = src[0].lid;
    return true;
}

static inline bool isCinput(Operand *op)
{
    return op->direction == 1;
}

bool resolveInstructionImmediateFixup(Instr *in)
{
    instStream *s = in->getOwningStream();
    if (!s) {
        reportError("failed to get owner istream from inst during immediate fixup\n");
        exitAfterCleanup(-1);
    }
    if (!in->addressAliasMode)
        return true;
    Operand *op = in->getOp(in->addressAliasOperandIndex);
    if (!op) {
        reportError("failed to get opernad for imm fix\n");
        exitAfterCleanup(-1);
    }
    if (!isCinput(op)) {
        reportError("operand isn't cinput for in fix\n");
        exitAfterCleanup(-1);
    }
    int ep;
    if (s->streamSerial) {
        if (in->expectedAliasLoopId < 0) {
            reportError("loop, epID<0 for operand in im fixup aaDstMode=%d..epMode=%d\n",
                          in->addressAliasDirection, in->expectedAliasLoopId);
            in->dumpOperandDiagnostics(false);
            exitAfterCleanup(-1);
        }
        ep = in->expectedAliasLoopId;
    } else {
        if (in->expectedAliasLoopId >= 0) {
            reportError("nonloop, epID>=0 for operand in im fixup aaDstmode=%d..epMode=%d\n",
                          in->addressAliasDirection, in->expectedAliasLoopId);
            in->dumpOperandDiagnostics(false);
            exitAfterCleanup(-1);
        }
        ep = 0;
    }
    if (in->addressAliasDirection == 1)
        ep--;
    int lid;
    if (s->streamSerial) {
        olList *oll = &op->parts[0].operandLocations;
        if (!getSrcEpLid(oll, s->streamSerial, s->loopRegionRole, &lid)) {
            reportError("failed to determine actual loopid of setter.. we are:\n");
            in->leDump();
            if (in->getOwningStream())
                reportError("parent stream is : %s\n", in->getOwningStream()->streamName());
            reportError("expected was %d\n", in->expectedAliasLoopId);
            exitAfterCleanup(-1);
        }
    } else
        lid = op->externalPartMask ? -1 : 0;
    if (ep != lid) {
    int diff = ep - lid;
    int dir;
    if (diff < 0) {
        dir = -1;
        diff = -diff;
    } else
        dir = 1;
    int mul;
    switch (in->addressAliasMode) {
    case 1:
        mul = 1;
        break;
    case 2:
        mul = -1;
        break;
    case 3:
        reportError("well - I'll code this one later\n");
        exitAfterCleanup(-1);
        return false;
    default:
        reportError("bad opchar mode %d\n", in->addressAliasMode);
        exitAfterCleanup(-1);
        return false;
    }
    if (dir * mul < 0)
        diff = -diff;
    int r = maddImmediate(&in->operands[in->addressAliasOperandIndex], &in->addressImmediateValue, diff);
    if (r < 0) {
        reportError("failed to madd immediates\n");
        in->leDump(in->addressAliasOperandIndex);
        exitAfterCleanup(-1);
    }
    if (r == 1) {
        reportError("ran out of bits for immfixup\n");
        return false;
    }
    }
    return true;
}

bool Init_immFixup(instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt) {
        reportError("Init_immFixup - failed to get work table\n");
        return false;
    }
    s->streamSerial = 0;
    for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
        initImm(in);
    return true;
}

void Instr::hasImmValue()
{
    immValid(&addressImmediateValue);
}

bool Instr::hasImmFixup()
{
    if (addressAliasMode != 0 || addressAliasDirection != 0 || addressAliasOperandIndex != -1 || addressImmediateValue.isValid())
        return true;
    return false;
}

void Instr::resetImmFixup()
{
    addressAliasMode = 0;
    addressAliasDirection = 0;
    addressAliasOperandIndex = -1;
    addressImmediateValue.clear();
}

extern const char parseImmediateError[] = "failed to parse immediate '%s'\n";
extern const char evalImmediateError[] = "failed to evaluate tree for '%s' to type %s\n";
extern const char parseImmediateWarn[] = "encountered an error parsing '%s'\n";
extern const char maddImmediateInputError[] = "madd immediate- input not valid\n";
