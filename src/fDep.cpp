#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"

bool buildForwardDependencyList(ScheduleTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool addAdditionalClonePredecessors(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, OperandLocation *o, olList *dst);

Operand *getEA(Instr *instruction)
{
    OpcodeDescriptor *instructionInfo = instruction->opcodeInfo;
    if (!instructionInfo) {
        reportError("no ii for getEA\n");
        exitAfterCleanup(-1);
    }
    if ((instructionInfo->kindAndFlags & 7) == 4) {
        if (instructionInfo->branchTargetOperandSelector >= instruction->operandCount) {
            reportError("eaop is out of bounds for operands st=%d\n", instructionInfo->kindData);
            instruction->dumpInstructionContext();
            exitAfterCleanup(-1);
        }
        Operand *operand = &instruction->operands[instructionInfo->branchTargetOperandSelector];
        if (operand->direction != VCL_OPERAND_READ) {
            reportError("effective address specifier isn't an input\n");
            exitAfterCleanup(-1);
        }
        return operand;
    }
    return 0;
}

static inline bool sameValue(Operand *operand, Operand *secondOperand)
{
    return sameImmediateValue(&operand->immediateValue, &secondOperand->immediateValue);
}

static inline bool sameOwner(Instr *instruction, Instr *secondInstruction)
{
    if (instruction->ownerTable == secondInstruction->ownerTable)
        return true;
    return false;
}

bool sameEA(Instr *instruction, Instr *secondInstruction)
{
    if (!instruction || !secondInstruction || !instruction->ramAccessType || !secondInstruction->ramAccessType || !sameOwner(instruction, secondInstruction))
        return false;
    Operand *ea1 = getEA(instruction);
    Operand *ea2 = getEA(secondInstruction);
    if (!ea1 || !ea2) {
        reportError("failed to get effective address operand\n");
        exitAfterCleanup(-1);
    }
    if (ea1->externalPartMask != ea2->externalPartMask)
        return false;
    RegisterEntry *r1 = ea1->assignedRegister;
    RegisterEntry *r2 = ea2->assignedRegister;
    if (r1 && r1 == r1->ownerPool->zeroRegister)
        r1 = 0;
    if (r2 && r2 == r2->ownerPool->zeroRegister)
        r2 = 0;
    if (r1 != r2)
        return false;
    if (r1) {
        if (ea1->componentMask != ea2->componentMask)
            return false;
        if (ea1->componentMask != VCL_COMPONENT_X) {
            reportError("mod bits not 1 for an effective address specifier\n");
            exitAfterCleanup(-1);
        }
        if (!sameOperandLocation(ea1->parts[0].operandLocations.head, ea2->parts[0].operandLocations.head))
            return false;
    }
    return sameValue(ea1, ea2);
}
