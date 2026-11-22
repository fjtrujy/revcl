#include "loopDebris.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"

char *dupString(const char *s);

static inline epiRec *getPrologueRecordByIndex(loopDebris *loopRecord, int prologueIndex)
{
    if (prologueIndex < 0 || prologueIndex >= loopRecord->m_nPro)
        return 0;
    return &loopRecord->m_pro[prologueIndex];
}

static inline epiRec *getEpilogueRecordByIndex(loopDebris *loopRecord, int epilogueIndex)
{
    if (epilogueIndex < 0 || epilogueIndex >= loopRecord->m_nEpi)
        return 0;
    return &loopRecord->m_epi[epilogueIndex];
}

static inline epiRec *ld_findEpi_inl(loopDebris *loopRecord, int key)
{
    for (int epilogueIndex = 0; epilogueIndex != loopRecord->m_nEpi; epilogueIndex++)
        if (loopRecord->m_epi[epilogueIndex].loopKey == key)
            return &loopRecord->m_epi[epilogueIndex];
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
    epiRec *region = &m_enter;
    if (!region) {
        reportError("failed to get enter epirec\n");
        return 0;
    }
    return region->stream;
}

instStream *loopDebris::getExitStream()
{
    epiRec *region = &m_exit;
    if (!region) {
        reportError("failed to get exit epirec\n");
        return 0;
    }
    return region->stream;
}

instStream *loopDebris::getMainStream()
{
    epiRec *region = &m_main;
    if (!region) {
        reportError("failed to get main epirec\n");
        return 0;
    }
    return region->stream;
}

instStream *loopDebris::getProStream(int prologueIndex)
{
    epiRec *region = getPrologueRecordByIndex(this, prologueIndex);
    if (!region) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return region->stream;
}

instStream *loopDebris::getEpiStream(int epilogueIndex)
{
    epiRec *region = getEpilogueRecordByIndex(this, epilogueIndex);
    if (!region) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return region->stream;
}

static inline void setDebrisMode(workTable *table, unsigned char mode)
{
    table->setTableRawMode(mode);
}

epiRec *loopDebris::allocPro(int size)
{
    char buf[4096];
    epiRec *region = 0;
    if (m_nPro < 16) {
        const char *name = "NOTSET";
        if (m_baseName)
            name = m_baseName;
        formatIntoBuffer(buf, "%s_PRO%d", name, m_nPro);
        region = &m_pro[m_nPro];
        region->reset();
        if (!region->init(size, buf)) {
            reportError("failed to init for pro\n");
            return 0;
        }
        region->stream->loopRegionRole = 2;
        if (preserveAllocationMode) {
            setDebrisMode(region->table, 2);
            region->table->allocationMode = requestedAllocationMode;
        } else {
            setDebrisMode(region->table, 1);
            region->table->allocationMode = 2;
        }
        m_nPro++;
    } else
        reportError("allocPro: overrun\n");
    return region;
}

