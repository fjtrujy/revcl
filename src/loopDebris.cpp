#include "loopDebris.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"

char *dupString(const char *s);

static inline epiRec *ld_getProRec_inl(loopDebris *ld, int i)
{
    if (i < 0 || i >= ld->m_nPro)
        return 0;
    return &ld->m_pro[i];
}

static inline epiRec *ld_getEpiRec_inl(loopDebris *ld, int i)
{
    if (i < 0 || i >= ld->m_nEpi)
        return 0;
    return &ld->m_epi[i];
}

static inline epiRec *ld_findEpi_inl(loopDebris *ld, int key)
{
    for (int i = 0; i != ld->m_nEpi; i++)
        if (ld->m_epi[i].unk_08 == key)
            return &ld->m_epi[i];
    return 0;
}

loopDebris::loopDebris() throw()
{
    m_entryName = 0;
    m_exitName = 0;
}

loopDebris::~loopDebris() throw()
{
    if (m_entryName)
        delete [] m_entryName;
    if (m_exitName)
        delete [] m_exitName;
    m_entryName = 0;
    m_exitName = 0;
}

bool loopDebris::init(const char *baseName, int a, bool flag)
{
    char buf[4096];
    if (!baseName) {
        reportError("loopDebris:: init - no base name set!\n");
        return false;
    }
    unk_10 = flag;
    m_entryName = 0;
    m_exitName = 0;
    unk_00 = 0;
    m_nPro = 0;
    m_nEpi = 0;
    unk_14 = a;
    m_baseName = baseName;
    formatIntoBuffer(buf, "%s_ENTRY_POINT", baseName);
    m_entryName = dupString(buf);
    formatIntoBuffer(buf, "%s_EXIT_POINT", baseName);
    m_exitName = dupString(buf);
    return true;
}

void loopDebris::func_0806770c()
{
    int i;
    if (!m_entryName) {
        reportError("LoopDebris - no entry name\n");
        exitAfterCleanup(-1);
    }
    for (i = 0; i != m_nPro; i++)
        ;
    for (i = 0; i != m_nEpi; i++)
        ;
}

