#include "Instr.h"
#include "os.h"

iGroupList::iGroupList()
{
    head = 0;
}

iGroupList::~iGroupList()
{
    iGroupNode *n = head;
    while (n) {
        iGroupNode *next = n->next;
        if (n)
            ::operator delete(n);
        n = next;
    }
    head = 0;
}

void iGroupList::init()
{
    if (head)
        reportWarning("igrouplist::init - first set!\n");
    iGroupNode *n = head;
    while (n) {
        iGroupNode *next = n->next;
        if (n)
            ::operator delete(n);
        n = next;
    }
    head = 0;
}

void iGroupList::clear()
{
    iGroupNode *n = head;
    while (n) {
        iGroupNode *next = n->next;
        if (n)
            ::operator delete(n);
        n = next;
    }
    head = 0;
}

inline iGroupNode::iGroupNode(InitTag)
{
    unk_04 = 0;
    unk_08 = 0;
}


bool iGroupList::add(void *p, int sz)
{
    if (sz <= 0)
        return 1;
    iGroupNode *n = new iGroupNode(iGroupNode::INIT);
    if (!n) {
        reportError("failed to get a new iGroupNode\n");
        return 0;
    }
    n->unk_04 = p;
    n->unk_08 = sz;
    n->next = head;
    head = n;
    return 1;
}

void iGroupList::dump()
{
    iGroupNode *n;
    int i = 0;
    for (n = head; n; n = n->next, i++)
        reportInfo(" [%4d ] [%d sz=%d]\n", i, n->unk_04, n->unk_08);
    if (i)
        reportInfo(" .. %d nodes\n", i);
}


iGroupNode::iGroupNode()
{
    unk_04 = 0;
    unk_08 = 0;
}

iGroupNode::~iGroupNode()
{
}

#include <stdlib.h>
#include "iGroupLog.h"
#include "Instr.h"
#include "workTable.h"
#include "RegPool.h"
#include "ol_s.h"

int reportInfo(const char *fmt, ...);
int reportError(const char *fmt, ...);

Instr **iGroupLog::table;
int iGroupLog::count;
int iGroupLog::size;

iGroupLog::iGroupLog()
{
    table = 0;
    count = 0;
    size = 0;
}

iGroupLog::~iGroupLog()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
}

int iGroupLog::init()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
    return 1;
}

int iGroupLog::reset()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
    return 1;
}

int iGroupLog::add(Instr *ins)
{
    if (!ins) {
        reportError("iGroup - add instruction - no instruction to add!\n");
        return 0;
    }
    if (count > size) {
        reportError("overflow for the iGroup log\n");
        return 0;
    }
    if (count == size) {
        size = count * 2 + 0x2000;
        int bytes = size * sizeof(Instr *);
        table = (Instr **)realloc(table, bytes);
        if (!table) {
            reportError("iGroup - failed to realloc table to %d\n", bytes);
            return 0;
        }
    }
    table[count] = ins;
    return count++;
}

int iGroupLog::logTable(workTable *wt)
{
    if (table) {
        reportError("logTable - called - but the table is already open\n");
        return 0;
    }
    for (regUse *p = wt->registerUses.head; p; p = p->next) {
        RegPoolEntry *reg = p->registerEntry;
        for (int i = 0; i != reg->partCount; i++) {
            if (((p->externalInputMask & p->externalOutputMask) >> i) & 1) {
            Instr *in;
            for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
                in->unk_0e = 0;
            for (Instr *cur = (in = wt->linearInstructions.head); in; cur = (in = in->linearLink.nextInstruction))
                for (int j = 0; j != cur->operandCount; j++) {
                    Operand *op = &cur->operands[j];
                    RegPoolEntry *r = op->assignedRegister;
                    if (r == reg && op->externalPartMask)
                        for (int k = 0; k != r->partCount; k++) {
                            if (isRegisterOperand(op, 2) && ((op->externalPartMask >> k) & 1))
                                in->markDependencyOutputReachable();
                            if (isRegisterOperand(op, 1) && ((op->externalPartMask >> k) & 1))
                                in->markDependencyInputReachable();
                        }
                }
            bool changed;
            do {
                changed = false;
                for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
                    if (in->unk_0e_1)
                        for (int j = 0; j != in->operandCount; j++) {
                            Operand *op = &in->operands[j];
                            RegPoolEntry *r = registerOperand(op, 2);
                            if (r)
                                for (int k = 0; k != r->partCount; k++)
                                    for (ol_s *o = op->parts[k].operandLocations.firstInternalLocation(); o; o = o->nextInternalLocation()) {
                                        Instr *t = o->instruction;
                                        changed = changed || t->markDependencyInputReachable();
                                    }
                        }
                wtInstrList *list = &wt->linearInstructions;
                for (in = list->tail; in; in = in->linearLink.previousInstruction)
                    if (in->unk_0e_0)
                        for (int j = 0; j != in->operandCount; j++) {
                            Operand *op = &in->operands[j];
                            RegPoolEntry *r = registerOperand(op, 1);
                            if (r)
                                for (int k = 0; k != r->partCount; k++)
                                    for (ol_s *o = op->parts[k].operandLocations.firstInternalLocation(); o; o = o->nextInternalLocation())
                                        o->instruction->unk_0e_0 = 1;
                        }
            } while (changed);
            reportInfo("  dumping scan for %s\n", reg->maskName(1 << i));
            int n = 0;
            for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction)
                if (in->unk_0e == 3) {
                    const char *name = in->opcodeInfo ? in->opcodeInfo->mnemonic : "NO_BASE_INST";
                    reportInfo("  [%4d] %d %s\n", n++, in->instructionId, name);
                }
            reportInfo("  ..%d matches\n", n);
            }
        }
    }
    return 1;
}
