#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"

Operand *getEA(Instr *i);
bool sameEA(Instr *a, Instr *b);
bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool func_08065fd4(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

static inline bool fdep_LinkInline(Instr *pre, Instr *post)
{
    if (!pre && !post) {
        reportError("fdep_Link -passed a null ptr\n");
        return false;
    }
    if (!pre->successorDependencies.addInstructionLocation(post)) {
        reportError("failed to add post to the post list\n");
        return false;
    }
    if (!post->predecessorDependencies.addInstructionLocation(pre)) {
        reportError("failed to add pre to the pre list\n");
        return false;
    }
    return true;
}


bool func_0806558c(Instr *i, int *row)
{
    *row = 0;
    for (ol_s *o = i->predecessorDependencies.head; o; o = o->next) {
        Instr *in = o->instruction;
        if (!in) {
            reportError("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->position.unk_00;
        if (r < 0) {
            *row = -1;
            return true;
        }
        if (r + 1 > *row)
            *row = r + 1;
    }
    return true;
}



bool func_080655e8(Instr *i, int *row)
{
    *row = 30000;
    for (ol_s *o = i->successorDependencies.head; o; o = o->next) {
        Instr *in = o->instruction;
        if (!in) {
            reportError("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->position.unk_00;
        if (r >= 0 && r < *row)
            *row = r;
    }
    return true;
}

bool instStream::buildForwardDependencies(bool a, bool b)
{
    if (!workTablePtr) {
        reportWarning("BuildfDepList - no work table set for %s!\n", streamName());
        return false;
    }
    return BuildfDepList(workTablePtr, a, b);
}

bool BuildfDepList(workTable *wt, bool force, bool)
{
    Instr *in;
    Instr *phaseHead;
    in = wt->linearInstructions.head;
    if (!in)
        goto finalize;
    for (; in; in = in->unk_1c.unk_00) {
        in->hasExtraPredecessors = 0;
        in->specialRegisterReadMask = 0;
        in->specialRegisterWriteMask = 0;
        if (!in->predecessorDependencies.clear()) {
            reportError("failed to dispose of existing prefdeplist\n");
            return false;
        }
        if (!in->successorDependencies.clear()) {
            reportError("failed to dispose of existing postfdeplist\n");
            return false;
        }
    }
    {
        phaseHead = wt->linearInstructions.head;
        in = phaseHead;
        if (!phaseHead)
            goto finalize_ready;
        int haveInstruction = phaseHead != 0;
        while (__builtin_expect(haveInstruction, 1)) {
            bool b40 = false;
            bool b10 = false;
            int ramBits = 0;
            Operand *ea = 0;
            bool b20 = false;
            if (in->opcodeFlags & 0x08) {
                reportError("found an instruction barrier fdep - this should be done with --barrier fixup!\n");
                in->leDump();
                return false;
            }
            if (in->opcodeFlags & 0x10)
                b10 = true;
            if (in->opcodeFlags & 0x20)
                b20 = true;
            if (in->opcodeFlags & 0x40)
                b40 = true;
            if (in->ramAccessType == 2) {
                ea = getEA(in);
                if (!ea) {
                    reportError("failed to get ea op for store\n");
                    return false;
                }
                b10 = true;
                b20 = true;
                ramBits = in->getRamBits();
                if (!ramBits)
                    reportWarning("found store instruction with no ram bits\n");
            }
            if (in->ramAccessType == 1) {
                ea = getEA(in);
                if (!ea) {
                    reportError("failed to get ea op for load\n");
                    return false;
                }
                b20 = true;
                ramBits = in->getRamBits();
                if (!ramBits)
                    reportWarning("found load instruction with no ram bits\n");
            }
            if (b20 || b10 || b40) {
                for (Instr *o = in->unk_1c.unk_04; o; o = o->unk_1c.unk_04) {
                    bool dep = false;
                    int type = (unsigned char)o->ramAccessType;
                    if (((b20 && type == 2) || (b10 && type == 1))
                        && (!ea || ((o->getRamBits() & ramBits)
                                    && (force || tagsMatch(o, in) || sameEA(in, o)))))
                        dep = true;
                    else if (b40 && (o->opcodeFlags & 0x40)) {
                        b40 = false;
                        dep = true;
                    } else {
                        if (in->ramAccessType == 2 && (o->opcodeFlags & 0x20))
                            dep = true;
                        if (in->ramAccessType == 1 && (o->opcodeFlags & 0x10))
                            dep = true;
                    }
                    if (dep) {
                        if (!fdep_LinkInline(o, in)) {
                            reportError("failed to add ol for fdeps \n");
                            exitAfterCleanup(-1);
                        }
                    }
                }
            } else if (in->branchLabelTags) {
                reportError("tag string, but no match criteria set\n");
                return false;
            }
            for (int k = 0; k != in->operandCount; k++) {
                Operand *op = &in->operands[k];
                if (op) {
                    RegPoolEntry *r = registerOperand(op, 2);
                    if (r) {
                        if (r->registerIndex == 48)
                            in->specialRegisterWriteMask = op->componentMask;
                        if (r->registerIndex == 48 || r->registerIndex == 49) {
                            for (int p = 0; p != r->partCount; p++)
                                if ((op->componentMask >> p) & 1)
                                    for (ol_s *l = op->parts[p].unk_54.head; l; l = l->next) {
                                        Instr *x = l->instruction;
                                        if (x)
                                            func_08065fd4(in, x);
                                    }
                        }
                    }
                    r = registerOperand(op, 1);
                    if (r && r->registerIndex == 48)
                        in->specialRegisterReadMask = op->componentMask;
                }
            }
            in = in->unk_1c.unk_00;
            haveInstruction = in != 0;
        }
    }
finalize:
    phaseHead = wt->linearInstructions.head;
finalize_ready:
    in = phaseHead;
    Instr *last[4] = { 0, 0, 0, 0 };
    // After entry, phaseHead is only a non-null continuation flag.
    for (; phaseHead; phaseHead = (Instr *)((in = in->unk_1c.unk_00) != 0)) {
        int p;
        for (p = 0; p != 4; p++)
            if (((in->specialRegisterWriteMask >> p) & 1) && !((in->specialRegisterReadMask >> p) & 1))
                last[p] = in;
        for (p = 0; p != 4; p++) {
            if ((in->specialRegisterReadMask >> p) & 1) {
                Instr *w = last[p];
                if (w)
                    for (ol_s *l = in->predecessorDependencies.head; l; l = l->next) {
                        Instr *pre = l->instruction;
                        if (pre->linearOrderIndex < w->linearOrderIndex) {
                            w->hasExtraPredecessors = 1;
                            w->predecessorDependencies.addInstructionLocation(pre);
                        }
                    }
            }
        }
        in->func_08054008(false);
        in->predecessorDependencies.validateOperandLocations();
        in->successorDependencies.validateOperandLocations();
    }
    return true;
}

bool tagsMatch(Instr *a, Instr *b)
{
    char *p = a->branchLabelTags;
    if (!p)
        return false;
    char *q0 = b->branchLabelTags;
    if (!q0)
        return false;
    while (*p) {
        char *q = q0;
        while (*q) {
            char *s = p;
            while (*q && *s && *q != ':' && *s != ':') {
                q++;
                s++;
            }
            if (*q == *s)
                return true;
            while (*q && *q != ':')
                q++;
            if (*q == ':')
                q++;
        }
        while (*p && *p != ':')
            p++;
        if (*p == ':')
            p++;
    }
    return false;
}

bool fixupCloneFdeps(workTable *wt)
{
    int n = 0;
    for (Instr *in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        Instr *orig = in->unk_08;
        n += 8;
        in->linearOrderIndex = n;
        if (!orig) {
            reportError("no original set\n");
            exitAfterCleanup(-1);
        }
        in->hasExtraPredecessors = orig->hasExtraPredecessors;
        if (!copyCloneFdeps(in, orig->predecessorDependencies.head, &in->predecessorDependencies)) {
            reportError("failed to fixup the pre fdep list\n");
            return false;
        }
        if (!copyCloneFdeps(in, orig->successorDependencies.head, &in->successorDependencies)) {
            reportError("failed to fix the post fdeps\n");
            return false;
        }
    }
    return true;
}

bool fdep_Link(Instr *pre, Instr *post)
{
    if (!pre && !post) {
        reportError("fdep_Link -passed a null ptr\n");
        return false;
    }
    if (!pre->successorDependencies.addInstructionLocation(post)) {
        reportError("failed to add post to the post list\n");
        return false;
    }
    if (!post->predecessorDependencies.addInstructionLocation(pre)) {
        reportError("failed to add pre to the pre list\n");
        return false;
    }
    return true;
}

bool copyCloneFdeps(Instr *, ol_s *o, olList *dst)
{
    for (; o; o = o->next) {
        Instr *in = o->instruction;
        if (!in) {
            reportError("qInstruction.. fDepList in_f not set in ol\n");
            return false;
        }
        Instr *clone = in->unk_08;
        if (!clone) {
            reportError("qInstruction - fdep list - clone image not set during rebuild\n");
            return false;
        }
        if (!dst->addInstructionLocation(clone)) {
            reportError("qInstruction - failed to add an OL to the fdep list from clone\n");
            return false;
        }
    }
    return true;
}

bool func_08065fd4(Instr *in, Instr *other)
{
    in->func_08054008(false);
    other->func_08054008(false);
    for (int k = 0; k != other->operandCount; k++) {
        Operand *op = &other->operands[k];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            for (int p = 0; p != r->partCount; p++)
                if ((op->componentMask >> p) & 1)
                    for (ol_s *l = op->parts[p].unk_54.head; l; l = l->next) {
                        Instr *x = l->instruction;
                        if (x && x != in && x->linearOrderIndex < in->linearOrderIndex) {
                            in->hasExtraPredecessors = 1;
                            if (!in->predecessorDependencies.addInstructionLocation(x)) {
                                reportError("failed to add\n");
                                exitAfterCleanup(-1);
                            }
                            x->func_08054008(false);
                        }
                    }
        }
    }
    return true;
}
