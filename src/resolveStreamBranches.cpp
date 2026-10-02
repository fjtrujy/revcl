#include "instStream.h"
#include "flowBlock.h"
#include "Program.h"
#include "fileEmit.h"
#include "Processor.h"

extern int errorOccurred;
extern bool quiet_080bbd98;
void resetRange();
int setRangeFrom(workTable *t);
char *dupString(const char *s);
int func_0806e530(int flags);
bool fixupBtorBranch(Program *p, instStream *s, int *changed, bool btor);
void func_0806ed78(Program *p, int a);


bool resolveStreamBranches(Program *p)
{
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        switch (s->unk_2cd) {
        case 1: {
            unsigned char bt = s->unk_2ce;
            if (!bt || (bt & 4)) {
                if (s->unk_108) {
                    if (!p->linkStreams(s, s->unk_108, 0)) {
                        func_0804ad38("failed to link streams together\n");
                        return false;
                    }
                } else if (!s->unk_124) {
                    func_0804ad38("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                    return false;
                }
            }
            if (bt) {
                if (!((bt >> 1) & 1)) {
                    int type = func_0806e530(bt);
                    char *label = s->getBranchTargetImmediate();
                    if (!label) {
                        func_0804ad38("failed to get branch target immediate\n");
                        return false;
                    }
                    instStream *t = p->findStream(label);
                    if (!t) {
                        func_0804ad84("failed to find label '%s'\n", label);
                        return false;
                    }
                    if (!p->linkStreams(s, t, type)) {
                        func_0804ad38("failed to link streams together\n");
                        return false;
                    }
                } else {
                    Instr *br = s->getBranch();
                    if (!br) {
                        func_0804ad38("failed to get a branch instruction for btor case\n");
                        return false;
                    }
                    if (br->unk_139c) {
                        int type = func_0806e530(s->unk_2ce);
                        char name[2064];
                        for (char *q = br->unk_139c; *q; q++) {
                            char *d = name;
                            char *r = q;
                            while (*r != ':')
                                *d++ = *r++;
                            *d = 0;
                            q = r;
                            instStream *t = p->findStream(name);
                            if (!t) {
                                func_0804ad84("branch-to-register tag, failed to find label '%s'\n", name);
                                return false;
                            }
                            if (!p->linkStreams(s, t, type)) {
                                func_0804ad38("failed to link streams together\n");
                                return false;
                            }
                        }
                    }
                }
            }
            break;
        }
        case 2:
            if (s->unk_2ce) {
                func_0804ad38("branchtype not null for end / cont\n");
                return false;
            }
            if (!s->unk_108) {
                func_0804ad84("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                return false;
            }
            if (!p->linkStreams(s, s->unk_108, 0)) {
                func_0804ad38("failed to link streams together\n");
                return false;
            }
            break;
        case 4:
            if (!s->unk_2d0) {
                func_0804ad38("no macro set in end_macro\n");
                return false;
            }
            /* fall through */
        case 3:
            if (s->unk_2ce) {
                func_0804ad38("branchtype not null for end / cont\n");
                return false;
            }
            break;
        default:
            func_0804ad38("bad end type in stream\n");
            return false;
        }
    }

    bool again;
    do {
        again = false;
        for (s = p->unk_a0; s; s = s->unk_108) {
            bool btor = (s->unk_2ce >> 1) & 1;
            if (btor) {
                int changed = 0;
                if (!fixupBtorBranch(p, s, &changed, btor)) {
                    func_0804ad38("failed to fixup a branch-to-register instruction\n");
                    return false;
                }
                if (changed)
                    again = true;
            }
        }
    } while (again);

    func_0806ed78(p, 0);
    for (s = p->unk_a0; s; s = s->unk_108) {
        Instr *br = s->getBranch();
        if (br && br->unk_139c) {
            delete[] br->unk_139c;
            br->unk_139c = 0;
        }
    }
    return true;
}

bool fixupBtorBranch(Program *p, instStream *s, int *changed, bool btor)
{
    *changed = 0;
    if (((s->unk_2ce >> 1) & 1) == 0) {
        func_0804ad38("balFix:: not reg-target\n");
        return false;
    }
    Instr *br = s->getBranch();
    if (!br) {
        func_0804ad38("balFix-no branch!\n");
        return false;
    }
    RegPoolEntry *reg = br->getBranchTargetRegister();
    if (!reg) {
        func_0804ad38("no target register input for btor\n");
        br->leDump();
        return false;
    }
    br->func_08054008(false);
    instStream *t;
    for (t = p->unk_a0; t; t = t->unk_108)
        t->unk_226 = 0;
    s->unk_226 = 1;
    bool first = true;
    bool again;
    do {
        again = false;
        for (t = p->unk_a0; t; t = t->unk_108) {
            if (t->unk_226 != 1)
                continue;
            workTable *wt = t->getTable();
            Instr *in;
            if (first) {
                in = br->unk_1c.unk_04;
                t->unk_226 = 0;
                first = false;
            } else {
                in = wt->unk_88.unk_04;
                t->unk_226 = 2;
            }
            for (; in; in = in->unk_1c.unk_04) {
                if (in->func_080553b4(reg, 0xff)) {
                    in->func_08054008(false);
                    break;
                }
            }
            if (in) {
                if ((in->unk_1388 >> 3) & 1) {
                    if (!t->unk_108) {
                        func_0804ad84("no physical successor to a branch+link\n");
                        return false;
                    }
                    if (!p->areLinkedBy(s, t->unk_108, 2)) {
                        if (!p->linkStreams(s, t->unk_108, 2)) {
                            func_0804ad38("failed to fixup a branch to register\n");
                            return false;
                        }
                        (*changed)++;
                        again = true;
                    }
                } else if (!br->unk_139c) {
                    func_0804ad38("VCL has found a branch to register at :\n");
                    br->func_08054008(false);
                    func_0804ad38("that's apparently set by:\n");
                    in->func_08054008(false);
                    func_0804ad38("unfortunately vcl can't figure out where the branch should go..\n");
                    func_0804ad38("try adding a tag list to the branch\n");
                    return false;
                }
            } else {
                for (linkNode *n = t->unk_128.unk_00; n; n = n->unk_08) {
                    instStream *o = n->unk_0c;
                    if (!o) {
                        func_0804ad38("null outist\n");
                        return false;
                    }
                    if (!o->unk_226) {
                        o->unk_226 = 1;
                        again = true;
                    }
                }
            }
        }
    } while (again);
    return true;
}