instStream *loopDebris::getEnterStream()
{
    epiRec *r = &m_enter;
    if (!r) {
        reportError("failed to get enter epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getExitStream()
{
    epiRec *r = &m_exit;
    if (!r) {
        reportError("failed to get exit epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getMainStream()
{
    epiRec *r = &m_main;
    if (!r) {
        reportError("failed to get main epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getProStream(int i)
{
    epiRec *r = ld_getProRec_inl(this, i);
    if (!r) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getEpiStream(int i)
{
    epiRec *r = ld_getEpiRec_inl(this, i);
    if (!r) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return r->unk_14;
}

static inline void setDebrisMode(workTable *wt, unsigned char mode)
{
    wt->func_08073774(mode);
}

epiRec *loopDebris::allocPro(int size)
{
    char buf[4096];
    epiRec *r = 0;
    if (m_nPro < 16) {
        const char *name = "NOTSET";
        if (m_baseName)
            name = m_baseName;
        formatIntoBuffer(buf, "%s_PRO%d", name, m_nPro);
        r = &m_pro[m_nPro];
        r->reset();
        if (!r->init(size, buf)) {
            reportError("failed to init for pro\n");
            return 0;
        }
        r->unk_14->unk_2c4 = 2;
        if (unk_10) {
            setDebrisMode(r->unk_18, 2);
            r->unk_18->unk_08 = unk_14;
        } else {
            setDebrisMode(r->unk_18, 1);
            r->unk_18->unk_08 = 2;
        }
        m_nPro++;
    } else
        reportError("allocPro: overrun\n");
    return r;
}

epiRec *loopDebris::allocEpi(int size)
{
    char buf[4096];
    epiRec *r = 0;
    if (m_nEpi < 16) {
        const char *name = "NOTSET";
        if (m_baseName)
            name = m_baseName;
        formatIntoBuffer(buf, "%s_EPI%d", name, m_nEpi);
        r = &m_epi[m_nEpi];
        r->reset();
        if (!r->init(size, buf)) {
            reportError("failed to init for epi\n");
            return 0;
        }
        r->unk_14->unk_2c4 = 3;
        if (unk_10) {
            setDebrisMode(r->unk_18, 2);
            r->unk_18->unk_08 = unk_14;
        } else {
            setDebrisMode(r->unk_18, 1);
            r->unk_18->unk_08 = 2;
        }
        m_nEpi++;
    } else
        reportError("getEpi: overrun\n");
    return r;
}

epiRec *loopDebris::allocMain(int size)
{
    char buf[4096];
    m_main.reset();
    const char *name = "NOTSET";
    if (m_baseName)
        name = m_baseName;
    formatIntoBuffer(buf, "%s_MAIN_LOOP", name);
    if (!m_main.init(size, buf)) {
        reportError("failed to init for epi\n");
        return 0;
    }
    m_main.unk_14->unk_2c4 = 5;
    m_main.unk_18->unk_08 = unk_14;
    m_main.unk_18->func_08073774(2);
    return &m_main;
}

epiRec *loopDebris::allocEnter(int size)
{
    m_enter.reset();
    if (!m_enter.init(size, m_entryName)) {
        reportError("failed to init for epi\n");
        return 0;
    }
    m_enter.unk_14->unk_2c4 = 4;
    m_enter.unk_18->unk_08 = 2;
    m_enter.unk_18->func_08073774(1);
    return &m_enter;
}

epiRec *loopDebris::allocExit(int size)
{
    m_exit.reset();
    if (!m_exit.init(size, m_exitName)) {
        reportError("failed to init for epi\n");
        return 0;
    }
    m_exit.unk_14->unk_2c4 = 1;
    m_exit.unk_18->unk_08 = 2;
    m_exit.unk_18->func_08073774(1);
    return &m_exit;
}


epiRec *loopDebris::getProRec(int i)
{
    if (i < 0 || i >= m_nPro)
        return 0;
    return &m_pro[i];
}

epiRec *loopDebris::getEpiRec(int i)
{
    if (i < 0 || i >= m_nEpi)
        return 0;
    return &m_epi[i];
}

epiRec *loopDebris::func_08067c70(int key)
{
    for (int i = 0; i != m_nEpi; i++)
        if (m_epi[i].unk_08 == key)
            return &m_epi[i];
    return 0;
}

#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "loopDebris.h"
#include "epiRec.h"
#include "Instr.h"

extern short g_streamSerial;

const char *getUniqueName(Program *p);
char *dupString(const char *s);
bool explodeLoop(instStream *s, loopDebris *ld, const char *baseName);

inline int loopDebris::getNumPro()
{
    return m_nPro;
}

inline int loopDebris::getNumEpi()
{
    return m_nEpi;
}

static inline instStream *checkedProStream(loopDebris *ld, int i)
{
    epiRec *r = ld_getProRec_inl(ld, i);
    if (!r) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return r->unk_14;
}

static inline epiRec *proRecord(loopDebris *ld, int i, int count)
{
    if (i < 0 || i >= count)
        return 0;
    return &ld->m_pro[i];
}

bool epiRec::init(int size, const char *name)
{
    reset();
    unk_14 = new instStream;
    if (!unk_14) {
        reportError("setup: failed to get iStream\n");
        exitAfterCleanup(-1);
    }
    mNode *n = unk_14->unk_110.findOrAppendByExactName(name);
    if (!n) {
        reportError("failed to add a name to iStream\n");
        exitAfterCleanup(-1);
    }
    workTable *wt = &unk_14->unk_148;
    if (!wt->init(3, size)) {
        reportError("failed to init it_pro for size %d\n", size);
        exitAfterCleanup(-1);
    }
    instStream *s = unk_14;
    int serial = g_streamSerial;
    unk_18 = wt;
    s->unk_2c2 = serial;
    wt->unk_10 = unk_14;
    unk_14->workTablePtr = &unk_14->unk_148;
    return true;
}

void epiRec::func_08067df0()
{
}

const char *epiRec::getName()
{
    if (!unk_14)
        return "!UNNAMED!";
    return unk_14->streamName();
}

int func_08067e18(Program *p, instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt->func_080741b4()) {
        reportError("BuildEP - loopsize not set in input stream\n");
        return -1;
    }
    if (wt->unk_09 != 3) {
        reportError("intput table is not vsm\n");
        return -1;
    }
    int i;
    loopDebris ld;
    char buf[4096];
    formatIntoBuffer(buf, "EXPL_%s_%s_", getUniqueName(p), s->streamName());
    if (!explodeLoop(s, &ld, buf)) {
        reportError("BuildEPSTreams - failed to explode %s\n", s->streamName());
        exitAfterCleanup(-1);
    }
    instStream *enter = ld.m_enter.unk_14;
    if (!enter) {
        reportError("no entry istream\n");
        exitAfterCleanup(-1);
    }
    instStream *exit = ld.m_exit.unk_14;
    if (!exit) {
        reportError("had no exit istream\n");
        exitAfterCleanup(-1);
    }
    instStream *fall = s->unk_134.getStream(0);
    if (!fall) {
        reportError("epipro:failed to get default output stream\n");
        return -1;
    }
    if (fall != s->nextStream) {
        reportError("exit stream not same as loopback successor\n");
        return -1;
    }
    if (!p->unlinkStreams(s, s, 1)) {
        reportError("failed to unlink original loopback\n");
        return -1;
    }
    if (!p->unlinkStreams(s, fall, 0)) {
        reportError("failed to unlink original to exit\n");
        return -1;
    }
    if (!p->retarget(s, enter)) {
        reportError("failed to retarget!\n");
        exitAfterCleanup(-1);
    }

    instStream *prev = enter;
    epiRec *r;
    loopDebris *d;
    for (i = 0; i != (d = &ld)->getNumPro(); i++) {
        epiRec *pr = proRecord(d, i, ld.getNumPro());
        if (!pr) {
            reportError("failed to get prologue\n");
            return -1;
        }
        instStream *ps = pr->unk_14;
        if (!ps) {
            reportError("failed to get stream for prologue\n");
            return -1;
        }
        if (prev) {
            if (!p->linkStreams(prev, ps, 0)) {
                reportError("failed to link enter to first prologue\n");
                return -1;
            }
            prev = 0;
        }
        if (pr->unk_1c) {
            epiRec *er = ld_findEpi_inl(&ld, pr->unk_0c + 1);
            if (!er) {
                reportError("failed to find epilogue\n");
                return -1;
            }
            instStream *es = er->unk_14;
            if (!es) {
                reportError("failed to find epi for a prologue\n");
                return -1;
            }
            if (!p->linkStreams(ps, es, 1)) {
                reportError("failed to link prologue to epilogue\n");
                return -1;
            }
        } else if (i != ld.getNumPro() - 1) {
            reportError("not last prologue, but no conditional\n");
            exitAfterCleanup(-1);
        }
        instStream *next;
        if (i == ld.getNumPro() - 1) {
            next = ld.m_main.unk_14;
        } else {
            next = checkedProStream(&ld, i + 1);
        }
        if (!next) {
            reportError("failed to get next stream for a prologue\n");
            return -1;
        }
        if (!p->linkStreams(ps, next, 0)) {
            reportError("failed to link prologue to fall through\n");
            return -1;
        }
    }

    instStream *main = ld.m_main.unk_14;
    if (!main) {
        reportError("not find new main!\n");
        return -1;
    }
    if (prev && !p->linkStreams(prev, main, 0)) {
        reportError("failed to link enter to main\n");
        return -1;
    }
    if (!p->linkStreams(main, main, 1)) {
        reportError("failed to link loop to itself\n");
        return -1;
    }

    instStream *succ = exit;
    if (ld.getNumEpi()) {
        epiRec *e0 = ld_getEpiRec_inl(&ld, 0);
        if (!e0) {
            reportError("failed to get epi0\n");
            return -1;
        }
        succ = e0->unk_14;
    }
    if (!succ) {
        reportError("no stream for epi0\n");
        return -1;
    }
    if (!p->linkStreams(main, succ, 0)) {
        reportError("failed to link main stream to successor\n");
        return -1;
    }

    for (i = 0; i != ld.getNumEpi(); i++) {
        r = ld_getEpiRec_inl(&ld, i);
        if (!r) {
            reportError("failed to get epi \n");
            return -1;
        }
        instStream *es = r->unk_14;
        if (!es) {
            reportError("failed to get istream epi\n");
            return -1;
        }
        int type = 0;
        if (r->unk_1c) {
            if (!r->unk_1c->changeBranchImmediate(ld.m_exitName)) {
                reportError("failed to change immediate\n");
                exitAfterCleanup(-1);
            }
            type = 1;
        }
        if (!p->linkStreams(es, exit, type)) {
            reportError("failed to link xref from epi to exit\n");
            exitAfterCleanup(-1);
        }
    }
    if (!p->linkStreams(exit, fall, 0)) {
        reportError("failed to link exit to old fall\n");
        return -1;
    }

    instStream *last = s->previousStream;
    if (!p->removeStream(s)) {
        reportError("failed to unlink istream\n");
        exitAfterCleanup(-1);
    }
    if (!p->insertStreamAfter(last, enter)) {
        reportError("failed to add enter after initial  prev\n");
        exitAfterCleanup(-1);
    }

    last = enter;
    for (i = 0; i != ld.getNumPro(); i++) {
        r = ld_getProRec_inl(&ld, i);
        if (!p->insertStreamAfter(last, r->unk_14)) {
            reportError("failed to add after\n");
            exitAfterCleanup(-1);
        }
        last = r->unk_14;
    }
    if (!p->insertStreamAfter(last, main)) {
        reportError("failed to add after\n");
        exitAfterCleanup(-1);
    }
    last = main;
    for (i = 0; i != ld.getNumEpi(); i++) {
        r = ld_getEpiRec_inl(&ld, i);
        if (!p->insertStreamAfter(last, r->unk_14)) {
            reportError("failed to add after\n");
            exitAfterCleanup(-1);
        }
        last = r->unk_14;
    }
    if (!p->insertStreamAfter(last, exit)) {
        reportError("failed to add after\n");
        exitAfterCleanup(-1);
    }
    if (!enter->unk_110.transferNodesFrom(&s->unk_110)) {
        reportError("failed to grab names from the source stream during epipro\n");
        exitAfterCleanup(-1);
    }
    return ld.getNumPro() + ld.getNumEpi() + 3;
}

static inline void initDebris(loopDebris *ld, const char *baseName, int a, bool flag)
{
    char buf[4096];
    if (!baseName) {
        reportError("loopDebris:: init - no base name set!\n");
        return;
    }
    ld->m_entryName = 0;
    ld->m_exitName = 0;
    ld->unk_00 = 0;
    ld->unk_10 = flag;
    ld->m_nPro = 0;
    ld->m_nEpi = 0;
    ld->unk_14 = a;
    ld->m_baseName = baseName;
    formatIntoBuffer(buf, "%s_ENTRY_POINT", baseName);
    ld->m_entryName = dupString(buf);
    formatIntoBuffer(buf, "%s_EXIT_POINT", baseName);
    ld->m_exitName = dupString(buf);
}

static inline const char *recName(epiRec *e)
{
    VclPtrWord name = (VclPtrWord)e->unk_14;
    if (!name)
        name = (VclPtrWord)"!UNNAMED!";
    else
        name = (VclPtrWord)((instStream *)name)->streamName();
    return (const char *)name;
}

static inline epiRec &mainRec(loopDebris *ld)
{
    return ld->m_main;
}

bool explodeLoop(instStream *s, loopDebris *ld, const char *baseName)
{
    if (!baseName) {
        reportError("ExplodeLoop - basename not set\n");
        exitAfterCleanup(-1);
    }
    g_streamSerial++;
    workTable *wt = s->getTable();
    if (!wt->func_080741b4()) {
        reportError("work table is not looping!\n");
        return false;
    }
    int count = s->unk_270;
    if (count <= 0 || (count = s->unk_274) < 0) {
        reportError("ExplodeLoop - called - but - known min/ known safe is bad (%d,%d)\n",
                      s->unk_270, s->unk_274);
        return false;
    }
    int mlid = wt->unk_3c;
    int clid = wt->getCondLid();
    int lsz = wt->func_080741b4();
    bool userFixed = wt->func_08073754() == 4;
    initDebris(ld, baseName, (unsigned char)wt->unk_08, userFixed);
    Instr *cond = wt->func_08073060();
    if (mlid < 0 || clid < 0 || clid > mlid || !cond) {
        reportError("iTable::BuildEPStreams - well - mlid or clid or no conditional ? something was wrong [clid=%d mlid=%d] %x\n",
                      clid, mlid, cond);
        wt->dump();
        return false;
    }
    ld->unk_00 = wt;
    epiRec *r = ld->allocEnter(0);
    if (!r) {
        reportError("failed to get enter part\n");
        return false;
    }
    if (!r->unk_18->prepRawTable()) {
        reportError("failed to prep enter table\n");
        return false;
    }

    count = s->unk_270;
    --count;
    int last = count + clid;
    if (last >= mlid)
        last = mlid;
    epiRec *pro = 0;
    for (int i = 0; i != mlid; i++) {
        if (!pro) {
            pro = ld->allocPro(lsz * (last - i + 1));
            if (!pro) {
                reportError("failed to get pro\n");
                exitAfterCleanup(-1);
            }
        }
        if (!cloneAppend(wt, pro->unk_18, i, 0, i >= last, true, false)) {
            reportError("failed to clone/append main table to prologue table\n");
            exitAfterCleanup(-1);
        }
        if (i == last || i == mlid - 1) {
            if (!pro->unk_18->prepRawTable()) {
                reportError("failed to prep pro table!\n");
                return false;
            }
            pro->unk_08 = 0;
            pro->unk_0c = i;
            pro->unk_00 = i + 1;
            pro->unk_04 = i - clid + 1;
            if (i == last)
                pro->unk_14->unk_2ce = 5;
            Instr *c = pro->unk_18->func_08073060();
            pro->unk_1c = c;
            if (c) {
                pro->unk_14->unk_2ce = 5;
                c->unk_1391 = 1;
            }
            last = i + 1;
            pro = 0;
        }
    }

    epiRec *m = ld->allocMain(lsz);
    if (!m) {
        reportError("failed to get main part\n");
        exitAfterCleanup(-1);
    }
    if (!cloneAppend(wt, m->unk_18, mlid, 0, true, true, true)) {
        reportError("failed to clone/append main table to loop table\n");
        exitAfterCleanup(-1);
    }
    if (!m->unk_18->prepRawTable()) {
        reportError("failed to prep main table!\n");
        return false;
    }
    m->unk_08 = 0;
    m->unk_0c = mlid;
    m->unk_00 = mlid + 1;
    m->unk_04 = mlid + 1;
    m->unk_14->unk_2ce = 5;
    m->unk_14->unk_144 = s->unk_144;
    m->unk_14->unk_140 = s->unk_140;
    m->unk_14->func_0806dc9c(s);
    m->unk_1c = m->unk_18->func_08073060();
    if (!m->unk_1c) {
        reportError("mainloop had no conditional\n");
        exitAfterCleanup(-1);
    }

    int n = lsz * (mlid - clid);
    if (n) {
        for (int j = ld->m_nPro - 1; j >= 0; j--) {
            epiRec *p = ld_getProRec_inl(ld, j);
            if (!p) {
                reportError("failed to get pro %d\n", j);
                exitAfterCleanup(-1);
            }
            epiRec *e = ld->allocEpi(n);
            e->unk_08 = p->unk_0c + 1;
            int proBase = p->unk_04;
            int lid = e->unk_08;
            for (int k = clid + 1; k <= mlid; k++, lid++) {
                if (!cloneAppend(wt, e->unk_18, lid, lid - proBase + 1, false, true, false)) {
                    reportError("failed to attach to epi sequence!\n");
                    return false;
                }
            }
        }
    }

    ld->func_0806770c();
    for (int i = 0; i != ld->m_nPro; i++) {
        epiRec *p = ld_getProRec_inl(ld, i);
        epiRec *next = (i == ld->m_nPro - 1) ? &mainRec(ld) : ld_getProRec_inl(ld, i + 1);
        if (!next) {
            reportError("failed to get next\n");
            exitAfterCleanup(-1);
        }
        instStream *nextStream = next->unk_14;
        instStream *proStream = p->unk_14;
        if (!proStream || !nextStream) {
            reportError("no istream set for prologue or next\n");
            exitAfterCleanup(-1);
        }
        Instr *br = p->unk_1c;
        if (br) {
            epiRec *e = ld_findEpi_inl(ld, p->unk_0c + 1);
            if (!e) {
                reportError("failed to find epilogue\n");
                exitAfterCleanup(-1);
            }
            if (!br->changeBranchImmediate(recName(e))) {
                reportError("failed to change immediate field of main branch\n");
                return false;
            }
        }
    }

    epiRec *mr = &ld->m_main;
    Instr *mc = mr->unk_1c;
    if (!mc) {
        reportError("main had no conditional\n");
        exitAfterCleanup(-1);
    }
    if (!mc->changeBranchImmediate(recName(mr))) {
        reportError("failed to change conditional target\n");
        exitAfterCleanup(-1);
    }

    int nEpi = ld->m_nEpi;
    for (int i = 0; i != nEpi; i++) {
        epiRec *e = ld_getEpiRec_inl(ld, i);
        if (!e) {
            reportError("failed to get epi\n");
            exitAfterCleanup(-1);
        }
        instStream *ist = e->unk_14;
        if (!ist) {
            reportError("failed to get ist\n");
            exitAfterCleanup(-1);
        }
        workTable *t = e->unk_18;
        if (!t) {
            reportError("no itable for e\n");
            exitAfterCleanup(-1);
        }
        if (i != nEpi - 1) {
            ist->unk_2ce = 1;
            Instr *b = new Instr(func_0806bf8c("b", false));
            if (!b) {
                reportError("failed to get branch\n");
                return false;
            }
            if (!b->changeBranchImmediate("UNRESOLVED")) {
                reportError("failed to change immediate\n");
                return false;
            }
            b->unk_13e8.unk_03 = 0;
            b->unk_1420 = -1;
            b->setOwner((cls_unk30 *)t);
            e->unk_1c = b;
            wtRow *row = t->func_080724f0(t->func_080741c0() - 2);
            if (!row) {
                reportError("failed to get ie for branch insert\n");
                return false;
            }
            row->entries[1].unk_00 = b;
        }
        if (!e->unk_18->prepRawTable()) {
            reportError("failed to prep epi table!\n");
            return false;
        }
    }

    epiRec *x = ld->allocExit(0);
    if (!x) {
        reportError("failed to get exit part\n");
        return false;
    }
    if (!x->unk_18->prepRawTable()) {
        reportError("failed to prep exit table\n");
        return false;
    }
    return true;
}
