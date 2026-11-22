#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(workTable *, int *);

struct optimiser {
    const char *name;
    int wmode;
    int flags;
    wrangleFunc func;
    int ownerStream;
};

extern optimiser optTable[11];
extern const char *currentOptimizerName;

instStream *selectUninsertedStream(Program *p);
bool prepExtInfo(Program *p, instStream *s, bool b);
workTable *prepUta(Program *p, instStream *s, int loopSize, char smart);
bool statReset(Program *p);
bool dynCalcStats(workTable *wt);
bool scheduleMidTableMode1(workTable *wt, wrangleFunc f);
bool scheduleMidTableMode0(workTable *wt, wrangleFunc f);
bool reduceExtents(workTable *wt);
bool fiddleBDL(workTable *wt);
bool assignLocalRegisters(Program *p, workTable *wt);
bool makeLpWork(instStream *s, int maxSpread);
bool disposeLpWork(instStream *s);
bool keepLpWork(instStream *s);
bool optReset(Program *p);
workTable *makeForceTable(Program *p, instStream *s);
void dumpProgramStreamsAndRegisters(Program *p, int a);
const char *stallReasonStr(short why);
bool sredTable(workTable *wt);
bool prepareReductionCandidates(Program *p, instStream *s, workTable *wt);
bool reduceLinearSchedule(Program *p, workTable *wt, unsigned char a, unsigned char b);

