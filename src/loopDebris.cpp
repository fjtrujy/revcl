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
        if (ld->m_epi[i].loopKey == key)
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
    preserveAllocationMode = flag;
    m_entryName = 0;
    m_exitName = 0;
    sourceWorkTable = 0;
    m_nPro = 0;
    m_nEpi = 0;
    requestedAllocationMode = a;
    m_baseName = baseName;
    formatIntoBuffer(buf, "%s_ENTRY_POINT", baseName);
    m_entryName = dupString(buf);
    formatIntoBuffer(buf, "%s_EXIT_POINT", baseName);
    m_exitName = dupString(buf);
    return true;
}

void loopDebris::validateLoopDebrisName()
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
    return r->stream;
}

instStream *loopDebris::getExitStream()
{
    epiRec *r = &m_exit;
    if (!r) {
        reportError("failed to get exit epirec\n");
        return 0;
    }
    return r->stream;
}

instStream *loopDebris::getMainStream()
{
    epiRec *r = &m_main;
    if (!r) {
        reportError("failed to get main epirec\n");
        return 0;
    }
    return r->stream;
}

instStream *loopDebris::getProStream(int i)
{
    epiRec *r = ld_getProRec_inl(this, i);
    if (!r) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return r->stream;
}

instStream *loopDebris::getEpiStream(int i)
{
    epiRec *r = ld_getEpiRec_inl(this, i);
    if (!r) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return r->stream;
}