epiRec *loopDebris::allocEpi(int size)
{
    char buf[4096];
    epiRec *region = 0;
    if (m_nEpi < 16) {
        const char *name = "NOTSET";
        if (m_baseName)
            name = m_baseName;
        formatIntoBuffer(buf, "%s_EPI%d", name, m_nEpi);
        region = &m_epi[m_nEpi];
        region->reset();
        if (!region->init(size, buf)) {
            reportError("failed to init for epi\n");
            return 0;
        }
        region->stream->loopRegionRole = 3;
        if (preserveAllocationMode) {
            setDebrisMode(region->table, 2);
            region->table->allocationMode = requestedAllocationMode;
        } else {
            setDebrisMode(region->table, 1);
            region->table->allocationMode = 2;
        }
        m_nEpi++;
    } else
        reportError("getEpi: overrun\n");
    return region;
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


epiRec *loopDebris::getProRec(int prologueIndex)
{
    if (prologueIndex < 0 || prologueIndex >= m_nPro)
        return 0;
    return &m_pro[prologueIndex];
}

epiRec *loopDebris::getEpiRec(int epilogueIndex)
{
    if (epilogueIndex < 0 || epilogueIndex >= m_nEpi)
        return 0;
    return &m_epi[epilogueIndex];
}

epiRec *loopDebris::findEpilogueByLoopKey(int key)
{
    for (int epilogueIndex = 0; epilogueIndex != m_nEpi; epilogueIndex++)
        if (m_epi[epilogueIndex].loopKey == key)
            return &m_epi[epilogueIndex];
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

inline int loopDebris::getPrologueRegionCount()
{
    return m_nPro;
}

inline int loopDebris::getEpilogueRegionCount()
{
    return m_nEpi;
}

static inline instStream *checkedProStream(loopDebris *loopRecord, int prologueIndex)
{
    epiRec *region = getPrologueRecordByIndex(loopRecord, prologueIndex);
    if (!region) {
        reportError("failed to get pro epirec\n");
        return 0;
    }
    return region->stream;
}

static inline epiRec *proRecord(loopDebris *loopRecord, int recordIndex, int count)
{
    if (recordIndex < 0 || recordIndex >= count)
        return 0;
    return &loopRecord->m_pro[recordIndex];
}

bool epiRec::init(int size, const char *name)
{
    reset();
    stream = new instStream;
    if (!stream) {
        reportError("setup: failed to get iStream\n");
        exitAfterCleanup(-1);
    }
    mNode *macroNode = stream->labels.findOrAppendByExactName(name);
    if (!macroNode) {
        reportError("failed to add a name to iStream\n");
        exitAfterCleanup(-1);
    }
    workTable *scheduleTable = &stream->sourceTable;
    if (!scheduleTable->init(3, size)) {
        reportError("failed to init it_pro for size %d\n", size);
        exitAfterCleanup(-1);
    }
    instStream *ownerStream = stream;
    int serial = g_streamSerial;
    table = scheduleTable;
    ownerStream->streamSerial = serial;
    scheduleTable->ownerStream = stream;
    stream->workTablePtr = &stream->sourceTable;
    return true;
}

void epiRec::noopLoopRegionHook()
{
}

const char *epiRec::getName()
{
    if (!stream)
        return "!UNNAMED!";
    return stream->streamName();
}

int expandOneLoopIntoRegions(Program *program, instStream *stream)
{
    workTable *table = stream->getTable();
    if (!table->getLoopSize()) {
        reportError("BuildEP - loopsize not set in input stream\n");
        return -1;
    }
    if (table->codeModeValue != 3) {
        reportError("intput table is not vsm\n");
        return -1;
    }
    int regionIndex;
    loopDebris loopRegions;
    char buf[4096];
    formatIntoBuffer(buf, "EXPL_%s_%s_", getUniqueName(program), stream->streamName());
    if (!explodeLoop(stream, &loopRegions, buf)) {
        reportError("BuildEPSTreams - failed to explode %s\n", stream->streamName());
        exitAfterCleanup(-1);
    }
    instStream *enter = loopRegions.m_enter.stream;
    if (!enter) {
        reportError("no entry istream\n");
        exitAfterCleanup(-1);
    }
    instStream *exit = loopRegions.m_exit.stream;
    if (!exit) {
        reportError("had no exit istream\n");
        exitAfterCleanup(-1);
    }
    instStream *fall = stream->outputLinks.getStream(0);
    if (!fall) {
        reportError("epipro:failed to get default output stream\n");
        return -1;
    }
    if (fall != stream->nextStream) {
        reportError("exit stream not same as loopback successor\n");
        return -1;
    }
    if (!program->unlinkStreams(stream, stream, 1)) {
        reportError("failed to unlink original loopback\n");
        return -1;
    }
    if (!program->unlinkStreams(stream, fall, 0)) {
        reportError("failed to unlink original to exit\n");
        return -1;
    }
    if (!program->retarget(stream, enter)) {
        reportError("failed to retarget!\n");
        exitAfterCleanup(-1);
    }

    instStream *prev = enter;
    epiRec *region;
    loopDebris *loopRecord;
    for (regionIndex = 0; regionIndex != (loopRecord = &loopRegions)->getPrologueRegionCount(); regionIndex++) {
        epiRec *prologueRegion = proRecord(loopRecord, regionIndex, loopRegions.getPrologueRegionCount());
        if (!prologueRegion) {
            reportError("failed to get prologue\n");
            return -1;
        }
        instStream *prologueStream = prologueRegion->stream;
        if (!prologueStream) {
            reportError("failed to get stream for prologue\n");
            return -1;
        }
        if (prev) {
            if (!program->linkStreams(prev, prologueStream, 0)) {
                reportError("failed to link enter to first prologue\n");
                return -1;
            }
            prev = 0;
        }
        if (prologueRegion->conditionalBranch) {
            epiRec *epilogueRegion = ld_findEpi_inl(&loopRegions, prologueRegion->sourceLoopId + 1);
            if (!epilogueRegion) {
                reportError("failed to find epilogue\n");
                return -1;
            }
            instStream *epilogueStream = epilogueRegion->stream;
            if (!epilogueStream) {
                reportError("failed to find epi for a prologue\n");
                return -1;
            }
            if (!program->linkStreams(prologueStream, epilogueStream, 1)) {
                reportError("failed to link prologue to epilogue\n");
                return -1;
            }
        } else if (regionIndex != loopRegions.getPrologueRegionCount() - 1) {
            reportError("not last prologue, but no conditional\n");
            exitAfterCleanup(-1);
        }
        instStream *next;
        if (regionIndex == loopRegions.getPrologueRegionCount() - 1) {
            next = loopRegions.m_main.stream;
        } else {
            next = checkedProStream(&loopRegions, regionIndex + 1);
        }
        if (!next) {
            reportError("failed to get next stream for a prologue\n");
            return -1;
        }
        if (!program->linkStreams(prologueStream, next, 0)) {
            reportError("failed to link prologue to fall through\n");
            return -1;
        }
    }

    instStream *main = loopRegions.m_main.stream;
    if (!main) {
        reportError("not find new main!\n");
        return -1;
    }
    if (prev && !program->linkStreams(prev, main, 0)) {
        reportError("failed to link enter to main\n");
        return -1;
    }
    if (!program->linkStreams(main, main, 1)) {
        reportError("failed to link loop to itself\n");
        return -1;
    }

    instStream *succ = exit;
    if (loopRegions.getEpilogueRegionCount()) {
        epiRec *e0 = getEpilogueRecordByIndex(&loopRegions, 0);
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
    if (!program->linkStreams(main, succ, 0)) {
        reportError("failed to link main stream to successor\n");
        return -1;
    }

    for (regionIndex = 0; regionIndex != loopRegions.getEpilogueRegionCount(); regionIndex++) {
        region = getEpilogueRecordByIndex(&loopRegions, regionIndex);
        if (!region) {
            reportError("failed to get epi \n");
            return -1;
        }
        instStream *epilogueStream = region->stream;
        if (!epilogueStream) {
            reportError("failed to get istream epi\n");
            return -1;
        }
        int type = 0;
        if (region->conditionalBranch) {
            if (!region->conditionalBranch->changeBranchImmediate(loopRegions.m_exitName)) {
                reportError("failed to change immediate\n");
                exitAfterCleanup(-1);
            }
            type = 1;
        }
        if (!program->linkStreams(epilogueStream, exit, type)) {
            reportError("failed to link xref from epi to exit\n");
            exitAfterCleanup(-1);
        }
    }
    if (!program->linkStreams(exit, fall, 0)) {
        reportError("failed to link exit to old fall\n");
        return -1;
    }

    instStream *last = stream->previousStream;
    if (!program->removeStream(stream)) {
        reportError("failed to unlink istream\n");
        exitAfterCleanup(-1);
    }
    if (!program->insertStreamAfter(last, enter)) {
        reportError("failed to add enter after initial  prev\n");
        exitAfterCleanup(-1);
    }

    last = enter;
    for (regionIndex = 0; regionIndex != loopRegions.getPrologueRegionCount(); regionIndex++) {
        region = getPrologueRecordByIndex(&loopRegions, regionIndex);
        if (!program->insertStreamAfter(last, region->stream)) {
            reportError("failed to add after\n");
            exitAfterCleanup(-1);
        }
        last = region->stream;
    }
    if (!program->insertStreamAfter(last, main)) {
        reportError("failed to add after\n");
        exitAfterCleanup(-1);
    }
    last = main;
    for (regionIndex = 0; regionIndex != loopRegions.getEpilogueRegionCount(); regionIndex++) {
        region = getEpilogueRecordByIndex(&loopRegions, regionIndex);
        if (!program->insertStreamAfter(last, region->stream)) {
            reportError("failed to add after\n");
            exitAfterCleanup(-1);
        }
        last = region->stream;
    }
    if (!program->insertStreamAfter(last, exit)) {
        reportError("failed to add after\n");
        exitAfterCleanup(-1);
    }
    if (!enter->labels.transferNodesFrom(&stream->labels)) {
        reportError("failed to grab names from the source stream during epipro\n");
        exitAfterCleanup(-1);
    }
    return loopRegions.getPrologueRegionCount() + loopRegions.getEpilogueRegionCount() + 3;
}

static inline void initDebris(loopDebris *loopRecord, const char *baseName, int a, bool flag)
{
    char buf[4096];
    if (!baseName) {
        reportError("loopDebris:: init - no base name set!\n");
        return;
    }
    loopRecord->m_entryName = 0;
    loopRecord->m_exitName = 0;
    loopRecord->sourceWorkTable = 0;
    loopRecord->preserveAllocationMode = flag;
    loopRecord->m_nPro = 0;
    loopRecord->m_nEpi = 0;
    loopRecord->requestedAllocationMode = a;
    loopRecord->m_baseName = baseName;
    formatIntoBuffer(buf, "%s_ENTRY_POINT", baseName);
    loopRecord->m_entryName = dupString(buf);
    formatIntoBuffer(buf, "%s_EXIT_POINT", baseName);
    loopRecord->m_exitName = dupString(buf);
}

static inline const char *recName(epiRec *epilogueRegion)
{
    VclPtrWord name = (VclPtrWord)epilogueRegion->stream;
    if (!name)
        name = (VclPtrWord)"!UNNAMED!";
    else
        name = (VclPtrWord)((instStream *)name)->streamName();
    return (const char *)name;
}

static inline epiRec &mainRec(loopDebris *loopRecord)
{
    return loopRecord->m_main;
}

bool explodeLoop(instStream *stream, loopDebris *loopRecord, const char *baseName)
{
    if (!baseName) {
        reportError("ExplodeLoop - basename not set\n");
        exitAfterCleanup(-1);
    }
    g_streamSerial++;
    workTable *scheduleTable = stream->getTable();
    if (!scheduleTable->getLoopSize()) {
        reportError("work table is not looping!\n");
        return false;
    }
    int count = stream->knownMinimumLoopCount;
    if (count <= 0 || (count = stream->safeMinimumLoopCount) < 0) {
        reportError("ExplodeLoop - called - but - known min/ known safe is bad (%d,%d)\n",
                      stream->knownMinimumLoopCount, stream->safeMinimumLoopCount);
        return false;
    }
    int mlid = scheduleTable->mainLoopId;
    int clid = scheduleTable->getTerminatingConditionalLoopId();
    int lsz = scheduleTable->getLoopSize();
    bool userFixed = scheduleTable->getTableRawMode() == 4;
    initDebris(loopRecord, baseName, (unsigned char)scheduleTable->allocationMode, userFixed);
    Instr *cond = scheduleTable->getConditionalBranch();
    if (mlid < 0 || clid < 0 || clid > mlid || !cond) {
        reportError("iTable::BuildEPStreams - well - mlid or clid or no conditional ? something was wrong [clid=%d mlid=%d] %x\n",
                      clid, mlid, cond);
        scheduleTable->dump();
        return false;
    }
    loopRecord->sourceWorkTable = scheduleTable;
    epiRec *region = loopRecord->allocEnter(0);
    if (!region) {
        reportError("failed to get enter part\n");
        return false;
    }
    if (!region->table->prepRawTable()) {
        reportError("failed to prep enter table\n");
        return false;
    }

    count = stream->knownMinimumLoopCount;
    --count;
    int last = count + clid;
    if (last >= mlid)
        last = mlid;
    epiRec *pro = 0;
    for (int prologueIndex = 0; prologueIndex != mlid; prologueIndex++) {
        if (!pro) {
            pro = loopRecord->allocPro(lsz * (last - prologueIndex + 1));
            if (!pro) {
                reportError("failed to get pro\n");
                exitAfterCleanup(-1);
            }
        }
        if (!cloneAppend(scheduleTable, pro->table, prologueIndex, 0, prologueIndex >= last, true, false)) {
            reportError("failed to clone/append main table to prologue table\n");
            exitAfterCleanup(-1);
        }
        if (prologueIndex == last || prologueIndex == mlid - 1) {
            if (!pro->table->prepRawTable()) {
                reportError("failed to prep pro table!\n");
                return false;
            }
            pro->loopKey = 0;
            pro->sourceLoopId = prologueIndex;
            pro->regionEndIndex = prologueIndex + 1;
            pro->regionStartIndex = prologueIndex - clid + 1;
            if (prologueIndex == last)
                pro->stream->branchType = 5;
            Instr *cloneInstruction = pro->table->getConditionalBranch();
            pro->conditionalBranch = cloneInstruction;
            if (cloneInstruction) {
                pro->stream->branchType = 5;
                cloneInstruction->reverseBranchSense = 1;
            }
            last = prologueIndex + 1;
            pro = 0;
        }
    }

    epiRec *mainRegion = loopRecord->allocMain(lsz);
    if (!mainRegion) {
        reportError("failed to get main part\n");
        exitAfterCleanup(-1);
    }
    if (!cloneAppend(scheduleTable, mainRegion->table, mlid, 0, true, true, true)) {
        reportError("failed to clone/append main table to loop table\n");
        exitAfterCleanup(-1);
    }
    if (!mainRegion->table->prepRawTable()) {
        reportError("failed to prep main table!\n");
        return false;
    }
    mainRegion->loopKey = 0;
    mainRegion->sourceLoopId = mlid;
    mainRegion->regionEndIndex = mlid + 1;
    mainRegion->regionStartIndex = mlid + 1;
    mainRegion->stream->branchType = 5;
    mainRegion->stream->appliedOptimizationMask = stream->appliedOptimizationMask;
    mainRegion->stream->hasLoopOptimization = stream->hasLoopOptimization;
    mainRegion->stream->copySourceRecordsFrom(stream);
    mainRegion->conditionalBranch = mainRegion->table->getConditionalBranch();
    if (!mainRegion->conditionalBranch) {
        reportError("mainloop had no conditional\n");
        exitAfterCleanup(-1);
    }

    int rowOffset = lsz * (mlid - clid);
    if (rowOffset) {
        for (int reversePrologueIndex = loopRecord->m_nPro - 1; reversePrologueIndex >= 0; reversePrologueIndex--) {
            epiRec *prologueRegion = getPrologueRecordByIndex(loopRecord, reversePrologueIndex);
            if (!prologueRegion) {
                reportError("failed to get pro %d\n", reversePrologueIndex);
                exitAfterCleanup(-1);
            }
            epiRec *epilogueRegion = loopRecord->allocEpi(rowOffset);
            epilogueRegion->loopKey = prologueRegion->sourceLoopId + 1;
            int proBase = prologueRegion->regionStartIndex;
            int lid = epilogueRegion->loopKey;
            for (int loopId = clid + 1; loopId <= mlid; loopId++, lid++) {
                if (!cloneAppend(scheduleTable, epilogueRegion->table, lid, lid - proBase + 1, false, true, false)) {
                    reportError("failed to attach to epi sequence!\n");
                    return false;
                }
            }
        }
    }

    loopRecord->validateLoopDebrisName();
    for (int primaryPrologueIndex = 0; primaryPrologueIndex != loopRecord->m_nPro; primaryPrologueIndex++) {
        epiRec *prologueRegion = getPrologueRecordByIndex(loopRecord, primaryPrologueIndex);
        epiRec *next = (primaryPrologueIndex == loopRecord->m_nPro - 1) ? &mainRec(loopRecord) : getPrologueRecordByIndex(loopRecord, primaryPrologueIndex + 1);
        if (!next) {
            reportError("failed to get next\n");
            exitAfterCleanup(-1);
        }
        instStream *nextStream = next->stream;
        instStream *proStream = prologueRegion->stream;
        if (!proStream || !nextStream) {
            reportError("no istream set for prologue or next\n");
            exitAfterCleanup(-1);
        }
        Instr *branchInstruction = prologueRegion->conditionalBranch;
        if (branchInstruction) {
            epiRec *epilogueRegion = ld_findEpi_inl(loopRecord, prologueRegion->sourceLoopId + 1);
            if (!epilogueRegion) {
                reportError("failed to find epilogue\n");
                exitAfterCleanup(-1);
            }
            if (!branchInstruction->changeBranchImmediate(recName(epilogueRegion))) {
                reportError("failed to change immediate field of main branch\n");
                return false;
            }
        }
    }

    epiRec *epilogueRecord = &loopRecord->m_main;
    Instr *moveCloneInstruction = epilogueRecord->conditionalBranch;
    if (!moveCloneInstruction) {
        reportError("main had no conditional\n");
        exitAfterCleanup(-1);
    }
    if (!moveCloneInstruction->changeBranchImmediate(recName(epilogueRecord))) {
        reportError("failed to change conditional target\n");
        exitAfterCleanup(-1);
    }

    int nEpi = loopRecord->m_nEpi;
    for (int epilogueIndex = 0; epilogueIndex != nEpi; epilogueIndex++) {
        epiRec *epilogueRegion = getEpilogueRecordByIndex(loopRecord, epilogueIndex);
        if (!epilogueRegion) {
            reportError("failed to get epi\n");
            exitAfterCleanup(-1);
        }
        instStream *ist = epilogueRegion->stream;
        if (!ist) {
            reportError("failed to get ist\n");
            exitAfterCleanup(-1);
        }
        workTable *table = epilogueRegion->table;
        if (!table) {
            reportError("no itable for e\n");
            exitAfterCleanup(-1);
        }
        if (epilogueIndex != nEpi - 1) {
            ist->branchType = 1;
            Instr *secondInstruction = new Instr(findOpcodeInfoByMnemonic("b", false));
            if (!secondInstruction) {
                reportError("failed to get branch\n");
                return false;
            }
            if (!secondInstruction->changeBranchImmediate("UNRESOLVED")) {
                reportError("failed to change immediate\n");
                return false;
            }
            secondInstruction->position.loopIteration = 0;
            secondInstruction->parsedLoopId = -1;
            secondInstruction->setOwner((cls_unk30 *)table);
            epilogueRegion->conditionalBranch = secondInstruction;
            wtRow *row = table->getRowByIndex(table->getRangeEnd() - 2);
            if (!row) {
                reportError("failed to get ie for branch insert\n");
                return false;
            }
            row->entries[1].instruction = secondInstruction;
        }
        if (!epilogueRegion->table->prepRawTable()) {
            reportError("failed to prep epi table!\n");
            return false;
        }
    }

    epiRec *otherRegion = loopRecord->allocExit(0);
    if (!otherRegion) {
        reportError("failed to get exit part\n");
        return false;
    }
    if (!otherRegion->table->prepRawTable()) {
        reportError("failed to prep exit table\n");
        return false;
    }
    return true;
}
