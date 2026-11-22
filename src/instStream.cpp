#include "os.h"
#include "instStream.h"

bool isReferencedLabel(Program *p, char *name);
bool isExportedSymbol(Program *p, char *name);

instStream::instStream()
{
    stats.clear();
    unk_2d8 = 0;
    appliedOptimizationMask = 0;
    hasLoopOptimization = 0;
    savedStallReason = 0;
    currentStallReason = 0;
    dependencyLength = -1;
    logicalLength = -1;
    physicalInstructionBudget = 0x4000;
    externalSchedulingPrepared = 0;
    absoluteLoopCount = 0;
    extraLoopCount = 0;
    loopRegionRole = 0;
    knownMinimumLoopCount = 0;
    safeMinimumLoopCount = 0;
    emissionStartIndex = -1;
    traceStore = 0;
    endDirectiveText = 0;
    nextStream = 0;
    previousStream = 0;
    streamId = 0;
    streamSerial = 0;
    unk_2c6 = 0;
    owningProgram = 0;
    workTablePtr = 0;
    savedTable = 0;
    optimizedTable = 0;
    emissionTable = 0;
    passThroughText = 0;
    containsBarrier = 0;
    mergedBarrierFlag = 0;
    leadingLabelFlags = 0;
    trailingLabelFlags = 0;
    participatesInStall = 0;
    allowUpwardReduction = 0;
    allowDownwardReduction = 0;
    unk_2b6 = -1;
    branchForwardLatencyDepth = -1;
    branchType = 0;
    endType = 1;
    startType = 1;
    optimizationRecordCount = 0;
}

instStream::~instStream()
{
    reset();
}

bool instStream::reset()
{
    savedStallReason = 0;
    currentStallReason = 0;
    labels.clearNodes();
    streamId = -1;
    owningProgram = 0;
    branchType = 0;
    if (traceStore)
        delete[] traceStore;
    traceStore = 0;
    if (endDirectiveText)
        delete[] endDirectiveText;
    endDirectiveText = 0;
    inputLinks.removeAll();
    outputLinks.removeAll();
    if (passThroughText)
        delete[] passThroughText;
    passThroughText = 0;
    return true;
}

void instStream::dump(int)
{
    labels.noopListTraversal();
    inputLinks.dump();
    canLoop();
    workTable *wt = workTablePtr;
    if (!wt) {
        reportWarning("iStream:: dump - null work table - defaulting to inputTable\n");
        wt = &sourceTable;
    }
    wt->dump();
    outputLinks.dump();
    int dep = getRecordedDependencyLength();
    int log = getRecordedLogicalLength();
    // length print compiled out in this build
    if (dep > 0 || log > 0) {
    }
}

void instStream::zeroStats()
{
    if (!workTablePtr) {
        reportError("no work table set for zeroStats!\n");
        exitAfterCleanup(-1);
    }
    for (Instr *i = workTablePtr->linearInstructions.head; i; i = i->linearLink.nextInstruction)
        i->schedulingStatistics.zeroAllCounters();
    schedulingStatistics.zeroAllCounters();
}

bool instStream::iOwn(Instr *i)
{
    if (!i) {
        reportWarning("iStream::iOwn - passed a null pointer\n");
        return false;
    }
    instStream *o = i->getOwningStream();
    if (!o) {
        reportWarning("iStream::iOwn - no owner for instruction\n");
        return false;
    }
    return o == this;
}

bool instStream::patchLatencies()
{
    workTable *wt = workTablePtr;
    int n = 0;
    if (!wt) {
        reportError("iStream::PatchLatencies - no work table\n");
        return false;
    }
    for (Instr *i = wt->linearInstructions.head; i; i = i->linearLink.nextInstruction) {
        i->linearOrderIndex = n;
        if (!i->patchBranchLatencies()) {
            reportError("failed to patch latencies for instruction\n");
            return false;
        }
        n++;
    }
    return true;
}

const char *istEndTypeName(int t)
{
    const char *s = "ISTET_BAD";
    switch (t) {
    case 1:
        s = "ISTET_NULL";
        break;
    case 2:
        s = "ISTET_END_CONT";
        break;
    case 3:
        s = "ISTET_END_END";
        break;
    case 4:
        s = "ISTET_END_MACRO";
        break;
    }
    return s;
}

const char *istBranchTypeName(int t)
{
    return branchTypeNameByCode(t);
}

const char *instStream::branchTypeName()
{
    return branchTypeNameByCode(branchType);
}

const char *istStartTypeName(int t)
{
    const char *s;
    switch (t) {
    case 1:
        s = "ISTST_NULL";
        break;
    case 2:
        s = "ISTST_ENTRY";
        break;
    default:
        s = "ISTST???\n";
        break;
    }
    return s;
}

const char *instStream::endTypeName()
{
    int t = endType;
    const char *s = "ISTET_BAD";
    switch (t) {
    case 1:
        s = "ISTET_NULL";
        break;
    case 2:
        s = "ISTET_END_CONT";
        break;
    case 3:
        s = "ISTET_END_END";
        break;
    case 4:
        s = "ISTET_END_MACRO";
        break;
    }
    return s;
}