bool workTable::qRegRef(regUse *ru)
{
    if (!ru) {
        reportError("qRegRef: passed a null pointer\n");
        exitAfterCleanup(-1);
    }
    regUse *c = ru->Clone();
    if (!c) {
        reportError("failed to clone regref\n");
        exitAfterCleanup(-1);
    }
    RegPoolEntry *reg = c->registerEntry;
    if (!reg) {
        reportError("no register attached to cloned rf during qRegRef\n");
        delete c;
        return false;
    }
    if (!registerUses.append(c)) {
        reportError("failed to add cloned regref to list\n");
        delete c;
        return false;
    }
    if (~c->outputMask & c->externalOutputMask) {
        reportError("xobits set, but o bits not\n");
        c->validateAndDumpParts();
        return false;
    }
    if (~c->inputMask & c->externalInputMask) {
        reportError("xibits set, but i bits not\n");
        c->validateAndDumpParts();
        return false;
    }
    for (int i = 0; i != reg->partCount; i++) {
        c->slots[i].earliestExternalCycle = ru->slots[i].earliestExternalCycle;
        c->slots[i].latestExternalCycle = ru->slots[i].latestExternalCycle;
        RegisterPartLockList *ll = getLockList(reg, i);
        if (!ll) {
            reportError("failed to get lock list ptr in register for rfv for reg %s\n", reg->registerName());
            exitAfterCleanup(-1);
        }
        if (c->slots[i].assignmentPayload) {
            reportError("rll is already set in rrp\n");
            exitAfterCleanup(-1);
        }
        c->slots[i].assignmentPayload = (VclPtrWord)ll;
        if (reg->getPartPayload(i)) {
            reportError("regref register part already had locklist\n");
            exitAfterCleanup(-1);
        }
        if (!reg->setPartPayload(i, (VclPtrWord)ll)) {
            reportError("failed to set curr lock list pointer in rf register\n");
            exitAfterCleanup(-1);
        }
        if ((c->outputMask >> i) & 1)
            ll->calculateLoopStats = 0;
        char xi = (c->externalInputMask >> i) & 1;
        char t = (c->throughputMask >> i) & 1;
        bool sync = false;
        if (reg->tracksHotOutputPaths && activeLoopSize == 0)
            sync = true;
        char nb = reg->nonBlockId;
        if (!nb && (c->throughputMask & (c->outputMask | c->externalOutputMask))) {
            reportError("t bits set for nbid register\n");
            c->validateAndDumpParts();
            return false;
        }
        if (t || xi || sync) {
            unsigned char nbz = nb == 0;
            OperandLock *lk = c->slots[i].getRegisterLock();
            if (!lk)
                reportError("failed to get lock for %s.%d as %s\n",
                              reg->registerName(), i, reg->registerName());
            if (!lk->setOwner(i, c)) {
                reportError("falied to set owner\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (!ll->addOutput(lk, 0, 1, nbz)) {
                reportError("failed to register output lock for regref\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (t) {
                bool ext = !(((c->outputMask | c->inputMask) >> i) & 1);
                if (!ll->addExtOutput(lk, ext)) {
                    reportError("failed to add pass thru reg ref as ext output for register %s\n",
                                  reg->registerName());
                    return false;
                }
                dump();
            }
            if (!ll->addOutputLock(lk)) {
                reportError("regRef::LoadLocks - failed to add output lock\n");
                return false;
            }
        }
    }
    return true;
}

bool setPipeBits(Instr *in);
bool AddToDo_Tail(Block *blk, Instr *n);
extern int nextReservedInstructionKey;

static inline bool isAAOperand(Operand *op)
{
    return op->operandStatus == 2;
}

static inline unsigned char operandLockMode(Operand *op)
{
    return op->operandStatus;
}

bool workTable::qInstruction(Instr *in)
{
    int i;
    if (!in->isNonSynthetic()) {
        reportError("tried to queue a synthetic\n");
        return false;
    }
    in->isNop();
    Instr *c = in->clone();
    if (!c) {
        reportError("failed to clone instruction\n");
        exitAfterCleanup(-1);
    }
    c->copyExternalTimingConstraints(in);
    if (!ownerStream) {
        reportError("qInstruction - no owner\n");
        return false;
    }
    if (c->addressAliasDirection == 1 || c->addressAliasDirection == 2) {
        if (c->addressAliasOperandIndex < 0) {
            reportError("m_aaOp<0 but dstMode is set!\n");
            exitAfterCleanup(-1);
        }
        c->operands[c->addressAliasOperandIndex].operandStatus = 2;
    }
    setPipeBits(c);

    for (i = 0; i != c->operandCount; i++) {
        Operand *src = in->getOp(i);
        Operand *dst = c->getOp(i);
        if (!src || !dst) {
            reportError("qInstruction failed to get an operand\n");
            return false;
        }
        RegPoolEntry *reg = dst->assignedRegister;
        if (!reg)
            continue;
        regUse *rr = registerUses.findByRegister(reg);
        if (!rr) {
            reportError("find reg ref failed to find rr from owner.. by now - this is fatal.. register %s\n",
                          reg->registerName());
            exitAfterCleanup(-1);
        }
        if (isRegisterOperand(dst, 1)) {
            if (isAAOperand(dst))
                dst->direction = 3;
        }
        if (isRegisterOperand(dst, 1)) {
            if (activeLoopSize && isVuRegister(reg) && dst->operandStatus != 2)
                dst->operandStatus = 1;
            for (int k = 0; k != reg->partCount; k++) {
                if ((dst->componentMask >> k) & 1) {
                OperandLock *lk = &dst->parts[k];
                int a = 0;
                RegisterPartLockList *ll = (RegisterPartLockList *)reg->getPartPayload(k);
                if (!ll) {
                    reportError("failed to get curr lock list during qinst\n");
                    exitAfterCleanup(-1);
                }
                lk->setOwner(k, c, i);
                OperandLock *out;
                if ((dst->externalPartMask >> k) & 1) {
                    regUse *o = registerUses.findByRegister(reg);
                    if (!o) {
                        reportError("failed to find regref owner for input\n");
                        return false;
                    }
                    out = o->slots[k].getRegisterLock();
                } else {
                    olList *uses = &src->parts[k].operandLocations;
                    ol_s *ol = uses->head;
                    if (!ol || ol->next) {
                        reportError("more or less than one oploc for non-extern input\n");
                        exitAfterCleanup(-1);
                    }
                    Instr *setter = ol->instruction;
                    if (!setter) {
                        reportError("could not get setting op for setter\n");
                        exitAfterCleanup(-1);
                    }
                    Instr *img = setter->cloneImage;
                    if (!img) {
                        reportError("qInstruction - setter had null work clone set\n");
                        return false;
                    }
                    if ((workTable *)img->ownerTable != this) {
                        reportError("qInstruction  - image of setter wasn't in this table\n");
                        return false;
                    }
                    Operand *sop = img->getOp(ol->operandIndex);
                    if (!sop) {
                        reportError("failed to get setter operand\n");
                        return false;
                    }
                    out = &sop->parts[k];
                    a = dst->additionalLatency;
                }
                lk->operandStatusCode = operandLockMode(dst);
                if (!ll->addInput(out, lk, a, dst->externalBlockModeFlag)) {
                    reportError("failed to register input operand\n");
                    return false;
                }
                }
            }
        }
        if (isRegisterOperand(dst, 2) && dst->operandStatus != 2 && activeLoopSize && isVuRegister(reg) && !rr->externalOutputMask)
            dst->operandStatus = 1;
    }

    for (i = 0; i != c->operandCount; i++) {
        Operand *op = c->getOp(i);
        RegPoolEntry *reg = registerOperand(op, 2);
        if (!reg)
            continue;
        for (int k = 0; k != reg->partCount; k++) {
            if ((op->componentMask >> k) & 1) {
                RegisterPartLockList *ll = (RegisterPartLockList *)reg->getPartPayload(k);
                if (!ll) {
                    reportError("couldn't get rll from reg\n");
                    exitAfterCleanup(-1);
                }
                OperandLock *lk = &op->parts[k];
                lk->setOwner(k, c, i);
                int a = 1;
                if (reg->tracksHotOutputPaths)
                    a = c->baseIssueSpacing;
                if (!ll->addOutput(lk, a, op->outputLatency, op->externalBlockModeFlag)) {
                    reportError("failed to register output operand : %s.%d\n", reg->registerName(), k);
                    return false;
                }
                if ((op->externalPartMask >> k) & 1) {
                    RegisterPartLockList *el = (RegisterPartLockList *)reg->getPartPayload(k);
                    if (!el)
                        reportError("couldn't get curr lock list for output\n");
                    if (!el->addExtOutput(lk, false)) {
                        reportError("failed to add ext output!\n");
                        return false;
                    }
                }
            }
        }
    }

    for (i = 0; i != c->operandCount; i++) {
        Operand *op = c->getOp(i);
        RegPoolEntry *reg = registerOperand(op, 1);
        if (!reg)
            continue;
        Operand *o2;
        for (int j = 0; (o2 = c->getOp(j)) != 0; j++) {
            if (registerOperand(o2, 2) != reg)
                continue;
            int k = 0;
            if (k != reg->partCount) {
                OpSlot *outputLock = o2->parts;
                OpSlot *inputLock = op->parts;
                do {
                    if (((op->componentMask & o2->componentMask) >> k) & 1) {
                        if (!inputLock->setInOut(outputLock)) {
                            reportError("failed to set in out\n");
                            exitAfterCleanup(-1);
                            return false;
                        }
                    }
                    k++;
                    outputLock++;
                    inputLock++;
                } while (k != reg->partCount);
            }
        }
    }

    if (!AddToDo_Tail((Block *)this, c)) {
        reportError("iTable::qInstruction - failed to add to todo tail\n");
        delete c;
        return false;
    }
    if (!logAddLinear_Tail(c)) {
        reportError("iTable::qInstruction - failed to add to linear list tail\n");
        return false;
    }
    if (loopSize && c->branchFlags) {
        nextReservedInstructionKey += 2;
        c->replacedInstructionWord = nextReservedInstructionKey;
        reserveEntry(nextReservedInstructionKey, loopSize - 2, 1);
    }
    return true;
}

