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
    instStream *s;
    for (s = streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        for (Instr *i = wt->linearInstructions.head; i; i = i->linearLink.nextInstruction) {
            if (!i->branchFlags)
                continue;
            iInfo *ii = i->opcodeInfo;
            if (!ii) {
                reportError("no inst for a branch\n");
                return false;
            }
            int n = ii->branchTargetOperandSelector;
            if (!n) {
                reportError("doUnique - branch target operand not set\n");
                exitAfterCleanup(-1);
            }
            Operand *op = &i->operands[n - 1];
            if (!op->isInput()) {
                reportError("alleged branch target immediate wasn't an input\n");
                exitAfterCleanup(-1);
            }
            if (op->operandKind == 0x1b) {
                mValue *v = &op->immediateValue;
                if (v->type != mValue::STRING) {
                    reportError("doUnique: a branch target isn't a literal string\n");
                    return false;
                }
                char *name = v->getString();
                if (!name) {
                    reportError("failed to get name\n");
                    return false;
                }
                if (!isExportedSymbol(this, name)) {
                    char *nn = mprefix_del(getUniqueName(this), name);
                    if (!nn) {
                        reportError("failed to cat name\n");
                        return false;
                    }
                    if (!v->setString(nn)) {
                        reportError("failed to set name\n");
                        return false;
                    }
                }
            }
        }
    }
    for (s = streamHead; s; s = s->nextStream) {
        for (mNode *n = s->labels.head; n; n = n->next) {
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
    for (instStream *s = streamHead; s; s = s->nextStream) {
        if (s->labels.findByNameIgnoreCase(name))
            return true;
    }
    return false;
}