const char *instStream::startTypeName()
{
    const char *s;
    switch (startType) {
    case 1:
        s = "ISTST_NULL";
        break;
    case 2:
        s = "ISTST_ENTRY";
        break;
    default:
        s = "ISTST???\n";
        break;
    }
    return s;
}

bool instStream::canLoop()
{
    if (branchType == 1 || branchType == 5) {
        instStream *s = outputLinks.getStream(1);
        if (!s) {
            reportError("canLoop - bad return from outIST getFirst\n");
            exitAfterCleanup(-1);
        } else {
            int sv = s->optimizationRecordCount;
            unsigned char z = s->branchType;
            z >>= 4;
            z >>= 4;
            if (z)
                return false;
            s->optimizationRecordCount = sv;
        }
        return s == this;
    }
    return false;
}

static inline void dumpEmitStream(instStream *s)
{
    s->dump(false);
}

char *instStream::emit(char *buf, int a, int *p1, int *p2)
{
    *p1 = 0;
    *p2 = 0;
    if (!buf)
        return 0;
    if (!labels.nodeCount) {
        reportError("iStream::Emit: stream with no name\n");
        return 0;
    }
    if (!(char)checkStartEnd()) {
        reportError("failed to check stream /start/end\n");
        dumpEmitStream(this);
        return 0;
    }
    if (startType != 2 && !inputLinks.countMarkedLinks() && !passThroughText) {
        reportInfo("skip target emit. .coz it's no longer visible\n");
        return buf;
    }
    for (mNode *n = labels.head; n; n = n->next) {
        if (n->name && (isReferencedLabel(owningProgram, n->name)
                          || isExportedSymbol(owningProgram, n->name) || n->flags))
            buf += formatIntoBuffer(buf, "%s:\n", n->name);
    }
    if (passThroughText) {
        buf += formatIntoBuffer(buf, "\n%s\n", passThroughText);
    } else {
        if (!emissionTable) {
            reportError("can't emit .. no table!\n");
            return 0;
        }
        buf = emissionTable->emit(buf, a, p1, p2);
        if (!buf) {
            reportError("failed to build out table\n");
            return 0;
        }
        if (endType == 4) {
            buf += formatIntoBuffer(buf, "\n");
            if (!endDirectiveText) {
                reportError("end string not set\n");
                return 0;
            }
            buf += formatIntoBuffer(buf, "         %s\n", endDirectiveText);
        }
    }
    return buf;
}

bool instStream::flowReset()
{
    if (!workTablePtr) {
        reportError("work table not set\n");
        return false;
    }
    if (!workTablePtr->registerUses.clear()) {
        reportError("iStream::flowReset() - failed to dispose regrefs\n");
        return false;
    }
    for (Instr *i = workTablePtr->linearInstructions.head; i; i = i->linearLink.nextInstruction) {
        if (!i->resetFlowDependencies()) {
            reportError("failed to init flow for isn\n");
            return false;
        }
    }
    if (!workTablePtr->resetFlowStalls()) {
        reportError("failed to flow reset the iEntries for a work table\n");
        return false;
    }
    participatesInStall = 0;
    return true;
}

const char *instStream::streamName()
{
    const char *candidateName = labels.firstNodeName();
    const char *resolvedName = "UNNAMED_ISTREAM";
    if (candidateName)
        resolvedName = candidateName;
    return resolvedName;
}

void instStream::dumpSynthetics()
{
    checkedTable("dump synthetics .. work table not set\n")->dumpSynthetics();
}

char *instStream::emitLabels(char *buf)
{
    if (emissionStartIndex >= 0) {
        for (mNode *n = labels.head; n; n = n->next) {
            if (n->name) {
                const char *p = "__v_";
                const char *s = n->name;
                while (*p && *s && *p == *s) {
                    p++;
                    s++;
                }
                if (*p)
                    buf += formatIntoBuffer(buf, "%-40s = %d\n", n->name, emissionStartIndex);
            }
        }
    }
    return buf;
}

Instr *instStream::getBranch()
{
    return checkedTable("getBranch  - no work table!\n")->getBranch();
}

bool instStream::removeBranch()
{
    requireTable("getBranch  - no work table!\n");
    if (!workTablePtr->removeBranch()) {
        reportError("failed to delete branch from work table\n");
        return false;
    }
    branchType = 0;
    return true;
}

char *instStream::getBranchTargetImmediate()
{
    char *r = 0;
    requireTable("getBranch  - no work table!\n");
    Instr *br = workTablePtr->getBranch();
    if (br)
        r = br->getBranchTargetImmediate();
    else
        reportWarning("getBranchTargetImmediate:: failed to get branch\n");
    return r;
}

RegPoolEntry *instStream::getBranchTargetRegister()
{
    RegPoolEntry *r = 0;
    requireTable("getBranch  - no work table!\n");
    Instr *br = workTablePtr->getBranch();
    if (br)
        r = br->getBranchTargetRegister();
    else
        reportWarning("getBranchTargetImmediate:: failed to get branch\n");
    return r;
}

