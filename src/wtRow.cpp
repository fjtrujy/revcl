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
        Instr *in = entries[entryIndex].instruction;
        if (!in)
            continue;
        if ((VclPtrWord)in & 1)
            continue;
        in->dumpOperandDiagnostics(false);
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

Instr *GetI(Instr **instructionSlot, int p)
{
    if (p < 0) {
        reportError("iEntry::GetI() - p<0\n");
        exitAfterCleanup(-1);
        return 0;
    }
    if (p >= 2) {
        reportError(wtRowGetIPMaxError);
        exitAfterCleanup(-1);
        return 0;
    }
    Instr *instruction = instructionSlot[p];
    if ((VclPtrWord)instruction & 1)
        return 0;
    return instruction;
}

int wtRow::countEntries()
{
    int n = 0;
    for (int entryIndex = 0; entryIndex != 2; entryIndex++)
        if (entries[entryIndex].instruction)
            n++;
    return n;
}

Instr *wtRow::onlyEntry()
{
    Instr *r = 0;
    for (int entryIndex = 0; entryIndex != 2; entryIndex++) {
        if (entries[entryIndex].instruction) {
            if (r)
                return 0;
            r = entries[entryIndex].instruction;
        }
    }
    return r;
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
    int r = 0;
    if (!skip) {
        for (int i = 0; i != 2; i++) {
            Instr *in = GetI(&entries[0].instruction, i);
            if (in)
                r |= in->outputMaskForRegister(reg, mask);
        }
    }
    return r;
}

int wtRow::inputMaskFor(RegPoolEntry *reg, unsigned char mask)
{
    int r = 0;
    if (!skip) {
        for (int i = 0; i != 2; i++) {
            Instr *in = GetI(&entries[0].instruction, i);
            if (in)
                r |= in->inputMaskForRegister(reg, mask);
        }
    }
    return r;
}

unsigned int wtRow::getMinimumExtraLatencyAge()
{
    Instr *in = GetI(&entries[0].instruction, 1);
    unsigned int m = 0;
    if (in) {
        for (int operandIndex = 0; operandIndex != in->operandCount; operandIndex++) {
            if (in->operands[operandIndex].additionalLatency) {
                unsigned int a = (unsigned short)in->operands[operandIndex].assignedRegister->parts[0].cycleAge;
                if (!m || a < m)
                    m = a;
            }
        }
    }
    return m;
}
