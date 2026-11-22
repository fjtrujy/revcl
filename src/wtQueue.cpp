#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(ScheduleTable *, int *);

struct optimiser {
    const char *name;
    int wmode;
    int flags;
    wrangleFunc func;
    int ownerStream;
};

extern optimiser optTable[11];
extern const char *currentOptimizerName;

InstructionStream *selectUninsertedStream(Program *p);
bool prepExtInfo(Program *p, InstructionStream *s, bool b);
ScheduleTable *prepUta(Program *p, InstructionStream *s, int loopSize, char smart);
bool statReset(Program *p);
bool dynCalcStats(ScheduleTable *wt);
bool scheduleMidTableMode1(ScheduleTable *wt, wrangleFunc f);
bool scheduleMidTableMode0(ScheduleTable *wt, wrangleFunc f);
bool reduceExtents(ScheduleTable *wt);
bool adjustTrailingBranchLayout(ScheduleTable *wt);
bool assignLocalRegisters(Program *p, ScheduleTable *wt);
bool makeLpWork(InstructionStream *s, int maxSpread);
bool disposeLpWork(InstructionStream *s);
bool keepLpWork(InstructionStream *s);
bool optReset(Program *p);
ScheduleTable *makeForceTable(Program *p, InstructionStream *s);
void dumpProgramStreamsAndRegisters(Program *p, int a);
const char *stallReasonStr(short why);
bool sredTable(ScheduleTable *wt);
bool prepareReductionCandidates(Program *p, InstructionStream *s, ScheduleTable *wt);
bool reduceLinearSchedule(Program *p, ScheduleTable *wt, unsigned char a, unsigned char b);

