#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegPoolEntry *collapseImmediateSourceRegister;
extern mValue *collapseImmediateValue;
extern int collapseImmediateOffset;
int decodeCollapsibleMoveImmediate(Instr *in);
int expandOneLoopIntoRegions(Program *p, instStream *s);


extern const char propagateValidMaskError[];
extern const char propagateValidDetailError[];
extern const char propagateTwoWritesError[];
extern const char propagateNormaliseError[];

bool noopMovePropagationHook(Program *program)
{
    return true;
}

int propagateMoves(workTable *scheduleTable)
{
    wtInstrList *list = &scheduleTable->linearInstructions;
    int count = 0;
    for (Instr *instruction = list->tail; instruction; instruction = instruction->linearLink.prev()) {
        if (instruction->normalizeZeroSources() < 0) {
            reportError(propagateNormaliseError);
            return 0;
        }
        if (!instruction->abstractMoveOpcode)
            continue;
        int mask = instruction->operands[0].componentMask;
        RegPoolEntry *dst = instruction->operands[0].assignedRegister;
        RegPoolEntry *src = instruction->operands[1].assignedRegister;
        int keep = 0;
        int srcWritten = 0;
        int valid = mask;
        int row = instruction->position.linearRow;
        Instr *nextInstruction = instruction->linearLink.next();
        Instr *cur = nextInstruction;
        if (row >= 0) {
            Instr *previousInstruction = instruction->linearLink.prev();
            if (previousInstruction && previousInstruction->position.linearRow == row) {
                bool previousDestinationPaired;
                if (previousInstruction->collectOutputRegisterMask(dst, &previousDestinationPaired)) {
                    reportError(propagateTwoWritesError);
                    return 0;
                }
                bool previousSourcePaired;
                srcWritten = previousInstruction->collectOutputRegisterMask(src, &previousSourcePaired);
            }
            if (nextInstruction && nextInstruction->position.linearRow == row) {
                bool nextDestinationPaired;
                if (nextInstruction->collectOutputRegisterMask(dst, &nextDestinationPaired)) {
                    reportError(propagateTwoWritesError);
                    return 0;
                }
                bool nextSourcePaired;
                srcWritten |= nextInstruction->collectOutputRegisterMask(src, &nextSourcePaired);
                cur = nextInstruction->linearLink.next();
            }
        }
        int pendSrc = 0;
        int pendDst = 0;
        while (cur && valid) {
            int currentRow = cur->position.linearRow;
            if (currentRow < 0 || currentRow != row) {
                srcWritten |= pendSrc;
                valid &= ~pendDst;
                pendSrc = 0;
                pendDst = 0;
            }
            bool paired = false;
            int used = cur->collectInputRegisterMask(dst, &paired);
            if (used) {
                if (!(used & ~valid) && !(used & srcWritten) && !paired)
                    for (int operandIndex = 0; operandIndex != cur->operandCount; operandIndex++) {
                        Operand *operand = &cur->operands[operandIndex];
                        if (registerOperand(operand, 1) == dst)
                            operand->getRegFrom(&instruction->operands[1]);
                        count++;
                    }
                else
                    keep |= used & valid;
            }
            bool pendingSourcePaired;
            pendSrc |= cur->collectOutputRegisterMask(src, &pendingSourcePaired);
            bool pendingDestinationPaired;
            pendDst |= cur->collectOutputRegisterMask(dst, &pendingDestinationPaired);
            cur = cur->linearLink.nextInstruction;
            row = currentRow;
        }
        if (~valid & instruction->operands[0].externalPartMask) {
            reportError(propagateValidMaskError,
                          dst->registerName(), src->registerName());
            reportError(propagateValidDetailError,
                          valid, instruction->operands[0].externalPartMask);
            return -1;
        }
        keep |= instruction->operands[0].externalPartMask;
        if (!keep)
            instruction->operands[0].assignedRegister = instruction->operands[1].assignedRegister;
        instruction->destinationMask = keep;
        instruction->operands[0].componentMask = keep;
        instruction->operands[1].componentMask = keep;
    }
    return count;
}

int removeDeadMoves(workTable *scheduleTable)
{
    int count = 0;
    Instr *next;
    for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = next) {
        next = instruction->linearLink.nextInstruction;
        if (instruction->abstractMoveOpcode && instruction->operands[0].assignedRegister == instruction->operands[1].assignedRegister) {
            if (!scheduleTable->destroyInstruction(instruction)) {
                reportError("failed to destroy a move\n");
                return -1;
            }
            count++;
        }
    }
    return count;
}
