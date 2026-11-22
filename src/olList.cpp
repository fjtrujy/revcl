// olList / ol_s operand-location lists, CIE group 5.
#pragma implementation "ol_s.h"
#include "Instr.h"
#include "ol_s.h"
#include "ol_s_inline.h"
#include "os.h"

void olList::validateOperandLocations()
{
    int n = 0;
    ol_s *p = head;
    if (count != 0) {
        for (; p; p = p->next) {
            if (p->instruction) {
                p->instruction->getOwningStream();
                p->instruction->func_08054008(false);
            }
            n++;
        }
    } else if (p) {
        reportError("opLocList - had zero count, but non empty head\n");
        abortAfterError();
    }
    if (n != count) {
        reportError("oploc list - bad list not nent\n");
        abortAfterError();
    }
}

void olList::dumpOperandLocations()
{
    int n = 0;
    ol_s *p = head;
    if (count != 0) {
        for (; p; p = p->next) {
            if (p->instruction) {
                p->instruction->getOwningStream();
                p->instruction->leDump(p->operandIndex, 1 << p->componentIndex);
            }
            n++;
        }
    } else if (p) {
        exitWithError("opLocList - had zero count, but non empty head\n");
    }
    if (n != count)
        exitWithError("oploc list - bad list not nent\n");
}

void ol_s::dumpInstructionContext()
{
    if (instruction) {
        instruction->getOwningStream();
        instruction->func_08054008(false);
    }
}

void ol_s::dumpOperandContext()
{
    if (instruction) {
        instruction->getOwningStream();
        instruction->leDump(operandIndex, 1 << componentIndex);
    }
}

Operand *operandAtLocation(ol_s *ol)
{
    if (ol->instruction)
        return &ol->instruction->operands[ol->operandIndex];
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
        dst->unk_12 = src->unk_12;
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
        unk_12 = src->unk_12;
    }
}

static inline void ol_s_init_inl(ol_s *o, Instr *inst, int op, int slot, bool ext)
{
    o->reset();
    if (ext)
        o->stateFlags = 1;
    o->instruction = inst;
    o->operandIndex = op;
    o->componentIndex = slot;
}

void ol_s::init(Instr *inst, int op, int slot, bool ext)
{
    reset();
    if (ext)
        stateFlags = 1;
    instruction = inst;
    operandIndex = op;
    componentIndex = slot;
}

static inline ol_s *allocOL()
{
    ol_s *p = olFreeList;
    if (!p) {
        p = new ol_s;
    } else {
        olFreeList = p->next;
        p->reset();
    }
    return p;
}

ol_s *olList::addOperandLocation(Instr *inst, int op, int slot, bool ext)
{
    ol_s *prev = 0;
    ol_s *p;
    for (p = head; p; p = p->next) {
        if (inst == p->instruction && op == p->operandIndex && slot == p->componentIndex) {
            if (ext != p->isExt()) {
                reportError("addOL: inst external state mismatch bExt=%d\n", ext);
                p->dumpInstructionContext();
                abortAfterError();
            }
            exitWithError("matched oploc\n");
            return p;
        }
        prev = p;
    }
    p = allocOL();
    ol_s_init_inl(p, inst, op, slot, ext);
    if (prev)
        prev->next = p;
    else
        head = p;
    count++;
    return p;
}

ol_s *olList::addInstructionLocation(Instr *inst)
{
    ol_s *prev = 0;
    ol_s *p;
    for (p = head; p; p = p->next) {
        if (inst == p->instruction)
            return p;
        prev = p;
    }
    p = allocOL();
    ol_s_init_inl(p, inst, 0, 0, false);
    if (prev)
        prev->next = p;
    else
        head = p;
    count++;
    return p;
}

ol_s *olList::copyOperandLocation(ol_s *ol, bool ext)
{
    return addOperandLocation(ol->instruction, ol->operandIndex, ol->componentIndex, ext);
}

ol_s *olList::firstInternalLocation()
{
    ol_s *p = head;
    if (p && p->isExt()) {
        p = p->next;
        while (p && p->isExt())
            p = p->next;
    }
    return p;
}

ol_s *ol_s::nextInternalLocation()
{
    ol_s *p = next;
    while (p && p->isExt())
        p = p->next;
    return p;
}

ol_s *olList::firstExternalLocation()
{
    ol_s *p = head;
    if (p && !p->isExt()) {
        p = p->next;
        while (p && !p->isExt())
            p = p->next;
    }
    return p;
}

ol_s *ol_s::nextExternalLocation()
{
    ol_s *p = next;
    while (p && !p->isExt())
        p = p->next;
    return p;
}

RegPoolEntry *registerAtLocation(ol_s *ol)
{
    Operand *op = ol->instruction ? &ol->instruction->operands[ol->operandIndex] : 0;
    if (op)
        return op->assignedRegister;
    exitWithError("no register!\n");
    return 0;
}

ol_s *olList::insertExternalLocation(ol_s *src)
{
    ol_s *prev = 0;
    ol_s *p;
    for (p = head; p; p = p->next) {
        if (src->unk_12 < p->unk_12)
            break;
        prev = p;
    }
    ol_s *n = allocOL();
    ol_s_copy_inl(n, src);
    n->stateFlags |= 1;
    n->next = p;
    if (prev)
        prev->next = n;
    else
        head = n;
    count++;
    return n;
}

void markLocationExternallyUsed(ol_s *ol)
{
    ol->stateFlags |= 2;
}

int func_080585a8(ol_s *ol)
{
    return ol->instruction->func_08055548();
}

int instructionSerialAtLocation(ol_s *ol)
{
    return ol->instruction->instructionSerial;
}

bool sameOperandLocation(ol_s *a, ol_s *b)
{
    if (a && b)
        return a->instruction == b->instruction && a->packedOperandComponent == b->packedOperandComponent;
    return a == b;
}

instStream *locationOwningStream(ol_s *ol)
{
    return ol->instruction->getOwningStream();
}

int locationRow(ol_s *ol)
{
    return ol->instruction->position.unk_00;
}

ol_s *allocateLocationNode()
{
    return allocOL();
}

int countFreeLocationNodes()
{
    int n = 0;
    for (ol_s *p = olFreeList; p; p = p->next)
        n++;
    return n;
}

ol_s *olFreeList = 0;