static inline void setDebrisMode(workTable *wt, unsigned char mode)
{
    wt->setTableRawMode(mode);
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
        r->stream->loopRegionRole = 2;
        if (preserveAllocationMode) {
            setDebrisMode(r->table, 2);
            r->table->allocationMode = requestedAllocationMode;
        } else {
            setDebrisMode(r->table, 1);
            r->table->allocationMode = 2;
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
        r->stream->loopRegionRole = 3;
        if (preserveAllocationMode) {
            setDebrisMode(r->table, 2);
            r->table->allocationMode = requestedAllocationMode;
        } else {
            setDebrisMode(r->table, 1);
            r->table->allocationMode = 2;
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
    m_main.stream->loopRegionRole = 5;
    m_main.table->allocationMode = requestedAllocationMode;
    m_main.table->setTableRawMode(2);
    return &m_main;
}

epiRec *loopDebris::allocEnter(int size)
{
    m_enter.reset();
    if (!m_enter.init(size, m_entryName)) {
        reportError("failed to init for epi\n");
        return 0;
    }
    m_enter.stream->loopRegionRole = 4;
    m_enter.table->allocationMode = 2;
    m_enter.table->setTableRawMode(1);
    return &m_enter;
}

epiRec *loopDebris::allocExit(int size)
{
    m_exit.reset();
    if (!m_exit.init(size, m_exitName)) {
        reportError("failed to init for epi\n");
        return 0;
    }
    m_exit.stream->loopRegionRole = 1;
    m_exit.table->allocationMode = 2;
    m_exit.table->setTableRawMode(1);
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

epiRec *loopDebris::findEpilogueByLoopKey(int key)
{
    for (int i = 0; i != m_nEpi; i++)
        if (m_epi[i].loopKey == key)
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
    return r->stream;
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
    stream = new instStream;
    if (!stream) {
        reportError("setup: failed to get iStream\n");
        exitAfterCleanup(-1);
    }
    mNode *n = stream->labels.findOrAppendByExactName(name);
    if (!n) {
        reportError("failed to add a name to iStream\n");
        exitAfterCleanup(-1);
    }
    workTable *wt = &stream->sourceTable;
    if (!wt->init(3, size)) {
        reportError("failed to init it_pro for size %d\n", size);
        exitAfterCleanup(-1);
    }
    instStream *s = stream;
    int serial = g_streamSerial;
    table = wt;
    s->streamSerial = serial;
    wt->ownerStream = stream;
    stream->workTablePtr = &stream->sourceTable;
    return true;
}

void epiRec::func_08067df0()
{
}

const char *epiRec::getName()
{
    if (!stream)
        return "!UNNAMED!";
    return stream->streamName();
}

int expandOneLoopIntoRegions(Program *p, instStream *s)
{
    workTable *wt = s->getTable();
    if (!wt->getLoopSize()) {
        reportError("BuildEP - loopsize not set in input stream\n");
        return -1;
    }
    if (wt->codeModeValue != 3) {
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
    instStream *enter = ld.m_enter.stream;
    if (!enter) {
        reportError("no entry istream\n");
        exitAfterCleanup(-1);
    }
    instStream *exit = ld.m_exit.stream;
    if (!exit) {
        reportError("had no exit istream\n");
        exitAfterCleanup(-1);
    }
    instStream *fall = s->outputLinks.getStream(0);
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
        instStream *ps = pr->stream;
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
        if (pr->conditionalBranch) {
            epiRec *er = ld_findEpi_inl(&ld, pr->sourceLoopId + 1);
            if (!er) {
                reportError("failed to find epilogue\n");
                return -1;
            }
            instStream *es = er->stream;
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
            next = ld.m_main.stream;
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

    instStream *main = ld.m_main.stream;
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
        succ = e0->stream;
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
        instStream *es = r->stream;
        if (!es) {
            reportError("failed to get istream epi\n");
            return -1;
        }
        int type = 0;
        if (r->conditionalBranch) {
            if (!r->conditionalBranch->changeBranchImmediate(ld.m_exitName)) {
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
        if (!p->insertStreamAfter(last, r->stream)) {
            reportError("failed to add after\n");
            exitAfterCleanup(-1);
        }
        last = r->stream;
    }
    if (!p->insertStreamAfter(last, main)) {
        reportError("failed to add after\n");
        exitAfterCleanup(-1);
    }
    last = main;
    for (i = 0; i != ld.getNumEpi(); i++) {
        r = ld_getEpiRec_inl(&ld, i);
        if (!p->insertStreamAfter(last, r->stream)) {
            reportError("failed to add after\n");
            exitAfterCleanup(-1);
        }
        last = r->stream;
    }
    if (!p->insertStreamAfter(last, exit)) {
        reportError("failed to add after\n");
        exitAfterCleanup(-1);
    }
    if (!enter->labels.transferNodesFrom(&s->labels)) {
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
    ld->sourceWorkTable = 0;
    ld->preserveAllocationMode = flag;
    ld->m_nPro = 0;
    ld->m_nEpi = 0;
    ld->requestedAllocationMode = a;
    ld->m_baseName = baseName;
    formatIntoBuffer(buf, "%s_ENTRY_POINT", baseName);
    ld->m_entryName = dupString(buf);
    formatIntoBuffer(buf, "%s_EXIT_POINT", baseName);
    ld->m_exitName = dupString(buf);
}

static inline const char *recName(epiRec *e)
{
    VclPtrWord name = (VclPtrWord)e->stream;
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
    if (!wt->getLoopSize()) {
        reportError("work table is not looping!\n");
        return false;
    }
    int count = s->knownMinimumLoopCount;
    if (count <= 0 || (count = s->safeMinimumLoopCount) < 0) {
        reportError("ExplodeLoop - called - but - known min/ known safe is bad (%d,%d)\n",
                      s->knownMinimumLoopCount, s->safeMinimumLoopCount);
        return false;
    }
    int mlid = wt->mainLoopId;
    int clid = wt->getCondLid();
    int lsz = wt->getLoopSize();
    bool userFixed = wt->getTableRawMode() == 4;
    initDebris(ld, baseName, (unsigned char)wt->allocationMode, userFixed);
    Instr *cond = wt->getConditionalBranch();
    if (mlid < 0 || clid < 0 || clid > mlid || !cond) {
        reportError("iTable::BuildEPStreams - well - mlid or clid or no conditional ? something was wrong [clid=%d mlid=%d] %x\n",
                      clid, mlid, cond);
        wt->dump();
        return false;
    }
    ld->sourceWorkTable = wt;
    epiRec *r = ld->allocEnter(0);
    if (!r) {
        reportError("failed to get enter part\n");
        return false;
    }
    if (!r->table->prepRawTable()) {
        reportError("failed to prep enter table\n");
        return false;
    }

    count = s->knownMinimumLoopCount;
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
        if (!cloneAppend(wt, pro->table, i, 0, i >= last, true, false)) {
            reportError("failed to clone/append main table to prologue table\n");
            exitAfterCleanup(-1);
        }
        if (i == last || i == mlid - 1) {
            if (!pro->table->prepRawTable()) {
                reportError("failed to prep pro table!\n");
                return false;
            }
            pro->loopKey = 0;
            pro->sourceLoopId = i;
            pro->regionEndIndex = i + 1;
            pro->regionStartIndex = i - clid + 1;
            if (i == last)
                pro->stream->branchType = 5;
            Instr *c = pro->table->getConditionalBranch();
            pro->conditionalBranch = c;
            if (c) {
                pro->stream->branchType = 5;
                c->reverseBranchSense = 1;
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
    if (!cloneAppend(wt, m->table, mlid, 0, true, true, true)) {
        reportError("failed to clone/append main table to loop table\n");
        exitAfterCleanup(-1);
    }
    if (!m->table->prepRawTable()) {
        reportError("failed to prep main table!\n");
        return false;
    }
    m->loopKey = 0;
    m->sourceLoopId = mlid;
    m->regionEndIndex = mlid + 1;
    m->regionStartIndex = mlid + 1;
    m->stream->branchType = 5;
    m->stream->appliedOptimizationMask = s->appliedOptimizationMask;
    m->stream->hasLoopOptimization = s->hasLoopOptimization;
    m->stream->copySourceRecordsFrom(s);
    m->conditionalBranch = m->table->getConditionalBranch();
    if (!m->conditionalBranch) {
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
            e->loopKey = p->sourceLoopId + 1;
            int proBase = p->regionStartIndex;
            int lid = e->loopKey;
            for (int k = clid + 1; k <= mlid; k++, lid++) {
                if (!cloneAppend(wt, e->table, lid, lid - proBase + 1, false, true, false)) {
                    reportError("failed to attach to epi sequence!\n");
                    return false;
                }
            }
        }
    }

    ld->validateLoopDebrisName();
    for (int i = 0; i != ld->m_nPro; i++) {
        epiRec *p = ld_getProRec_inl(ld, i);
        epiRec *next = (i == ld->m_nPro - 1) ? &mainRec(ld) : ld_getProRec_inl(ld, i + 1);
        if (!next) {
            reportError("failed to get next\n");
            exitAfterCleanup(-1);
        }
        instStream *nextStream = next->stream;
        instStream *proStream = p->stream;
        if (!proStream || !nextStream) {
            reportError("no istream set for prologue or next\n");
            exitAfterCleanup(-1);
        }
        Instr *br = p->conditionalBranch;
        if (br) {
            epiRec *e = ld_findEpi_inl(ld, p->sourceLoopId + 1);
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
    Instr *mc = mr->conditionalBranch;
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
        instStream *ist = e->stream;
        if (!ist) {
            reportError("failed to get ist\n");
            exitAfterCleanup(-1);
        }
        workTable *t = e->table;
        if (!t) {
            reportError("no itable for e\n");
            exitAfterCleanup(-1);
        }
        if (i != nEpi - 1) {
            ist->branchType = 1;
            Instr *b = new Instr(findOpcodeInfoByMnemonic("b", false));
            if (!b) {
                reportError("failed to get branch\n");
                return false;
            }
            if (!b->changeBranchImmediate("UNRESOLVED")) {
                reportError("failed to change immediate\n");
                return false;
            }
            b->position.loopIteration = 0;
            b->parsedLoopId = -1;
            b->setOwner((cls_unk30 *)t);
            e->conditionalBranch = b;
            wtRow *row = t->getRowByIndex(t->getRangeEnd() - 2);
            if (!row) {
                reportError("failed to get ie for branch insert\n");
                return false;
            }
            row->entries[1].unk_00 = b;
        }
        if (!e->table->prepRawTable()) {
            reportError("failed to prep epi table!\n");
            return false;
        }
    }

    epiRec *x = ld->allocExit(0);
    if (!x) {
        reportError("failed to get exit part\n");
        return false;
    }
    if (!x->table->prepRawTable()) {
        reportError("failed to prep exit table\n");
        return false;
    }
    return true;
}
