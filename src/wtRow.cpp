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
    unk_00 = 0;
}

wtEntry::~wtEntry()
{
    if (unk_00 && !((VclPtrWord)unk_00 & 1)) {
        if (unk_00->unk_1428)
            addFreeMove(unk_00);
        else
            delete unk_00;
    }
    unk_00 = 0;
}

static inline bool clearWtEntry(wtEntryInl *e) { e->unk_00 = 0; return 1; }

bool wtRow::clearEntries()
{
    for (int i = 0; i != 2; i++)
        if (!clearWtEntry(&entries[i]))
            return 0;
    return 1;
}

bool wtEntry::clearInstruction()
{
    unk_00 = 0;
    return 1;
}

void wtRow::refreshInstructionOperands(int)
{
    for (int i = 0; i != 2; i++) {
        Instr *in = entries[i].unk_00;
        if (!in)
            continue;
        if ((VclPtrWord)in & 1)
            continue;
        in->dumpOperandDiagnostics(false);
    }
}

Instr *wtRow::getEntry(int p)
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
    return entries[p].unk_00;
}

Instr *GetI(Instr **ie, int p)
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
    Instr *in = ie[p];
    if ((VclPtrWord)in & 1)
        return 0;
    return in;
}

int wtRow::countEntries()
{
    int n = 0;
    for (int i = 0; i != 2; i++)
        if (entries[i].unk_00)
            n++;
    return n;
}

Instr *wtRow::onlyEntry()
{
    Instr *r = 0;
    for (int i = 0; i != 2; i++) {
        if (entries[i].unk_00) {
            if (r)
                return 0;
            r = entries[i].unk_00;
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
            Instr *in = GetI(&entries[0].unk_00, i);
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
            Instr *in = GetI(&entries[0].unk_00, i);
            if (in)
                r |= in->inputMaskForRegister(reg, mask);
        }
    }
    return r;
}

unsigned int wtRow::getMinimumExtraLatencyAge()
{
    Instr *in = GetI(&entries[0].unk_00, 1);
    unsigned int m = 0;
    if (in) {
        for (int i = 0; i != in->operandCount; i++) {
            if (in->operands[i].additionalLatency) {
                unsigned int a = (unsigned short)in->operands[i].assignedRegister->parts[0].cycleAge;
                if (!m || a < m)
                    m = a;
            }
        }
    }
    return m;
}
