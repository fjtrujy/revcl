

#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "VUStr.h"

bool initPrune(InstructionStream *s);
bool initPrune(Instr *in);
int tryPrune(InstructionStream *s);
int tryPrune(Instr *in, bool raw);
bool doPrune(InstructionStream *s);
int doPrune(Instr *in);
int hackIOmask(Instr *in, int outOp, unsigned char mask, int inOp);

static inline bool initPrune_p(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!initPrune(stream)) {
            reportError("failed to init prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

static inline int tryPrune_p(Program *program)
{
    int successfulPruneCount = 0;
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        int pruneResult = tryPrune(stream);
        if (pruneResult < 0) {
            reportError("flowMon: failed to  tryPrune %s\n", stream->streamName());
            return -1;
        }
        successfulPruneCount += pruneResult;
    }
    return successfulPruneCount;
}

static inline bool doPrune_p(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!doPrune(stream)) {
            reportError("failed to do prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

bool pruneProgramOperands(Program *program)
{
    if (!initPrune_p(program)) {
        reportError("failed to init prune!\n");
        return false;
    }
    int prunePassCount;
    do {
        prunePassCount = tryPrune_p(program);
        if (prunePassCount < 0) {
            reportError("flowMon::Prune - an error occured\n");
            exitAfterCleanup(-1);
        }
    } while (prunePassCount);
    if (!doPrune_p(program)) {
        reportError("failed to do prune!\n");
        return false;
    }
    return true;
}

bool initPrune(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!initPrune(stream)) {
            reportError("failed to init prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

bool initPrune(InstructionStream *stream)
{
    ScheduleTable *scheduleTable = stream->getTable();
    if (!scheduleTable) {
        reportError("failed to get work table\n");
        return false;
    }
    for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        if (!initPrune(instruction)) {
            reportError("init prune failed\n");
            return false;
        }
    return true;
}

bool initPrune(Instr *instruction)
{
    instruction->pruningRestricted = 0;
    if (instruction->opcodeInfo) {
        char canPrune = instruction->opcodeInfo->canPruneOperands();
        if (!canPrune)
            instruction->pruningRestricted = 1;
        for (int operandIndex = 0;; operandIndex++) {
            Operand *operand = instruction->getOp(operandIndex);
            if (!operand)
                break;
            operand->pruneMask = 0;
            if (operand->isInput() && instruction->pruningRestricted)
                operand->pruneMask = operand->componentMask;
        }
        return true;
    }
    reportError("found inst with no m_Inst\n");
    return false;
}

int tryPrune(Program *program)
{
    int successfulPruneCount = 0;
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        int pruneResult = tryPrune(stream);
        if (pruneResult < 0) {
            reportError("flowMon: failed to  tryPrune %s\n", stream->streamName());
            return -1;
        }
        successfulPruneCount += pruneResult;
    }
    return successfulPruneCount;
}

static inline void dumpPruneInstruction(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

int tryPrune(InstructionStream *stream)
{
    int successfulPruneCount = 0;
    ScheduleTable *scheduleTable = stream->getTable();
    if (!scheduleTable) {
        reportError("failed to get work table\n");
        return 0;
    }
    bool raw = false;
    if (scheduleTable->codeModeValue == 4 || scheduleTable->getTableRawMode() == 4)
        raw = true;
    for (Instr *currentInstruction = scheduleTable->linearInstructions.head; currentInstruction; currentInstruction = currentInstruction->linearLink.nextInstruction) {
        int pruneResult = tryPrune(currentInstruction, raw);
        if (pruneResult < 0) {
            reportError("rec  prune failed for instruction\n");
            dumpPruneInstruction(currentInstruction);
            return -1;
        }
        successfulPruneCount += pruneResult;
    }
    return successfulPruneCount;
}

int tryPrune(Instr *instruction, bool raw)
{
    int pruneCount = 0;
    if (!instruction->opcodeInfo)
        return 0;
    if (raw) {
        Operand *registerOperand, *candidateOperand, *lookup;
        for (int operandIndex = 0;
             ((operandIndex < 0 ? (lookup = instruction->getOp(operandIndex)) :
                       (lookup = operandIndex < instruction->operandCount ? &instruction->operands[operandIndex] : 0)),
              registerOperand = lookup, candidateOperand = lookup, lookup = 0, candidateOperand) != 0; operandIndex++) {
            if (registerOperand->isInput())
                (RegisterEntry *&)registerOperand = registerOperand->assignedRegister;
            else
                (RegisterEntry *&)registerOperand = 0;
            if (registerOperand)
                candidateOperand->pruneMask = candidateOperand->componentMask;
        }
    }
    Operand *out;
    for (int outputOperandIndex = 0; (out = instruction->getOp(outputOperandIndex)) != 0; outputOperandIndex++) {
        int isOut = out->direction == 2;
        RegisterEntry *reg = isOut ? out->assignedRegister : 0;
        if (!reg)
            continue;
        int cnt = 0;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if ((out->componentMask >> partIndex) & 1)
            for (OperandLocation *location = out->parts[partIndex].operandLocations.head; location; location = location->next) {
                Operand *sourceOperand = operandAtLocation(location);
                if (!sourceOperand) {
                    reportError("failed to get an operand from an oploc\n");
                    return -1;
                }
                int componentIndex = location->componentIndex;
                if (sourceOperand->assignedRegister != reg) {
                    reportError("there was a reg mismatch\n");
                    return -1;
                }
                if (componentIndex != partIndex) {
                    reportError("swizzle detected\n");
                    return -1;
                }
                if (!((sourceOperand->componentMask >> partIndex) & 1)) {
                    reportError("there was a mop bit mismatch\n");
                    return -1;
                }
                if (((sourceOperand->pruneMask >> partIndex) & 1) && ((~out->pruneMask >> partIndex) & 1)) {
                    cnt++;
                    pruneCount++;
                    out->pruneMask |= 1 << partIndex;
                }
            }
        }
        if (!cnt)
            continue;
        if (out->operandKind == 10 || out->operandKind == 11)
            out->pruneMask = out->componentMask;
        Operand *inputOperand;
        for (int inputOperandIndex = 0; (inputOperand = instruction->getOp(inputOperandIndex)) != 0; inputOperandIndex++) {
            int isIn = inputOperand->direction == 1;
            RegisterEntry *registerEntry = isIn ? inputOperand->assignedRegister : 0;
            if (!registerEntry)
                continue;
            int bits = hackIOmask(instruction, outputOperandIndex, out->pruneMask, inputOperandIndex);
            if (bits < 0) {
                reportError("lprune: failed to deduce which inputs affected which outputs\n");
                return -1;
            }
            if ((inputOperand->componentMask & bits) != bits) {
                reportError("bits were used.. but outside of mod bit mask!\n");
                reportError("OUTPUT(%d) INPUT(%d) inst : \n", outputOperandIndex, inputOperandIndex);
                reportError("A_BITS=%x .. INPUT_MODBITS=%x : \n", bits, inputOperand->componentMask);
                reportError("otype is opt->m_oType=%d\n", inputOperand->operandKind);
                dumpPruneInstruction(instruction);
                return -1;
            }
            inputOperand->pruneMask |= bits;
        }
    }
    return pruneCount;
}

bool doPrune(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!doPrune(stream)) {
            reportError("failed to do prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

extern int errorOccurred;
extern bool quietDiagnostics;

bool doPrune(InstructionStream *stream)
{
    ScheduleTable *scheduleTable = stream->getTable();
    if (!scheduleTable) {
        reportError("failed to get work table\n");
        return false;
    }
    if (scheduleTable->getTableRawMode() == 4 || scheduleTable->getTableRawMode() == 3)
        return true;
    Instr *first = scheduleTable->linearInstructions.head;
    Instr *currentInstruction = first;
    Instr *next;
    if (first) do {
        next = currentInstruction->linearLink.nextInstruction;
        int pruneResult = doPrune(currentInstruction);
        if (pruneResult < 0) {
            reportError("do  prune failed for instruction\n");
            dumpPruneInstruction(currentInstruction);
            return false;
        }
        if (pruneResult == 0) {
            if (!currentInstruction->isNop()) {
                reportVerboseInfo("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = quietDiagnostics;
                currentInstruction->dumpInstructionContext();
                errorOccurred = saved;
            }
            if (!scheduleTable->destroyInstruction(currentInstruction)) {
                reportError("failed to destroy instruction\n");
                if (currentInstruction)
                    dumpPruneInstruction(currentInstruction);
                return false;
            }
        }
        currentInstruction = next;
    } while (currentInstruction);
    return true;
}

bool rotateInputComponentMask(void *, Instr *in);

int doPrune(Instr *prunedInstruction)
{
    int operandIndex;
    Operand *currentOperand;
    unsigned char any = 0;
    if (prunedInstruction->pruningRestricted)
        return 1;
    for (operandIndex = 0; (currentOperand = prunedInstruction->getOp(operandIndex)) != 0; operandIndex++) {
        any |= currentOperand->pruneMask;
    }
    if (!any)
        return 0;
    OpcodeDescriptor *opcodeInfo = prunedInstruction->opcodeInfo;
    if (!opcodeInfo) {
        reportError("Instruction::doPrune() - had no iInfo\n");
        return -1;
    }
    if (prunedInstruction->abstractMoveOpcode && prunedInstruction->operands[0].assignedRegister == prunedInstruction->operands[1].assignedRegister)
        return 0;
    if (opcodeInfo->destinationMaskMode == 1) {
        int effectiveDestinationMask, nIn = 0, inOR = 0, outOR = 0, nOut = 0;
        for (operandIndex = 0; (currentOperand = prunedInstruction->getOp(operandIndex)) != 0; operandIndex++) {
            if (currentOperand->operandKind != 1 && currentOperand->operandKind != 9)
                continue;
            if (!currentOperand->assignedRegister) {
                reportError("couldn't get a register for V_FL4 or V_ACC operand\n");
                return -1;
            }
            if (currentOperand->isInput()) {
                inOR |= currentOperand->pruneMask;
                nIn++;
            }
            if (currentOperand->isOutput()) {
                if (nOut && currentOperand->pruneMask != outOR) {
                    reportError("prune - found two inputs with different pBits state for FL4/ACC\n");
                    return -1;
                }
                outOR |= currentOperand->pruneMask;
                nOut++;
            }
        }
        if (!nOut && !nIn) {
            reportError("MD_ANY set - but couldn't find any fl4 or acc operands for %s\n", opcodeInfo->mnemonic);
            return -1;
        }
        if (nOut > 1) {
            reportError("nOut >2! for fl4/acc\n ");
            prunedInstruction->dumpInstructionContext();
            return -1;
        }
        if (opcodeInfo->decodeHook == rotateInputComponentMask)
            inOR = (inOR >> 1) | ((inOR & 1) << 3);
        if (nIn) {
            if (outOR & ~inOR) {
                reportError("doPrune:: output had larger extent than input.. out=%x vs in=%x\n", outOR, inOR);
                prunedInstruction->dumpInstructionContext();
                return -1;
            }
            effectiveDestinationMask = inOR;
        } else if (!outOR)
            effectiveDestinationMask = prunedInstruction->destinationMask;
        else
            effectiveDestinationMask = outOR;
        if (effectiveDestinationMask & ~prunedInstruction->destinationMask) {
            reportError("inOR was out of range of instruction mod bits nb=%x,mb=%x!\n", effectiveDestinationMask, prunedInstruction->destinationMask);
            dumpPruneInstruction(prunedInstruction);
            return -1;
        }
        if (effectiveDestinationMask != prunedInstruction->destinationMask) {
            reportVerboseInfo("PRUNE FIELD: [%s->%s]  ", formatComponentMaskLetters(prunedInstruction->destinationMask), formatComponentMaskLetters(effectiveDestinationMask));
            int saved = errorOccurred;
            errorOccurred = quietDiagnostics;
            prunedInstruction->dumpInstructionContext();
            errorOccurred = saved;
        }
        prunedInstruction->destinationMask = effectiveDestinationMask;
        for (operandIndex = 0; (currentOperand = prunedInstruction->getOp(operandIndex)) != 0; operandIndex++) {
            if (currentOperand->operandKind == 1 || currentOperand->operandKind == 9)
                currentOperand->componentMask = effectiveDestinationMask;
        }
        if (opcodeInfo->decodeHook == rotateInputComponentMask)
            prunedInstruction->operands[1].componentMask = (((effectiveDestinationMask & 8) >> 3) | (effectiveDestinationMask << 1)) & 0xf;
    }
    for (operandIndex = 0; (currentOperand = prunedInstruction->getOp(operandIndex)) != 0; operandIndex++) {
        RegisterEntry *registerEntry = registerOperand(currentOperand, 2);
        if (!registerEntry || currentOperand->pruneMask || !registerEntry->isVirtual)
            continue;
        RegPool *pool = registerEntry->ownerPool;
        if (!pool) {
            reportError("register had no regpool\n");
            return -1;
        }
        if (pool->zeroRegister && !currentOperand->pairedOperandIndexPlusOne) {
            reportVerboseInfo("ZERO FIX   : ");
            int saved = errorOccurred;
            errorOccurred = quietDiagnostics;
            prunedInstruction->dumpInstructionContext(operandIndex);
            errorOccurred = saved;
            currentOperand->assignedRegister = pool->zeroRegister;
        }
    }
    return 1;
}

