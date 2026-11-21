// Unique program-local instruction and stream names.
#include "linkList.h"
#include "os.h"
#include "instStream.h"
#include "Program.h"

bool func_0806f4e8(Program *p, char *name);
const char *getUniqueName(Program *p);
char *mprefix_del(const char *a, const char *b);

bool Program::doUnique()
{
    instStream *s;
    for (s = unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        for (Instr *i = wt->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
            if (!i->unk_1388)
                continue;
            iInfo *ii = i->unk_34;
            if (!ii) {
                func_0804ad38("no inst for a branch\n");
                return false;
            }
            int n = ii->unk_1a;
            if (!n) {
                func_0804ad38("doUnique - branch target operand not set\n");
                func_0804adf0(-1);
            }
            Operand *op = &i->unk_38[n - 1];
            if (!op->isInput()) {
                func_0804ad38("alleged branch target immediate wasn't an input\n");
                func_0804adf0(-1);
            }
            if (op->unk_04 == 0x1b) {
                mValue *v = &op->unk_1c;
                if (v->type != mValue::STRING) {
                    func_0804ad38("doUnique: a branch target isn't a literal string\n");
                    return false;
                }
                char *name = v->getString();
                if (!name) {
                    func_0804ad38("failed to get name\n");
                    return false;
                }
                if (!func_0806f4e8(this, name)) {
                    char *nn = mprefix_del(getUniqueName(this), name);
                    if (!nn) {
                        func_0804ad38("failed to cat name\n");
                        return false;
                    }
                    if (!v->setString(nn)) {
                        func_0804ad38("failed to set name\n");
                        return false;
                    }
                }
            }
        }
    }
    for (s = unk_a0; s; s = s->unk_108) {
        for (mNode *n = s->unk_110.unk_0c; n; n = n->unk_04) {
            if (!func_0806f4e8(this, n->unk_14)) {
                char *nn = mprefix_del(getUniqueName(this), n->unk_14);
                if (!nn) {
                    func_0804ad38("failed to cat\n");
                    return false;
                }
                if (!n->setName(nn)) {
                    func_0804ad38("failed to set new name\n");
                    return false;
                }
            }
        }
    }
    for (mNode *n = unk_88.unk_0c; n; n = n->unk_04) {
        if (!func_0806f4e8(this, n->unk_14)) {
            char *nn = mprefix_del(getUniqueName(this), n->unk_14);
            if (!nn) {
                func_0804ad38("failed to cat\n");
                return false;
            }
            if (!n->setName(nn)) {
                func_0804ad38("failed to set new name\n");
                return false;
            }
        }
    }
    return true;
}

bool Program::func_0806e878(const char *name)
{
    for (instStream *s = unk_a0; s; s = s->unk_108) {
        if (s->unk_110.func_0804b284(name))
            return true;
    }
    return false;
}

