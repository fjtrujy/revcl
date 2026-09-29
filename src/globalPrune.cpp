// Global prune: remove streams unreachable from the top and redundant branches.
#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

extern int errorOccurred;
extern bool quiet_080bbd98;

int func_0806e530(int flags);

bool globalPrune(Program *p)
{
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_2cc == 2) {
            s->unk_264 = 1;
            s->unk_226 = 1;
        } else {
            s->unk_264 = 0;
            s->unk_226 = 0;
        }
        if (s->unk_124) {
            func_0804acb0("pass thru  - setting reached top for %s\n", s->func_0806d674());
            s->unk_264 = 1;
        }
    }

    bool changed;
    do {
        changed = false;
        for (s = p->unk_a0; s; s = s->unk_108) {
            if (s->unk_226 == 1) {
                s->unk_226 = 2;
                for (linkNode *l = s->unk_134.unk_00; l; l = l->unk_08) {
                    instStream *o = l->unk_0c;
                    if (!o) {
                        func_0804ad38("null outist\n");
                        return false;
                    }
                    o->unk_264 = 1;
                    if (o->unk_226 == 0) {
                        o->unk_226 = 1;
                        changed = true;
                    }
                }
            }
        }
    } while (changed);

    instStream *next;
    for (s = p->unk_a0; s; s = next) {
        next = s->unk_108;
        if (s->unk_264)
            continue;
        func_0804acb0("prune inaccessible stream %s\n", s->func_0806d674());
        workTable *t = s->getTable();
        for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00)
            if (!in->func_08054138()) {
                func_0804acb0("REMOVE INST: ");
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                in->leDump();
                errorOccurred = saved;
            }
        linkNode *l;
        while ((l = s->unk_128.unk_00) != 0) {
            if (!p->unlinkStreams(l->unk_0c, s, l->unk_14)) {
                func_0804ad38("unlink failed for input\n");
                return false;
            }
        }
        while ((l = s->unk_134.unk_00) != 0) {
            if (!p->unlinkStreams(s, l->unk_0c, l->unk_14)) {
                func_0804ad38("unlink failed for output\n");
                return false;
            }
        }
        if (!p->removeStream(s)) {
            func_0804ad38("failed to unlink an ist\n");
            return false;
        }
        delete s;
    }

    instStream *follow;
    for (s = p->unk_a0; s; s = follow) {
        follow = s->unk_108;
        instStream *next = follow;
        if (next && s->unk_2ce && s->func_0806d968() != 4) {
            bool keepLink = false;
            linkNode *l;
            for (l = s->unk_134.unk_00; l; l = l->unk_08) {
                if (l->unk_0c != next)
                    break;
                if (!l->unk_14)
                    keepLink = true;
            }
            if (!l) {
                func_0804acb0("remove branch..\n");
                Instr *br = s->getBranch();
                if (!br) {
                    func_0804ad38("which wasn't there?\n");
                    return false;
                }
                int saved = errorOccurred;
                errorOccurred = !quiet_080bbd98;
                br->func_08054008(false);
                errorOccurred = saved;
                int type = func_0806e530(s->unk_2ce);
                if (!p->unlinkStreams(s, next, type)) {
                    func_0804ad38("failed to unlink the streams through the branch\n");
                    return false;
                }
                if (!s->removeBranch()) {
                    func_0804ad38("failed to remove branch instruction\n");
                    return false;
                }
                if (!keepLink && !p->linkStreams(s, next, 0)) {
                    func_0804ad38("failed to relink two streams following removing an absolute branch\n");
                    return false;
                }
            }
        }
    }
    return true;
}