unsigned char instStream::getCodeMode()
{
    workTable *wt = workTablePtr;
    if (wt)
        return wt->codeModeValue;
    reportError("getCodeMode - work table not set\n");
    return 0;
}

const char *instStream::getCodeModeName()
{
    workTable *wt = workTablePtr;
    if (wt)
        return codeModeName(wt->codeModeValue);
    reportError("getCodeMode - work table not set\n");
    return 0;
}

unsigned char instStream::getRawSchedulingMode()
{
    workTable *wt = workTablePtr;
    if (wt)
        return wt->getTableRawMode();
    reportError("getCodeMode - work table not set\n");
    return 0;
}

static inline bool isRawTable(workTable *wt)
{
    return wt->codeModeValue == 3;
}

static inline bool isFreeRawTable(workTable *wt)
{
    return isRawTable(wt) && (wt->rawSchedulingMode == 1 || wt->rawSchedulingMode == 3);
}

bool instStream::usesFreeRawLayout()
{
    workTable *wt = workTablePtr;
    if (!wt)
        return false;
    return isFreeRawTable(wt);
}

static inline bool isFixedRawTable(workTable *wt)
{
    return isRawTable(wt) && (wt->rawSchedulingMode == 2 || wt->rawSchedulingMode == 4);
}

bool instStream::usesFixedRawLayout()
{
    workTable *wt = workTablePtr;
    if (!wt)
        return false;
    return isFixedRawTable(wt);
}

unsigned char instStream::getAllocMode()
{
    workTable *wt = workTablePtr;
    if (wt)
        return wt->allocationMode;
    reportError("getAllocMode - work table not set\n");
    return 0;
}

bool instStream::isOutsideRawMode()
{
    workTable *wt = workTablePtr;
    if (wt && wt->codeModeValue == 3)
        return false;
    return true;
}

bool instStream::hasSafeTrailingInstructions()
{
    if (workTablePtr)
        return workTablePtr->hasSafeTrailingInstructions();
    return false;
}

int instStream::checkStartEndStatus()
{
    int st = startType;
    int ok = 0;
    if (st == 1 || st == 2) {
        int et = endType;
        unsigned endTypeCount = 4;
        ok = (unsigned)(et - 1) < endTypeCount;
    }
    return ok;
}

bool instStream::processCmdSynth(Instr *i)
{
    if (!i) {
        reportError("processCmdSynth - passed a null ptr\n");
        return false;
    }
    if (i->parsedLoopId > 0) {
        reportUserMessage("parseLid set for processCmdSynth\n");
        i->leDump();
        return false;
    }
    switch ((unsigned char)i->syntheticKind) {
    case 5:
        if (endType != 1) {
            reportError("CONT directive encountered - but end mode isn't null\n");
            return false;
        }
        endType = 2;
        break;
    case 8:
        if (!i->getImmediateOperand(0, &knownMinimumLoopCount) || !i->getImmediateOperand(1, &safeMinimumLoopCount)) {
            reportUserMessage("failed to decode the --LoopCS directive, one of the 2 expected operands failed to parse\n");
            i->leDump();
            return false;
        }
        if (knownMinimumLoopCount <= 0) {
            reportUserMessage("for --LoopCS, the minimum known count (arg 1) must be must be >0\n");
            i->leDump();
            return false;
        }
        if (safeMinimumLoopCount < 0) {
            reportUserMessage("for --LoopCS, the minimum safe count (arg 2) must be must be >=0\n");
            i->leDump();
            return false;
        }
        break;
    case 9:
        if (!i->getImmediateOperand(0, &extraLoopCount) || extraLoopCount < 0) {
            reportUserMessage("failed to get immediate field 0 (extra loop count) while processing --LoopExtra. value must be >=0\n");
            i->leDump();
            return false;
        }
        break;
    case 10:
        if (containsBarrier) {
            const char *s = labels.firstNodeName();
            const char *n = "UNNAMED_ISTREAM";
            if (s)
                n = s;
            reportError("multiple barriers in one flow..%s?\n", n);
            return false;
        }
        containsBarrier = 1;
        break;
    default:
        reportUserMessage("unexpected synthetic instruction encountered\n");
        break;
    case 12:
        if (!i->getImmediateOperand(0, &absoluteLoopCount) || absoluteLoopCount <= 2) {
            reportUserMessage("failed to get immediate field 0 (extra loop count) while processing --LoopAbs. value must be >=2\n");
            i->leDump();
            return false;
        }
        break;
    }
    return true;
}

bool instStream::disposeInputTable()
{
    workTable *t = &sourceTable;
    if (t == workTablePtr) {
        reportError("can't dispose of input table while it's the work table\n");
        abortAfterError();
    }
    return t->dispose();
}

void instStream::copySourceRecordsFrom(instStream *src)
{
    optimizationRecordCount = src->optimizationRecordCount;
    for (int i = 0; i != optimizationRecordCount; i++)
        optimizationRecords[i] = src->optimizationRecords[i];
}
