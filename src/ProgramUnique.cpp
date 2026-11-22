// Unique program-local instruction and stream names.
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

bool isExportedSymbol(Program *p, char *name);
const char *getUniqueName(Program *p);
char *mprefix_del(const char *a, const char *b);

bool Program::doUnique()
{
    instStream *stream;
    for (stream = streamHead; stream; stream = stream->nextStream) {
        workTable *scheduleTable = stream->getTable();
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            if (!instruction->branchFlags)
                continue;
            iInfo *instructionInfo = instruction->opcodeInfo;
            if (!instructionInfo) {
                reportError("no inst for a branch\n");
                return false;
            }
            int n = instructionInfo->branchTargetOperandSelector;
            if (!n) {
                reportError("doUnique - branch target operand not set\n");
                exitAfterCleanup(-1);
            }
            Operand *op = &instruction->operands[n - 1];
            if (!op->isInput()) {
                reportError("alleged branch target immediate wasn't an input\n");
                exitAfterCleanup(-1);
            }
            if (op->operandKind == 0x1b) {
                mValue *expressionValue = &op->immediateValue;
                if (expressionValue->type != mValue::STRING) {
                    reportError("doUnique: a branch target isn't a literal string\n");
                    return false;
                }
                char *name = expressionValue->getString();
                if (!name) {
                    reportError("failed to get name\n");
                    return false;
                }
                if (!isExportedSymbol(this, name)) {
                    char *qualifiedName = mprefix_del(getUniqueName(this), name);
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
                char *nn = mprefix_del(getUniqueName(this), macroNode->name);
                if (!nn) {
                    reportError("failed to cat\n");
                    return false;
                }
                if (!macroNode->setName(nn)) {
                    reportError("failed to set new name\n");
                    return false;
                }
            }
        }
    }
    for (mNode *n = uniqueNamePrefixes.head; n; n = n->next) {
        if (!isExportedSymbol(this, n->name)) {
            char *nn = mprefix_del(getUniqueName(this), n->name);
            if (!nn) {
                reportError("failed to cat\n");
                return false;
            }
            if (!n->setName(nn)) {
                reportError("failed to set new name\n");
                return false;
            }
        }
    }
    return true;
}

bool Program::hasStreamLabel(const char *name)
{
    for (instStream *stream = streamHead; stream; stream = stream->nextStream) {
        if (stream->labels.findByNameIgnoreCase(name))
            return true;
    }
    return false;
}

