#include "Instr.h"
#include "os.h"

iGroupList::iGroupList()
{
    head = 0;
}

iGroupList::~iGroupList()
{
    iGroupNode *groupNode = head;
    while (groupNode) {
        iGroupNode *next = groupNode->next;
        if (groupNode)
            ::operator delete(groupNode);
        groupNode = next;
    }
    head = 0;
}

void iGroupList::init()
{
    if (head)
        reportWarning("igrouplist::init - first set!\n");
    iGroupNode *instructionGroupNode = head;
    while (instructionGroupNode) {
        iGroupNode *next = instructionGroupNode->next;
        if (instructionGroupNode)
            ::operator delete(instructionGroupNode);
        instructionGroupNode = next;
    }
    head = 0;
}

void iGroupList::clear()
{
    iGroupNode *groupNode = head;
    while (groupNode) {
        iGroupNode *next = groupNode->next;
        if (groupNode)
            ::operator delete(groupNode);
        groupNode = next;
    }
    head = 0;
}

inline iGroupNode::iGroupNode(InitTag)
{
    itemPointer = 0;
    itemSize = 0;
}


bool iGroupList::add(void *p, int sz)
{
    if (sz <= 0)
        return 1;
    iGroupNode *instructionGroupNode = new iGroupNode(iGroupNode::INIT);
    if (!instructionGroupNode) {
        reportError("failed to get a new iGroupNode\n");
        return 0;
    }
    instructionGroupNode->itemPointer = p;
    instructionGroupNode->itemSize = sz;
    instructionGroupNode->next = head;
    head = instructionGroupNode;
    return 1;
}

void iGroupList::dump()
{
    iGroupNode *groupNode;
    int i = 0;
    for (groupNode = head; groupNode; groupNode = groupNode->next, i++)
        reportInfo(" [%4d ] [%d sz=%d]\n", i, groupNode->itemPointer, groupNode->itemSize);
    if (i)
        reportInfo(" .. %d nodes\n", i);
}


iGroupNode::iGroupNode()
{
    itemPointer = 0;
    itemSize = 0;
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

int iGroupLog::logTable(workTable *sourceTable)
{
    if (table) {
        reportError("logTable - called - but the table is already open\n");
        return 0;
    }
    for (regUse *previousUse = sourceTable->registerUses.head; previousUse; previousUse = previousUse->next) {
        RegPoolEntry *reg = previousUse->registerEntry;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if (((previousUse->externalInputMask & previousUse->externalOutputMask) >> partIndex) & 1) {
            Instr *instruction;
            for (instruction = sourceTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
                instruction->dependencyReachabilityFlags = 0;
            for (Instr *cur = (instruction = sourceTable->linearInstructions.head); instruction; cur = (instruction = instruction->linearLink.nextInstruction))
                for (int operandIndex = 0; operandIndex != cur->operandCount; operandIndex++) {
                    Operand *operand = &cur->operands[operandIndex];
                    RegPoolEntry *registerEntry = operand->assignedRegister;
                    if (registerEntry == reg && operand->externalPartMask)
                        for (int k = 0; k != registerEntry->partCount; k++) {
                            if (isRegisterOperand(operand, 2) && ((operand->externalPartMask >> k) & 1))
                                instruction->markDependencyOutputReachable();
                            if (isRegisterOperand(operand, 1) && ((operand->externalPartMask >> k) & 1))
                                instruction->markDependencyInputReachable();
                        }
                }
            bool changed;
            do {
                changed = false;
                for (instruction = sourceTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
                    if (instruction->inputDependencyReachable)
                        for (int j = 0; j != instruction->operandCount; j++) {
                            Operand *operand = &instruction->operands[j];
                            RegPoolEntry *registerEntry = registerOperand(operand, 2);
                            if (registerEntry)
                                for (int k = 0; k != registerEntry->partCount; k++)
                                    for (ol_s *operandLocation = operand->parts[k].operandLocations.firstInternalLocation(); operandLocation; operandLocation = operandLocation->nextInternalLocation()) {
                                        Instr *t = operandLocation->instruction;
                                        changed = changed || t->markDependencyInputReachable();
                                    }
                        }
                wtInstrList *list = &sourceTable->linearInstructions;
                for (instruction = list->tail; instruction; instruction = instruction->linearLink.previousInstruction)
                    if (instruction->outputDependencyReachable)
                        for (int j = 0; j != instruction->operandCount; j++) {
                            Operand *operand = &instruction->operands[j];
                            RegPoolEntry *registerEntry = registerOperand(operand, 1);
                            if (registerEntry)
                                for (int k = 0; k != registerEntry->partCount; k++)
                                    for (ol_s *operandLocation = operand->parts[k].operandLocations.firstInternalLocation(); operandLocation; operandLocation = operandLocation->nextInternalLocation())
                                        operandLocation->instruction->outputDependencyReachable = 1;
                        }
            } while (changed);
            reportInfo("  dumping scan for %s\n", reg->maskName(1 << partIndex));
            int n = 0;
            for (instruction = sourceTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
                if (instruction->dependencyReachabilityFlags == 3) {
                    const char *name = instruction->opcodeInfo ? instruction->opcodeInfo->mnemonic : "NO_BASE_INST";
                    reportInfo("  [%4d] %d %s\n", n++, instruction->instructionId, name);
                }
            reportInfo("  ..%d matches\n", n);
            }
        }
    }
    return 1;
}
