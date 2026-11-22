// olList / ol_s operand-location lists, CIE group 5.
#pragma implementation "ol_s.h"
#include "Instr.h"
#include "ol_s.h"
#include "ol_s_inline.h"
#include "os.h"

void olList::validateOperandLocations()
{
    int visitedCount = 0;
    OperandLocation *location = head;
    if (count != 0) {
        for (; location; location = location->next) {
            if (location->instruction) {
                location->instruction->getOwningStream();
                location->instruction->dumpOperandDiagnostics(false);
            }
            visitedCount++;
        }
    } else if (location) {
        reportError("opLocList - had zero count, but non empty head\n");
        abortAfterError();
    }
    if (visitedCount != count) {
        reportError("oploc list - bad list not nent\n");
        abortAfterError();
    }
}

void olList::dumpOperandLocations()
{
    int visitedCount = 0;
    OperandLocation *location = head;
    if (count != 0) {
        for (; location; location = location->next) {
            if (location->instruction) {
                location->instruction->getOwningStream();
                location->instruction->dumpInstructionContext(location->operandIndex, 1 << location->componentIndex);
            }
            visitedCount++;
        }
    } else if (location) {
        exitWithError("opLocList - had zero count, but non empty head\n");
    }
    if (visitedCount != count)
        exitWithError("oploc list - bad list not nent\n");
}

void OperandLocation::dumpInstructionContext()
{
    if (instruction) {
        instruction->getOwningStream();
        instruction->dumpOperandDiagnostics(false);
    }
}

void OperandLocation::dumpOperandContext()
{
    if (instruction) {
        instruction->getOwningStream();
        instruction->dumpInstructionContext(operandIndex, 1 << componentIndex);
    }
}

Operand *operandAtLocation(OperandLocation *location)
{
    if (location->instruction)
        return &location->instruction->operands[location->operandIndex];
    return 0;
}

static inline void ol_s_copy_inl(OperandLocation *dst, OperandLocation *src)
{
    dst->reset();
    if (src) {
        dst->stateFlags = src->stateFlags;
        dst->instruction = src->instruction;
        dst->operandIndex = src->operandIndex;
        dst->componentIndex = src->componentIndex;
        dst->cycleOffset = src->cycleOffset;
    }
}

void OperandLocation::copyLocationFrom(OperandLocation *src)
{
    reset();
    if (src) {
        stateFlags = src->stateFlags;
        instruction = src->instruction;
        operandIndex = src->operandIndex;
        componentIndex = src->componentIndex;
        cycleOffset = src->cycleOffset;
    }
}

static inline void ol_s_init_inl(OperandLocation *location, Instr *instruction, int operandIndex, int slot, bool ext)
{
    location->reset();
    if (ext)
        location->stateFlags = 1;
    location->instruction = instruction;
    location->operandIndex = operandIndex;
    location->componentIndex = slot;
}

void OperandLocation::init(Instr *inst, int operandPosition, int slot, bool ext)
{
    reset();
    if (ext)
        stateFlags = 1;
    instruction = inst;
    operandIndex = operandPosition;
    componentIndex = slot;
}

static inline OperandLocation *allocOL()
{
    OperandLocation *location = olFreeList;
    if (!location) {
        location = new OperandLocation;
    } else {
        olFreeList = location->next;
        location->reset();
    }
    return location;
}

OperandLocation *olList::addOperandLocation(Instr *instruction, int operandIndex, int slot, bool ext)
{
    OperandLocation *prev = 0;
    OperandLocation *location;
    for (location = head; location; location = location->next) {
        if (instruction == location->instruction && operandIndex == location->operandIndex && slot == location->componentIndex) {
            if (ext != location->isExt()) {
                reportError("addOL: inst external state mismatch bExt=%d\n", ext);
                location->dumpInstructionContext();
                abortAfterError();
            }
            exitWithError("matched oploc\n");
            return location;
        }
        prev = location;
    }
    location = allocOL();
    ol_s_init_inl(location, instruction, operandIndex, slot, ext);
    if (prev)
        prev->next = location;
    else
        head = location;
    count++;
    return location;
}

OperandLocation *olList::addInstructionLocation(Instr *instruction)
{
    OperandLocation *prev = 0;
    OperandLocation *location;
    for (location = head; location; location = location->next) {
        if (instruction == location->instruction)
            return location;
        prev = location;
    }
    location = allocOL();
    ol_s_init_inl(location, instruction, 0, 0, false);
    if (prev)
        prev->next = location;
    else
        head = location;
    count++;
    return location;
}

OperandLocation *olList::copyOperandLocation(OperandLocation *location, bool ext)
{
    return addOperandLocation(location->instruction, location->operandIndex, location->componentIndex, ext);
}

OperandLocation *olList::firstInternalLocation()
{
    OperandLocation *location = head;
    if (location && location->isExt()) {
        location = location->next;
        while (location && location->isExt())
            location = location->next;
    }
    return location;
}

OperandLocation *OperandLocation::nextInternalLocation()
{
    OperandLocation *location = next;
    while (location && location->isExt())
        location = location->next;
    return location;
}

OperandLocation *olList::firstExternalLocation()
{
    OperandLocation *location = head;
    if (location && !location->isExt()) {
        location = location->next;
        while (location && !location->isExt())
            location = location->next;
    }
    return location;
}

OperandLocation *OperandLocation::nextExternalLocation()
{
    OperandLocation *location = next;
    while (location && !location->isExt())
        location = location->next;
    return location;
}

RegisterEntry *registerAtLocation(OperandLocation *location)
{
    Operand *operand = location->instruction ? &location->instruction->operands[location->operandIndex] : 0;
    if (operand)
        return operand->assignedRegister;
    exitWithError("no register!\n");
    return 0;
}

OperandLocation *olList::insertExternalLocation(OperandLocation *src)
{
    OperandLocation *prev = 0;
    OperandLocation *location;
    for (location = head; location; location = location->next) {
        if (src->cycleOffset < location->cycleOffset)
            break;
        prev = location;
    }
    OperandLocation *insertedLocation = allocOL();
    ol_s_copy_inl(insertedLocation, src);
    insertedLocation->stateFlags |= 1;
    insertedLocation->next = location;
    if (prev)
        prev->next = insertedLocation;
    else
        head = insertedLocation;
    count++;
    return insertedLocation;
}

void markLocationExternallyUsed(OperandLocation *location)
{
    location->stateFlags |= 2;
}

int hasRawOrImmediateInstructionAtLocation(OperandLocation *location)
{
    return location->instruction->hasRawTableOrImmediateToken();
}

int instructionSerialAtLocation(OperandLocation *location)
{
    return location->instruction->instructionSerial;
}

bool sameOperandLocation(OperandLocation *location, OperandLocation *secondLocation)
{
    if (location && secondLocation)
        return location->instruction == secondLocation->instruction && location->packedOperandComponent == secondLocation->packedOperandComponent;
    return location == secondLocation;
}

InstructionStream *locationOwningStream(OperandLocation *location)
{
    return location->instruction->getOwningStream();
}

int locationRow(OperandLocation *location)
{
    return location->instruction->position.linearRow;
}

OperandLocation *allocateLocationNode()
{
    return allocOL();
}

int countFreeLocationNodes()
{
    int freeCount = 0;
    for (OperandLocation *location = olFreeList; location; location = location->next)
        freeCount++;
    return freeCount;
}

OperandLocation *olFreeList = 0;
