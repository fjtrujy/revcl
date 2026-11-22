// Unique program-local instruction and stream names.
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

bool isExportedSymbol(Program *p, char *name);
const char *getUniqueName(Program *p);
char *joinNameWithPrefix(const char *a, const char *b);

bool Program::doUnique()
{
    InstructionStream *stream;
    for (stream = streamHead; stream; stream = stream->nextStream) {
        ScheduleTable *scheduleTable = stream->getTable();
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            if (!instruction->branchFlags)
                continue;
            OpcodeDescriptor *instructionInfo = instruction->opcodeInfo;
            if (!instructionInfo) {
                reportError("no inst for a branch\n");
                return false;
            }
            int targetOperandSelector = instructionInfo->branchTargetOperandSelector;
            if (!targetOperandSelector) {
                reportError("doUnique - branch target operand not set\n");
                exitAfterCleanup(-1);
            }
            Operand *branchTargetOperand = &instruction->operands[targetOperandSelector - 1];
            if (!branchTargetOperand->isInput()) {
                reportError("alleged branch target immediate wasn't an input\n");
                exitAfterCleanup(-1);
            }
            if (branchTargetOperand->operandKind == 0x1b) {
                TypedValue *expressionValue = &branchTargetOperand->immediateValue;
                if (expressionValue->type != TypedValue::STRING) {
                    reportError("doUnique: a branch target isn't a literal string\n");
                    return false;
                }
                char *name = expressionValue->getString();
                if (!name) {
                    reportError("failed to get name\n");
                    return false;
                }
                if (!isExportedSymbol(this, name)) {
                    char *qualifiedName = joinNameWithPrefix(getUniqueName(this), name);
                    if (!qualifiedName) {
                        reportError("failed to cat name\n");
                        return false;
                    }
                    if (!expressionValue->setString(qualifiedName)) {
                        reportError("failed to set name\n");
                        return false;
                    }
                }
            }
        }
    }
    for (stream = streamHead; stream; stream = stream->nextStream) {
        for (mNode *macroNode = stream->labels.head; macroNode; macroNode = macroNode->next) {
            if (!isExportedSymbol(this, macroNode->name)) {
                char *qualifiedLabelName = joinNameWithPrefix(getUniqueName(this), macroNode->name);
                if (!qualifiedLabelName) {
                    reportError("failed to cat\n");
                    return false;
                }
                if (!macroNode->setName(qualifiedLabelName)) {
                    reportError("failed to set new name\n");
                    return false;
                }
            }
        }
    }
    for (mNode *prefixNode = uniqueNamePrefixes.head; prefixNode; prefixNode = prefixNode->next) {
        if (!isExportedSymbol(this, prefixNode->name)) {
            char *qualifiedPrefixName = joinNameWithPrefix(getUniqueName(this), prefixNode->name);
            if (!qualifiedPrefixName) {
                reportError("failed to cat\n");
                return false;
            }
            if (!prefixNode->setName(qualifiedPrefixName)) {
                reportError("failed to set new name\n");
                return false;
            }
        }
    }
    return true;
}

bool Program::hasStreamLabel(const char *name)
{
    for (InstructionStream *stream = streamHead; stream; stream = stream->nextStream) {
        if (stream->labels.findByNameIgnoreCase(name))
            return true;
    }
    return false;
}

