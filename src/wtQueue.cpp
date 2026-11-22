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

bool workTable::qRegRef(regUse *registerUse)
{
    if (!registerUse) {
        reportError("qRegRef: passed a null pointer\n");
        exitAfterCleanup(-1);
    }
    regUse *c = registerUse->Clone();
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
    for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
        c->slots[partIndex].earliestExternalCycle = registerUse->slots[partIndex].earliestExternalCycle;
        c->slots[partIndex].latestExternalCycle = registerUse->slots[partIndex].latestExternalCycle;
        RegisterPartLockList *lockList = getLockList(reg, partIndex);
        if (!lockList) {
            reportError("failed to get lock list ptr in register for rfv for reg %s\n", reg->registerName());
            exitAfterCleanup(-1);
        }
        if (c->slots[partIndex].assignmentPayload) {
            reportError("rll is already set in rrp\n");
            exitAfterCleanup(-1);
        }
        c->slots[partIndex].assignmentPayload = (VclPtrWord)lockList;
        if (reg->getPartPayload(partIndex)) {
            reportError("regref register part already had locklist\n");
            exitAfterCleanup(-1);
        }
        if (!reg->setPartPayload(partIndex, (VclPtrWord)lockList)) {
            reportError("failed to set curr lock list pointer in rf register\n");
            exitAfterCleanup(-1);
        }
        if ((c->outputMask >> partIndex) & 1)
            lockList->calculateLoopStats = 0;
        char xi = (c->externalInputMask >> partIndex) & 1;
        char t = (c->throughputMask >> partIndex) & 1;
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
            OperandLock *lock = c->slots[partIndex].getRegisterLock();
            if (!lock)
                reportError("failed to get lock for %s.%d as %s\n",
                              reg->registerName(), partIndex, reg->registerName());
            if (!lock->setOwner(partIndex, c)) {
                reportError("falied to set owner\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (!lockList->addOutput(lock, 0, 1, nbz)) {
                reportError("failed to register output lock for regref\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (t) {
                bool ext = !(((c->outputMask | c->inputMask) >> partIndex) & 1);
                if (!lockList->addExtOutput(lock, ext)) {
                    reportError("failed to add pass thru reg ref as ext output for register %s\n",
                                  reg->registerName());
                    return false;
                }
                dump();
            }
            if (!lockList->addOutputLock(lock)) {
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

static inline bool isAAOperand(Operand *operand)
{
    return operand->operandStatus == 2;
}

static inline unsigned char operandLockMode(Operand *operand)
{
    return operand->operandStatus;
}

bool workTable::qInstruction(Instr *instruction)
{
    int i;
    if (!instruction->isNonSynthetic()) {
        reportError("tried to queue a synthetic\n");
        return false;
    }
    instruction->isNop();
    Instr *cloneInstruction = instruction->clone();
    if (!cloneInstruction) {
        reportError("failed to clone instruction\n");
        exitAfterCleanup(-1);
    }
    cloneInstruction->copyExternalTimingConstraints(instruction);
    if (!ownerStream) {
        reportError("qInstruction - no owner\n");
        return false;
    }
    if (cloneInstruction->addressAliasDirection == 1 || cloneInstruction->addressAliasDirection == 2) {
        if (cloneInstruction->addressAliasOperandIndex < 0) {
            reportError("m_aaOp<0 but dstMode is set!\n");
            exitAfterCleanup(-1);
        }
        cloneInstruction->operands[cloneInstruction->addressAliasOperandIndex].operandStatus = 2;
    }
    setPipeBits(cloneInstruction);

    for (i = 0; i != cloneInstruction->operandCount; i++) {
        Operand *src = instruction->getOp(i);
        Operand *dst = cloneInstruction->getOp(i);
        if (!src || !dst) {
            reportError("qInstruction failed to get an operand\n");
            return false;
        }
        RegPoolEntry *reg = dst->assignedRegister;
        if (!reg)
            continue;
        regUse *registerUse = registerUses.findByRegister(reg);
        if (!registerUse) {
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
            for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
                if ((dst->componentMask >> partIndex) & 1) {
                OperandLock *lock = &dst->parts[partIndex];
                int a = 0;
                RegisterPartLockList *lockList = (RegisterPartLockList *)reg->getPartPayload(partIndex);
                if (!lockList) {
                    reportError("failed to get curr lock list during qinst\n");
                    exitAfterCleanup(-1);
                }
                lock->setOwner(partIndex, cloneInstruction, i);
                OperandLock *out;
                if ((dst->externalPartMask >> partIndex) & 1) {
                    regUse *o = registerUses.findByRegister(reg);
                    if (!o) {
                        reportError("failed to find regref owner for input\n");
                        return false;
                    }
                    out = o->slots[partIndex].getRegisterLock();
                } else {
                    olList *uses = &src->parts[partIndex].operandLocations;
                    ol_s *operandLocation = uses->head;
                    if (!operandLocation || operandLocation->next) {
                        reportError("more or less than one oploc for non-extern input\n");
                        exitAfterCleanup(-1);
                    }
                    Instr *setter = operandLocation->instruction;
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
                    Operand *sop = img->getOp(operandLocation->operandIndex);
                    if (!sop) {
                        reportError("failed to get setter operand\n");
                        return false;
                    }
                    out = &sop->parts[partIndex];
                    a = dst->additionalLatency;
                }
                lock->operandStatusCode = operandLockMode(dst);
                if (!lockList->addInput(out, lock, a, dst->externalBlockModeFlag)) {
                    reportError("failed to register input operand\n");
                    return false;
                }
                }
            }
        }
        if (isRegisterOperand(dst, 2) && dst->operandStatus != 2 && activeLoopSize && isVuRegister(reg) && !registerUse->externalOutputMask)
            dst->operandStatus = 1;
    }

    for (i = 0; i != cloneInstruction->operandCount; i++) {
        Operand *operand = cloneInstruction->getOp(i);
        RegPoolEntry *reg = registerOperand(operand, 2);
        if (!reg)
            continue;
        for (int k = 0; k != reg->partCount; k++) {
            if ((operand->componentMask >> k) & 1) {
                RegisterPartLockList *lockList = (RegisterPartLockList *)reg->getPartPayload(k);
                if (!lockList) {
                    reportError("couldn't get rll from reg\n");
                    exitAfterCleanup(-1);
                }
                OperandLock *lock = &operand->parts[k];
                lock->setOwner(k, cloneInstruction, i);
                int a = 1;
                if (reg->tracksHotOutputPaths)
                    a = cloneInstruction->baseIssueSpacing;
                if (!lockList->addOutput(lock, a, operand->outputLatency, operand->externalBlockModeFlag)) {
                    reportError("failed to register output operand : %s.%d\n", reg->registerName(), k);
                    return false;
                }
                if ((operand->externalPartMask >> k) & 1) {
                    RegisterPartLockList *registerPartLocks = (RegisterPartLockList *)reg->getPartPayload(k);
                    if (!registerPartLocks)
                        reportError("couldn't get curr lock list for output\n");
                    if (!registerPartLocks->addExtOutput(lock, false)) {
                        reportError("failed to add ext output!\n");
                        return false;
                    }
                }
            }
        }
    }

    for (i = 0; i != cloneInstruction->operandCount; i++) {
        Operand *operand = cloneInstruction->getOp(i);
        RegPoolEntry *reg = registerOperand(operand, 1);
        if (!reg)
            continue;
        Operand *o2;
        for (int j = 0; (o2 = cloneInstruction->getOp(j)) != 0; j++) {
            if (registerOperand(o2, 2) != reg)
                continue;
            int k = 0;
            if (k != reg->partCount) {
                OpSlot *outputLock = o2->parts;
                OpSlot *inputLock = operand->parts;
                do {
                    if (((operand->componentMask & o2->componentMask) >> k) & 1) {
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

    if (!AddToDo_Tail((Block *)this, cloneInstruction)) {
        reportError("iTable::qInstruction - failed to add to todo tail\n");
        delete cloneInstruction;
        return false;
    }
    if (!logAddLinear_Tail(cloneInstruction)) {
        reportError("iTable::qInstruction - failed to add to linear list tail\n");
        return false;
    }
    if (loopSize && cloneInstruction->branchFlags) {
        nextReservedInstructionKey += 2;
        cloneInstruction->replacedInstructionWord = nextReservedInstructionKey;
        reserveEntry(nextReservedInstructionKey, loopSize - 2, 1);
    }
    return true;
}

