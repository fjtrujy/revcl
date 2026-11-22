#include "os.h"
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
bool fixupBtorBranch(Program *p, instStream *s, int *changed);
void func_0806ed78(Program *p, int a);


bool resolveStreamBranches(Program *p)
{
    instStream *s;
    for (s = p->streamHead; s; s = s->unk_108) {
        switch (s->unk_2cd) {
        case 1: {
            unsigned char bt = s->unk_2ce;
            int headFlags = bt;
            if (!(unsigned char)headFlags || (unsigned char)(headFlags & 4)) {
                if (s->unk_108) {
                    if (!p->linkStreams(s, s->unk_108, 0)) {
                        reportError("failed to link streams together\n");
                        return false;
                    }
                    bt = s->unk_2ce;
                } else if (!s->unk_124) {
                    reportError("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                    return false;
                }
            }
            int flags = bt;
            if ((unsigned char)flags) {
                int btorMask = 1;
                int btor = (bt >> 1) & btorMask;
                if (!(unsigned char)btor) {
                    int type = func_0806e530(flags);
                    char *label = s->getBranchTargetImmediate();
                    if (!label) {
                        reportError("failed to get branch target immediate\n");
                        return false;
                    }
                    instStream *t = p->findStream(label);
                    if (!t) {
                        reportUserMessage("failed to find label '%s'\n", label);
                        return false;
                    }
                    if (!p->linkStreams(s, t, type)) {
                        reportError("failed to link streams together\n");
                        return false;
                    }
                } else {
                    Instr *br = s->getBranch();
                    if (!br) {
                        reportError("failed to get a branch instruction for btor case\n");
                        return false;
                    }
                    if (br->unk_139c) {
                        int type = func_0806e530(s->unk_2ce);
                        char name[2064];
                        for (char *q = br->unk_139c; *q; q++) {
                            char *r = q;
                            char *d = name;
                            while (*r != ':')
                                *d++ = *r++;
                            *d = 0;
                            q = r;
                            instStream *t = p->findStream(name);
                            if (!t) {
                                reportUserMessage("branch-to-register tag, failed to find label '%s'\n", name);
                                return false;
                            }
                            if (!p->linkStreams(s, t, type)) {
                                reportError("failed to link streams together\n");
                                return false;
                            }
                        }
                    }
                }
            }
            break;
        }
        case 4:
            if (!s->unk_2d0) {
                reportError("no macro set in end_macro\n");
                return false;
            }
            /* fall through */
        case 3:
            if (s->unk_2ce) {
                reportError("branchtype not null for end / cont\n");
                return false;
            }
            break;
        case 2:
            if (s->unk_2ce) {
                reportError("branchtype not null for end / cont\n");
                return false;
            }
            if (!s->unk_108) {
                reportUserMessage("fell off the bottom of code! .. last stream should either end in END or be a branch\n");
                return false;
            }
            if (!p->linkStreams(s, s->unk_108, 0)) {
                reportError("failed to link streams together\n");
                return false;
            }
            break;
        default:
            reportError("bad end type in stream\n");
            return false;
        }
    }

    bool again;
    do {
        again = false;
        for (s = p->streamHead; s; s = s->unk_108) {
            int btorMask = 1;
            int btor = (s->unk_2ce >> 1) & btorMask;
            if ((unsigned char)btor) {
                int changed = 0;
                if (!fixupBtorBranch(p, s, &changed)) {
                    reportError("failed to fixup a branch-to-register instruction\n");
                    return false;
                }
                if (changed)
                    again = true;
            }
        }
    } while (again);

    func_0806ed78(p, 0);
    for (s = p->streamHead; s; s = s->unk_108) {
        Instr *br = s->getBranch();
        if (br && br->unk_139c) {
            delete[] br->unk_139c;
            br->unk_139c = 0;
        }
    }
    return true;
}

bool fixupBtorBranch(Program *p, instStream *s, int *changed)
{
    *changed = 0;
    if (((s->unk_2ce >> 1) & 1) == 0) {
        reportError("balFix:: not reg-target\n");
        return false;
    }
    Instr *br = s->getBranch();
    if (!br) {
        reportError("balFix-no branch!\n");
        return false;
    }
    RegPoolEntry *reg = br->getBranchTargetRegister();
    if (!reg) {
        reportError("no target register input for btor\n");
        br->leDump();
        return false;
    }
    br->func_08054008(false);
    instStream *t;
    for (t = p->streamHead; t; t = t->unk_108)
        t->unk_226 = 0;
    s->unk_226 = 1;
    bool first = true;
    bool again;
    do {
        again = false;
        for (t = p->streamHead; t; t = t->unk_108) {
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
                        reportUserMessage("no physical successor to a branch+link\n");
                        return false;
                    }
                    if (!p->areLinkedBy(s, t->unk_108, 2)) {
                        if (!p->linkStreams(s, t->unk_108, 2)) {
                            reportError("failed to fixup a branch to register\n");
                            return false;
                        }
                        (*changed)++;
                        again = true;
                    }
                } else if (!br->unk_139c) {
                    reportError("VCL has found a branch to register at :\n");
                    br->func_08054008(false);
                    reportError("that's apparently set by:\n");
                    in->func_08054008(false);
                    reportError("unfortunately vcl can't figure out where the branch should go..\n");
                    reportError("try adding a tag list to the branch\n");
                    return false;
                }
            } else {
                for (linkNode *n = t->unk_128.unk_00; n; n = n->unk_08) {
                    instStream *o = n->unk_0c;
                    if (!o) {
                        reportError("null outist\n");
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
