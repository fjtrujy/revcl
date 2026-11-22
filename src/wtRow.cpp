#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "loadStalls_shared.h"

extern const char wtRowGetIPMaxError[];
extern const char wtRowAddStallLenError[];

wtRow::wtRow()
{
    cycleTime = 0;
    skip = 0;
    stallTypeMask = 0;
    maxStallLength = 0;
}

wtRow::~wtRow()
{
}

wtEntry::wtEntry()
{
    instruction = 0;
}

wtEntry::~wtEntry()
{
    if (instruction && !((VclPtrWord)instruction & 1)) {
        if (instruction->moveRecordWord)
            addFreeMove(instruction);
        else
            delete instruction;
    }
    instruction = 0;
}

static inline bool clearWtEntry(wtEntryInl *rowEntry) { rowEntry->instruction = 0; return 1; }

bool wtRow::clearEntries()
{
    for (int entryIndex = 0; entryIndex != 2; entryIndex++)
        if (!clearWtEntry(&entries[entryIndex]))
            return 0;
    return 1;
}

bool wtEntry::clearInstruction()
{
    instruction = 0;
    return 1;
}

void wtRow::refreshInstructionOperands(int)
{
    for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
        Instr *instruction = entries[entryIndex].instruction;
        if (!instruction)
            continue;
        if ((VclPtrWord)instruction & 1)
            continue;
        instruction->dumpOperandDiagnostics(false);
    }
}

Instr *wtRow::getEntry(int entryIndex)
{
    if (entryIndex < 0) {
        reportError("iEntry::GetI() - p<0\n");
        exitAfterCleanup(-1);
        return 0;
    }
    if (entryIndex >= 2) {
        reportError(wtRowGetIPMaxError);
        exitAfterCleanup(-1);
        return 0;
    }
    return entries[entryIndex].instruction;
}

Instr *GetI(Instr **instructionSlot, int pipeIndex)
{
    if (pipeIndex < 0) {
        reportError("iEntry::GetI() - p<0\n");
        exitAfterCleanup(-1);
        return 0;
    }
    if (pipeIndex >= 2) {
        reportError(wtRowGetIPMaxError);
        exitAfterCleanup(-1);
        return 0;
    }
    Instr *instruction = instructionSlot[pipeIndex];
    if ((VclPtrWord)instruction & 1)
        return 0;
    return instruction;
}

int wtRow::countEntries()
{
    int entryCount = 0;
    for (int entryIndex = 0; entryIndex != 2; entryIndex++)
        if (entries[entryIndex].instruction)
            entryCount++;
    return entryCount;
}

Instr *wtRow::onlyEntry()
{
    Instr *onlyInstruction = 0;
    for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
        if (entries[entryIndex].instruction) {
            if (onlyInstruction)
                return 0;
            onlyInstruction = entries[entryIndex].instruction;
        }
    }
    return onlyInstruction;
}

bool wtRow::clearStalls()
{
    stallTypeMask = 0;
    maxStallLength = 0;
    return 1;
}

void wtRow::resetStallMetadata()
{
    stallTypeMask = 0;
    maxStallLength = 0;
}

bool wtRow::addStall(int type, int len)
{
    if (len <= 0) {
        reportError(wtRowAddStallLenError);
        return 0;
    }
    if (len > 255)
        reportWarning("truncating stall length..\n");
    if (type < 0 || type > 2) {
        reportError("bad stall type\n");
        return 0;
    }
    stallTypeMask |= 1 << type;
    if (maxStallLength < len)
        maxStallLength = len;
    return 1;
}

int wtRow::outputMaskFor(RegPoolEntry *reg, unsigned char mask)
{
    int outputMask = 0;
    if (!skip) {
        for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
            Instr *instruction = GetI(&entries[0].instruction, pipeIndex);
            if (instruction)
                outputMask |= instruction->outputMaskForRegister(reg, mask);
        }
    }
    return outputMask;
}

int wtRow::inputMaskFor(RegPoolEntry *reg, unsigned char mask)
{
    int inputMask = 0;
    if (!skip) {
        for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
            Instr *instruction = GetI(&entries[0].instruction, pipeIndex);
            if (instruction)
                inputMask |= instruction->inputMaskForRegister(reg, mask);
        }
    }
    return inputMask;
}

unsigned int wtRow::getMinimumExtraLatencyAge()
{
    Instr *instruction = GetI(&entries[0].instruction, 1);
    unsigned int minimumCycleAge = 0;
    if (instruction) {
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            if (instruction->operands[operandIndex].additionalLatency) {
                unsigned int registerCycleAge = (unsigned short)instruction->operands[operandIndex].assignedRegister->parts[0].cycleAge;
                if (!minimumCycleAge || registerCycleAge < minimumCycleAge)
                    minimumCycleAge = registerCycleAge;
            }
        }
    }
    return minimumCycleAge;
}
