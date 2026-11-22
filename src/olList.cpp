// olList / ol_s operand-location lists, CIE group 5.
#pragma implementation "ol_s.h"
#include "Instr.h"
#include "ol_s.h"
#include "ol_s_inline.h"
#include "os.h"

void olList::validateOperandLocations()
{
    int visitedCount = 0;
    ol_s *location = head;
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
    ol_s *location = head;
    if (count != 0) {
        for (; location; location = location->next) {
            if (location->instruction) {
                location->instruction->getOwningStream();
                location->instruction->leDump(location->operandIndex, 1 << location->componentIndex);
            }
            visitedCount++;
        }
    } else if (location) {
        exitWithError("opLocList - had zero count, but non empty head\n");
    }
    if (visitedCount != count)
        exitWithError("oploc list - bad list not nent\n");
}

void ol_s::dumpInstructionContext()
{
    if (instruction) {
        instruction->getOwningStream();
        instruction->dumpOperandDiagnostics(false);
    }
}

void ol_s::dumpOperandContext()
{
    if (instruction) {
        instruction->getOwningStream();
        instruction->leDump(operandIndex, 1 << componentIndex);
    }
}

Operand *operandAtLocation(ol_s *location)
{
    if (location->instruction)
        return &location->instruction->operands[location->operandIndex];
    return 0;
}

static inline void ol_s_copy_inl(ol_s *dst, ol_s *src)
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

void ol_s::copyLocationFrom(ol_s *src)
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

static inline void ol_s_init_inl(ol_s *location, Instr *inst, int operandIndex, int slot, bool ext)
{
    location->reset();
    if (ext)
        location->stateFlags = 1;
    location->instruction = inst;
    location->operandIndex = operandIndex;
    location->componentIndex = slot;
}

void ol_s::init(Instr *inst, int operandPosition, int slot, bool ext)
{
    reset();
    if (ext)
        stateFlags = 1;
    instruction = inst;
    operandIndex = operandPosition;
    componentIndex = slot;
}

static inline ol_s *allocOL()
{
    ol_s *location = olFreeList;
    if (!location) {
        location = new ol_s;
    } else {
        olFreeList = location->next;
        location->reset();
    }
    return location;
}

ol_s *olList::addOperandLocation(Instr *inst, int operandIndex, int slot, bool ext)
{
    ol_s *prev = 0;
    ol_s *location;
    for (location = head; location; location = location->next) {
        if (inst == location->instruction && operandIndex == location->operandIndex && slot == location->componentIndex) {
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
    ol_s_init_inl(location, inst, operandIndex, slot, ext);
    if (prev)
        prev->next = location;
    else
        head = location;
    count++;
    return location;
}

ol_s *olList::addInstructionLocation(Instr *inst)
{
    ol_s *prev = 0;
    ol_s *location;
    for (location = head; location; location = location->next) {
        if (inst == location->instruction)
            return location;
        prev = location;
    }
    location = allocOL();
    ol_s_init_inl(location, inst, 0, 0, false);
    if (prev)
        prev->next = location;
    else
        head = location;
    count++;
    return location;
}

ol_s *olList::copyOperandLocation(ol_s *location, bool ext)
{
    return addOperandLocation(location->instruction, location->operandIndex, location->componentIndex, ext);
}

ol_s *olList::firstInternalLocation()
{
    ol_s *location = head;
    if (location && location->isExt()) {
        location = location->next;
        while (location && location->isExt())
            location = location->next;
    }
    return location;
}

ol_s *ol_s::nextInternalLocation()
{
    ol_s *location = next;
    while (location && location->isExt())
        location = location->next;
    return location;
}

ol_s *olList::firstExternalLocation()
{
    ol_s *location = head;
    if (location && !location->isExt()) {
        location = location->next;
        while (location && !location->isExt())
            location = location->next;
    }
    return location;
}

ol_s *ol_s::nextExternalLocation()
{
    ol_s *location = next;
    while (location && !location->isExt())
        location = location->next;
    return location;
}

RegPoolEntry *registerAtLocation(ol_s *location)
{
    Operand *operand = location->instruction ? &location->instruction->operands[location->operandIndex] : 0;
    if (operand)
        return operand->assignedRegister;
    exitWithError("no register!\n");
    return 0;
}

ol_s *olList::insertExternalLocation(ol_s *src)
{
    ol_s *prev = 0;
    ol_s *location;
    for (location = head; location; location = location->next) {
        if (src->cycleOffset < location->cycleOffset)
            break;
        prev = location;
    }
    ol_s *insertedLocation = allocOL();
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

void markLocationExternallyUsed(ol_s *location)
{
    location->stateFlags |= 2;
}

int hasRawOrImmediateInstructionAtLocation(ol_s *location)
{
    return location->instruction->hasRawTableOrImmediateToken();
}

int instructionSerialAtLocation(ol_s *location)
{
    return location->instruction->instructionSerial;
}

bool sameOperandLocation(ol_s *location, ol_s *secondLocation)
{
    if (location && secondLocation)
        return location->instruction == secondLocation->instruction && location->packedOperandComponent == secondLocation->packedOperandComponent;
    return location == secondLocation;
}

instStream *locationOwningStream(ol_s *location)
{
    return location->instruction->getOwningStream();
}

int locationRow(ol_s *location)
{
    return location->instruction->position.linearRow;
}

ol_s *allocateLocationNode()
{
    return allocOL();
}

int countFreeLocationNodes()
{
    int freeCount = 0;
    for (ol_s *location = olFreeList; location; location = location->next)
        freeCount++;
    return freeCount;
}

ol_s *olFreeList = 0;
