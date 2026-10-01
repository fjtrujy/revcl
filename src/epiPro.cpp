#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "loopDebris.h"
#include "epiRec.h"
#include "Instr.h"

const char *getUniqueName(Program *p);
bool func_08068840(instStream *s, loopDebris *ld, const char *prefix);

int func_08067e18(Program *p, instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt->func_080741b4()) {
        func_0804ad38("BuildEP - loopsize not set in input stream\n");
        return -1;
    }
    if (wt->unk_09 != 3) {
        func_0804ad38("intput table is not vsm\n");
        return -1;
    }
    int i;
    loopDebris ld;
    char buf[4096];
    func_0804adbc(buf, "EXPL_%s_%s_", getUniqueName(p), s->func_0806d674());
    if (!func_08068840(s, &ld, buf)) {
        func_0804ad38("BuildEPSTreams - failed to explode %s\n", s->func_0806d674());
        func_0804adf0(-1);
    }
    instStream *enter = ld.m_enter.unk_14;
    if (!enter) {
        func_0804ad38("no entry istream\n");
        func_0804adf0(-1);
    }
    instStream *exit = ld.m_exit.unk_14;
    if (!exit) {
        func_0804ad38("had no exit istream\n");
        func_0804adf0(-1);
    }
    instStream *fall = s->unk_134.getStream(0);
    if (!fall) {
        func_0804ad38("epipro:failed to get default output stream\n");
        return -1;
    }
    if (fall != s->unk_108) {
        func_0804ad38("exit stream not same as loopback successor\n");
        return -1;
    }
    if (!p->unlinkStreams(s, s, 1)) {
        func_0804ad38("failed to unlink original loopback\n");
        return -1;
    }
    if (!p->unlinkStreams(s, fall, 0)) {
        func_0804ad38("failed to unlink original to exit\n");
        return -1;
    }
    if (!p->retarget(s, enter)) {
        func_0804ad38("failed to retarget!\n");
        func_0804adf0(-1);
    }

    instStream *prev = enter;
    for (i = 0; i != ld.m_nPro; i++) {
        epiRec *pr = ld.getProRec(i);
        if (!pr) {
            func_0804ad38("failed to get prologue\n");
            return -1;
        }
        instStream *ps = pr->unk_14;
        if (!ps) {
            func_0804ad38("failed to get stream for prologue\n");
            return -1;
        }
        if (prev) {
            if (!p->linkStreams(prev, ps, 0)) {
                func_0804ad38("failed to link enter to first prologue\n");
                return -1;
            }
            prev = 0;
        }
        if (pr->unk_1c) {
            epiRec *er = ld.func_08067c70(pr->unk_0c + 1);
            if (!er) {
                func_0804ad38("failed to find epilogue\n");
                return -1;
            }
            instStream *es = er->unk_14;
            if (!es) {
                func_0804ad38("failed to find epi for a prologue\n");
                return -1;
            }
            if (!p->linkStreams(ps, es, 1)) {
                func_0804ad38("failed to link prologue to epilogue\n");
                return -1;
            }
        } else if (i != ld.m_nPro - 1) {
            func_0804ad38("not last prologue, but no conditional\n");
            func_0804adf0(-1);
        }
        instStream *next;
        if (i == ld.m_nPro - 1) {
            next = ld.m_main.unk_14;
        } else {
            epiRec *r = ld.getProRec(i + 1);
            if (r) {
                next = r->unk_14;
            } else {
                func_0804ad38("failed to get pro epirec\n");
                next = 0;
            }
        }
        if (!next) {
            func_0804ad38("failed to get next stream for a prologue\n");
            return -1;
        }
        if (!p->linkStreams(ps, next, 0)) {
            func_0804ad38("failed to link prologue to fall through\n");
            return -1;
        }
    }

    instStream *main = ld.m_main.unk_14;
    if (!main) {
        func_0804ad38("not find new main!\n");
        return -1;
    }
    if (prev && !p->linkStreams(prev, main, 0)) {
        func_0804ad38("failed to link enter to main\n");
        return -1;
    }
    if (!p->linkStreams(main, main, 1)) {
        func_0804ad38("failed to link loop to itself\n");
        return -1;
    }

    instStream *succ = exit;
    if (ld.m_nEpi) {
        epiRec *e0 = ld.getEpiRec(0);
        if (!e0) {
            func_0804ad38("failed to get epi0\n");
            return -1;
        }
        succ = e0->unk_14;
    }
    if (!succ) {
        func_0804ad38("no stream for epi0\n");
        return -1;
    }
    if (!p->linkStreams(main, succ, 0)) {
        func_0804ad38("failed to link main stream to successor\n");
        return -1;
    }

    for (i = 0; i != ld.m_nEpi; i++) {
        epiRec *er = ld.getEpiRec(i);
        if (!er) {
            func_0804ad38("failed to get epi \n");
            return -1;
        }
        instStream *es = er->unk_14;
        if (!es) {
            func_0804ad38("failed to get istream epi\n");
            return -1;
        }
        int type = 0;
        if (er->unk_1c) {
            if (!er->unk_1c->changeBranchImmediate(ld.m_exitName)) {
                func_0804ad38("failed to change immediate\n");
                func_0804adf0(-1);
            }
            type = 1;
        }
        if (!p->linkStreams(es, exit, type)) {
            func_0804ad38("failed to link xref from epi to exit\n");
            func_0804adf0(-1);
        }
    }
    if (!p->linkStreams(exit, fall, 0)) {
        func_0804ad38("failed to link exit to old fall\n");
        return -1;
    }

    instStream *last = s->unk_10c;
    if (!p->removeStream(s)) {
        func_0804ad38("failed to unlink istream\n");
        func_0804adf0(-1);
    }
    if (!p->insertStreamAfter(last, enter)) {
        func_0804ad38("failed to add enter after initial  prev\n");
        func_0804adf0(-1);
    }

    last = enter;
    for (i = 0; i != ld.m_nPro; i++) {
        epiRec *r = ld.getProRec(i);
        if (!p->insertStreamAfter(last, r->unk_14)) {
            func_0804ad38("failed to add after\n");
            func_0804adf0(-1);
        }
        last = r->unk_14;
    }
    if (!p->insertStreamAfter(last, main)) {
        func_0804ad38("failed to add after\n");
        func_0804adf0(-1);
    }
    last = main;
    for (i = 0; i != ld.m_nEpi; i++) {
        epiRec *r = ld.getEpiRec(i);
        if (!p->insertStreamAfter(last, r->unk_14)) {
            func_0804ad38("failed to add after\n");
            func_0804adf0(-1);
        }
        last = r->unk_14;
    }
    if (!p->insertStreamAfter(last, exit)) {
        func_0804ad38("failed to add after\n");
        func_0804adf0(-1);
    }
    if (!enter->unk_110.func_0804b4d0(&s->unk_110)) {
        func_0804ad38("failed to grab names from the source stream during epipro\n");
        func_0804adf0(-1);
    }
    return ld.m_nPro + ld.m_nEpi + 3;
}
