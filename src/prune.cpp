

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
    int successfulPruneCount = 0;
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
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
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!initPrune(stream)) {
            reportError("failed to init prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

bool initPrune(instStream *stream)
{
    workTable *scheduleTable = stream->getTable();
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
        char ok = instruction->opcodeInfo->canPruneOperands();
        if (!ok)
            instruction->pruningRestricted = 1;
        for (int i = 0;; i++) {
            Operand *operand = instruction->getOp(i);
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
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
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

int tryPrune(instStream *stream)
{
    int successfulPruneCount = 0;
    workTable *scheduleTable = stream->getTable();
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
                (RegPoolEntry *&)registerOperand = registerOperand->assignedRegister;
            else
                (RegPoolEntry *&)registerOperand = 0;
            if (registerOperand)
                candidateOperand->pruneMask = candidateOperand->componentMask;
        }
    }
    Operand *out;
    for (int o = 0; (out = instruction->getOp(o)) != 0; o++) {
        int isOut = out->direction == 2;
        RegPoolEntry *reg = isOut ? out->assignedRegister : 0;
        if (!reg)
            continue;
        int cnt = 0;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if ((out->componentMask >> partIndex) & 1)
            for (ol_s *location = out->parts[partIndex].operandLocations.head; location; location = location->next) {
                Operand *src = operandAtLocation(location);
                if (!src) {
                    reportError("failed to get an operand from an oploc\n");
                    return -1;
                }
                int sw = location->componentIndex;
                if (src->assignedRegister != reg) {
                    reportError("there was a reg mismatch\n");
                    return -1;
                }
                if (sw != partIndex) {
                    reportError("swizzle detected\n");
                    return -1;
                }
                if (!((src->componentMask >> partIndex) & 1)) {
                    reportError("there was a mop bit mismatch\n");
                    return -1;
                }
                if (((src->pruneMask >> partIndex) & 1) && ((~out->pruneMask >> partIndex) & 1)) {
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
        for (int k = 0; (inputOperand = instruction->getOp(k)) != 0; k++) {
            int isIn = inputOperand->direction == 1;
            RegPoolEntry *registerEntry = isIn ? inputOperand->assignedRegister : 0;
            if (!registerEntry)
                continue;
            int bits = hackIOmask(instruction, o, out->pruneMask, k);
            if (bits < 0) {
                reportError("lprune: failed to deduce which inputs affected which outputs\n");
                return -1;
            }
            if ((inputOperand->componentMask & bits) != bits) {
                reportError("bits were used.. but outside of mod bit mask!\n");
                reportError("OUTPUT(%d) INPUT(%d) inst : \n", o, k);
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
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!doPrune(stream)) {
            reportError("failed to do prune for %s\n", stream->streamName());
            return false;
        }
    return true;
}

extern int errorOccurred;
extern bool quietDiagnostics;

bool doPrune(instStream *stream)
{
    workTable *scheduleTable = stream->getTable();
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
                currentInstruction->leDump();
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

int doPrune(Instr *in)
{
    int i;
    Operand *currentOperand;
    unsigned char any = 0;
    if (in->pruningRestricted)
        return 1;
    for (i = 0; (currentOperand = in->getOp(i)) != 0; i++) {
        any |= currentOperand->pruneMask;
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
        for (i = 0; (currentOperand = in->getOp(i)) != 0; i++) {
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
        for (i = 0; (currentOperand = in->getOp(i)) != 0; i++) {
            if (currentOperand->operandKind == 1 || currentOperand->operandKind == 9)
                currentOperand->componentMask = m;
        }
        if (info->decodeHook == rotateInputComponentMask)
            in->operands[1].componentMask = (((m & 8) >> 3) | (m << 1)) & 0xf;
    }
    for (i = 0; (currentOperand = in->getOp(i)) != 0; i++) {
        RegPoolEntry *registerEntry = registerOperand(currentOperand, 2);
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
            in->leDump(i);
            errorOccurred = saved;
            currentOperand->assignedRegister = pool->zeroRegister;
        }
    }
    return 1;
}