bool ScheduleTable::queueRegisterUse(RegisterUse *registerUse)
{
    if (!registerUse) {
        reportError("qRegRef: passed a null pointer\n");
        exitAfterCleanup(-1);
    }
    RegisterUse *clonedRegisterUse = registerUse->Clone();
    if (!clonedRegisterUse) {
        reportError("failed to clone regref\n");
        exitAfterCleanup(-1);
    }
    RegisterEntry *registerEntry = clonedRegisterUse->registerEntry;
    if (!registerEntry) {
        reportError("no register attached to cloned rf during qRegRef\n");
        delete clonedRegisterUse;
        return false;
    }
    if (!registerUses.append(clonedRegisterUse)) {
        reportError("failed to add cloned regref to list\n");
        delete clonedRegisterUse;
        return false;
    }
    if (~clonedRegisterUse->outputMask & clonedRegisterUse->externalOutputMask) {
        reportError("xobits set, but o bits not\n");
        clonedRegisterUse->validateAndDumpParts();
        return false;
    }
    if (~clonedRegisterUse->inputMask & clonedRegisterUse->externalInputMask) {
        reportError("xibits set, but i bits not\n");
        clonedRegisterUse->validateAndDumpParts();
        return false;
    }
    for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
        clonedRegisterUse->slots[partIndex].earliestExternalCycle = registerUse->slots[partIndex].earliestExternalCycle;
        clonedRegisterUse->slots[partIndex].latestExternalCycle = registerUse->slots[partIndex].latestExternalCycle;
        RegisterPartLockList *lockList = getLockList(registerEntry, partIndex);
        if (!lockList) {
            reportError("failed to get lock list ptr in register for rfv for reg %s\n", registerEntry->registerName());
            exitAfterCleanup(-1);
        }
        if (clonedRegisterUse->slots[partIndex].assignmentPayload) {
            reportError("rll is already set in rrp\n");
            exitAfterCleanup(-1);
        }
        clonedRegisterUse->slots[partIndex].assignmentPayload = (VclPtrWord)lockList;
        if (registerEntry->getPartPayload(partIndex)) {
            reportError("regref register part already had locklist\n");
            exitAfterCleanup(-1);
        }
        if (!registerEntry->setPartPayload(partIndex, (VclPtrWord)lockList)) {
            reportError("failed to set curr lock list pointer in rf register\n");
            exitAfterCleanup(-1);
        }
        if ((clonedRegisterUse->outputMask >> partIndex) & 1)
            lockList->calculateLoopStats = 0;
        char hasExternalInput = (clonedRegisterUse->externalInputMask >> partIndex) & 1;
        char hasThroughput = (clonedRegisterUse->throughputMask >> partIndex) & 1;
        bool sync = false;
        if (registerEntry->tracksHotOutputPaths && activeLoopSize == 0)
            sync = true;
        char nonBlockingId = registerEntry->nonBlockId;
        if (!nonBlockingId && (clonedRegisterUse->throughputMask & (clonedRegisterUse->outputMask | clonedRegisterUse->externalOutputMask))) {
            reportError("t bits set for nbid register\n");
            clonedRegisterUse->validateAndDumpParts();
            return false;
        }
        if (hasThroughput || hasExternalInput || sync) {
            unsigned char nbz = nonBlockingId == 0;
            OperandLock *lock = clonedRegisterUse->slots[partIndex].getRegisterLock();
            if (!lock)
                reportError("failed to get lock for %s.%d as %s\n",
                              registerEntry->registerName(), partIndex, registerEntry->registerName());
            if (!lock->setOwner(partIndex, clonedRegisterUse)) {
                reportError("falied to set owner\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (!lockList->addOutput(lock, 0, 1, nbz)) {
                reportError("failed to register output lock for regref\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (hasThroughput) {
                bool ext = !(((clonedRegisterUse->outputMask | clonedRegisterUse->inputMask) >> partIndex) & 1);
                if (!lockList->addExtOutput(lock, ext)) {
                    reportError("failed to add pass thru reg ref as ext output for register %s\n",
                                  registerEntry->registerName());
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
bool appendPendingInstruction(Block *blk, Instr *n);
extern int nextReservedInstructionKey;

static inline bool isAAOperand(Operand *operand)
{
    return operand->operandStatus == 2;
}

static inline unsigned char operandLockMode(Operand *operand)
{
    return operand->operandStatus;
}

bool ScheduleTable::queueInstruction(Instr *instruction)
{
    int operandIndex;
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

    for (operandIndex = 0; operandIndex != cloneInstruction->operandCount; operandIndex++) {
        Operand *sourceOperand = instruction->getOp(operandIndex);
        Operand *destinationOperand = cloneInstruction->getOp(operandIndex);
        if (!sourceOperand || !destinationOperand) {
            reportError("qInstruction failed to get an operand\n");
            return false;
        }
        RegisterEntry *registerEntry = destinationOperand->assignedRegister;
        if (!registerEntry)
            continue;
        RegisterUse *registerUse = registerUses.findByRegister(registerEntry);
        if (!registerUse) {
            reportError("find reg ref failed to find rr from owner.. by now - this is fatal.. register %s\n",
                          registerEntry->registerName());
            exitAfterCleanup(-1);
        }
        if (isRegisterOperand(destinationOperand, 1)) {
            if (isAAOperand(destinationOperand))
                destinationOperand->direction = VCL_OPERAND_READ_WRITE;
        }
        if (isRegisterOperand(destinationOperand, 1)) {
            if (activeLoopSize && isVuRegister(registerEntry) && destinationOperand->operandStatus != 2)
                destinationOperand->operandStatus = 1;
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                if ((destinationOperand->componentMask >> partIndex) & 1) {
                OperandLock *lock = &destinationOperand->parts[partIndex];
                int inputLockMode = 0;
                RegisterPartLockList *lockList = (RegisterPartLockList *)registerEntry->getPartPayload(partIndex);
                if (!lockList) {
                    reportError("failed to get curr lock list during qinst\n");
                    exitAfterCleanup(-1);
                }
                lock->setOwner(partIndex, cloneInstruction, operandIndex);
                OperandLock *out;
                if ((destinationOperand->externalPartMask >> partIndex) & 1) {
                    RegisterUse *ownerRegisterUse = registerUses.findByRegister(registerEntry);
                    if (!ownerRegisterUse) {
                        reportError("failed to find regref owner for input\n");
                        return false;
                    }
                    out = ownerRegisterUse->slots[partIndex].getRegisterLock();
                } else {
                    olList *uses = &sourceOperand->parts[partIndex].operandLocations;
                    OperandLocation *operandLocation = uses->head;
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
                    if ((ScheduleTable *)img->ownerTable != this) {
                        reportError("qInstruction  - image of setter wasn't in this table\n");
                        return false;
                    }
                    Operand *sop = img->getOp(operandLocation->operandIndex);
                    if (!sop) {
                        reportError("failed to get setter operand\n");
                        return false;
                    }
                    out = &sop->parts[partIndex];
                    inputLockMode = destinationOperand->additionalLatency;
                }
                lock->operandStatusCode = operandLockMode(destinationOperand);
                if (!lockList->addInput(out, lock, inputLockMode, destinationOperand->externalBlockModeFlag)) {
                    reportError("failed to register input operand\n");
                    return false;
                }
                }
            }
        }
        if (isRegisterOperand(destinationOperand, 2) && destinationOperand->operandStatus != 2 && activeLoopSize && isVuRegister(registerEntry) && !registerUse->externalOutputMask)
            destinationOperand->operandStatus = 1;
    }

    for (operandIndex = 0; operandIndex != cloneInstruction->operandCount; operandIndex++) {
        Operand *operand = cloneInstruction->getOp(operandIndex);
        RegisterEntry *registerEntry = registerOperand(operand, 2);
        if (!registerEntry)
            continue;
        for (int outputPartIndex = 0; outputPartIndex != registerEntry->partCount; outputPartIndex++) {
            if ((operand->componentMask >> outputPartIndex) & 1) {
                RegisterPartLockList *lockList = (RegisterPartLockList *)registerEntry->getPartPayload(outputPartIndex);
                if (!lockList) {
                    reportError("couldn't get rll from reg\n");
                    exitAfterCleanup(-1);
                }
                OperandLock *lock = &operand->parts[outputPartIndex];
                lock->setOwner(outputPartIndex, cloneInstruction, operandIndex);
                int issueSpacing = 1;
                if (registerEntry->tracksHotOutputPaths)
                    issueSpacing = cloneInstruction->baseIssueSpacing;
                if (!lockList->addOutput(lock, issueSpacing, operand->outputLatency, operand->externalBlockModeFlag)) {
                    reportError("failed to register output operand : %s.%d\n", registerEntry->registerName(), outputPartIndex);
                    return false;
                }
                if ((operand->externalPartMask >> outputPartIndex) & 1) {
                    RegisterPartLockList *registerPartLocks = (RegisterPartLockList *)registerEntry->getPartPayload(outputPartIndex);
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

    for (operandIndex = 0; operandIndex != cloneInstruction->operandCount; operandIndex++) {
        Operand *operand = cloneInstruction->getOp(operandIndex);
        RegisterEntry *registerEntry = registerOperand(operand, 1);
        if (!registerEntry)
            continue;
        Operand *o2;
        for (int outputOperandIndex = 0; (o2 = cloneInstruction->getOp(outputOperandIndex)) != 0; outputOperandIndex++) {
            if (registerOperand(o2, 2) != registerEntry)
                continue;
            int pairedPartIndex = 0;
            if (pairedPartIndex != registerEntry->partCount) {
                OpSlot *outputLock = o2->parts;
                OpSlot *inputLock = operand->parts;
                do {
                    if (((operand->componentMask & o2->componentMask) >> pairedPartIndex) & 1) {
                        if (!inputLock->setInOut(outputLock)) {
                            reportError("failed to set in out\n");
                            exitAfterCleanup(-1);
                            return false;
                        }
                    }
                    pairedPartIndex++;
                    outputLock++;
                    inputLock++;
                } while (pairedPartIndex != registerEntry->partCount);
            }
        }
    }

    if (!appendPendingInstruction((Block *)this, cloneInstruction)) {
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

