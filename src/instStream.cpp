#include "os.h"
#include "instStream.h"

bool isReferencedLabel(Program *p, char *name);
bool isExportedSymbol(Program *p, char *name);

InstructionStream::InstructionStream()
{
    stats.clear();
    resetOnlyStreamEmissionWord = 0;
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
    resetOnlyStreamLoopShort = 0;
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
    resetOnlyStreamBranchTimingSentinel = -1;
    branchForwardLatencyDepth = -1;
    branchType = 0;
    endType = 1;
    startType = 1;
    optimizationRecordCount = 0;
}

InstructionStream::~InstructionStream()
{
    reset();
}

bool InstructionStream::reset()
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

void InstructionStream::dump(int unusedDumpMode)
{
    labels.noopListTraversal();
    inputLinks.dump();
    canLoop();
    ScheduleTable *table = workTablePtr;
    if (!table) {
        reportWarning("iStream:: dump - null work table - defaulting to inputTable\n");
        table = &sourceTable;
    }
    table->dump();
    outputLinks.dump();
    int dep = getRecordedDependencyLength();
    int log = getRecordedLogicalLength();
    // length print compiled out in this build
    if (dep > 0 || log > 0) {
    }
}

void InstructionStream::zeroStats()
{
    if (!workTablePtr) {
        reportError("no work table set for zeroStats!\n");
        exitAfterCleanup(-1);
    }
    for (Instr *instruction = workTablePtr->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        instruction->schedulingStatistics.zeroAllCounters();
    schedulingStatistics.zeroAllCounters();
}

bool InstructionStream::iOwn(Instr *instruction)
{
    if (!instruction) {
        reportWarning("iStream::iOwn - passed a null pointer\n");
        return false;
    }
    InstructionStream *otherStream = instruction->getOwningStream();
    if (!otherStream) {
        reportWarning("iStream::iOwn - no owner for instruction\n");
        return false;
    }
    return otherStream == this;
}

bool InstructionStream::patchLatencies()
{
    ScheduleTable *table = workTablePtr;
    int patchedLatencyCount = 0;
    if (!table) {
        reportError("iStream::PatchLatencies - no work table\n");
        return false;
    }
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        instruction->linearOrderIndex = patchedLatencyCount;
        if (!instruction->patchBranchLatencies()) {
            reportError("failed to patch latencies for instruction\n");
            return false;
        }
        patchedLatencyCount++;
    }
    return true;
}

const char *istEndTypeName(int streamEndType)
{
    const char *endTypeNameText = "ISTET_BAD";
    switch (streamEndType) {
    case 1:
        endTypeNameText = "ISTET_NULL";
        break;
    case 2:
        endTypeNameText = "ISTET_END_CONT";
        break;
    case 3:
        endTypeNameText = "ISTET_END_END";
        break;
    case 4:
        endTypeNameText = "ISTET_END_MACRO";
        break;
    }
    return endTypeNameText;
}

const char *istBranchTypeName(int branchType)
{
    return branchTypeNameByCode(branchType);
}

const char *InstructionStream::branchTypeName()
{
    return branchTypeNameByCode(branchType);
}

const char *istStartTypeName(int streamStartType)
{
    const char *startTypeNameText;
    switch (streamStartType) {
    case 1:
        startTypeNameText = "ISTST_NULL";
        break;
    case 2:
        startTypeNameText = "ISTST_ENTRY";
        break;
    default:
        startTypeNameText = "ISTST???\n";
        break;
    }
    return startTypeNameText;
}

const char *InstructionStream::endTypeName()
{
    int streamEndType = endType;
    const char *endTypeNameText = "ISTET_BAD";
    switch (streamEndType) {
    case 1:
        endTypeNameText = "ISTET_NULL";
        break;
    case 2:
        endTypeNameText = "ISTET_END_CONT";
        break;
    case 3:
        endTypeNameText = "ISTET_END_END";
        break;
    case 4:
        endTypeNameText = "ISTET_END_MACRO";
        break;
    }
    return endTypeNameText;
}

const char *InstructionStream::startTypeName()
{
    const char *startTypeNameText;
    switch (startType) {
    case 1:
        startTypeNameText = "ISTST_NULL";
        break;
    case 2:
        startTypeNameText = "ISTST_ENTRY";
        break;
    default:
        startTypeNameText = "ISTST???\n";
        break;
    }
    return startTypeNameText;
}

bool InstructionStream::canLoop()
{
    if (branchType == 1 || branchType == 5) {
        InstructionStream *stream = outputLinks.getStream(1);
        if (!stream) {
            reportError("canLoop - bad return from outIST getFirst\n");
            exitAfterCleanup(-1);
        } else {
            int optimizationCount = stream->optimizationRecordCount;
            unsigned char branchFlagHighBits = stream->branchType;
            branchFlagHighBits >>= 4;
            branchFlagHighBits >>= 4;
            if (branchFlagHighBits)
                return false;
            stream->optimizationRecordCount = optimizationCount;
        }
        return stream == this;
    }
    return false;
}

static inline void dumpEmitStream(InstructionStream *stream)
{
    stream->dump(false);
}

char *InstructionStream::emit(char *outputBuffer, int commentMode, int *p1, int *p2)
{
    *p1 = 0;
    *p2 = 0;
    if (!outputBuffer)
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
        return outputBuffer;
    }
    for (mNode *node = labels.head; node; node = node->next) {
        if (node->name && (isReferencedLabel(owningProgram, node->name)
                          || isExportedSymbol(owningProgram, node->name) || node->flags))
            outputBuffer += formatIntoBuffer(outputBuffer, "%s:\n", node->name);
    }
    if (passThroughText) {
        outputBuffer += formatIntoBuffer(outputBuffer, "\n%s\n", passThroughText);
    } else {
        if (!emissionTable) {
            reportError("can't emit .. no table!\n");
            return 0;
        }
        outputBuffer = emissionTable->emit(outputBuffer, commentMode, p1, p2);
        if (!outputBuffer) {
            reportError("failed to build out table\n");
            return 0;
        }
        if (endType == 4) {
            outputBuffer += formatIntoBuffer(outputBuffer, "\n");
            if (!endDirectiveText) {
                reportError("end string not set\n");
                return 0;
            }
            outputBuffer += formatIntoBuffer(outputBuffer, "         %s\n", endDirectiveText);
        }
    }
    return outputBuffer;
}

bool InstructionStream::flowReset()
{
    if (!workTablePtr) {
        reportError("work table not set\n");
        return false;
    }
    if (!workTablePtr->registerUses.clear()) {
        reportError("iStream::flowReset() - failed to dispose regrefs\n");
        return false;
    }
    for (Instr *instruction = workTablePtr->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        if (!instruction->resetFlowDependencies()) {
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

const char *InstructionStream::streamName()
{
    const char *candidateName = labels.firstNodeName();
    const char *resolvedName = "UNNAMED_ISTREAM";
    if (candidateName)
        resolvedName = candidateName;
    return resolvedName;
}

void InstructionStream::dumpSynthetics()
{
    checkedTable("dump synthetics .. work table not set\n")->dumpSynthetics();
}

char *InstructionStream::emitLabels(char *outputBuffer)
{
    if (emissionStartIndex >= 0) {
        for (mNode *node = labels.head; node; node = node->next) {
            if (node->name) {
                const char *prefixCursor = "__v_";
                const char *labelCursor = node->name;
                while (*prefixCursor && *labelCursor && *prefixCursor == *labelCursor) {
                    prefixCursor++;
                    labelCursor++;
                }
                if (*prefixCursor)
                    outputBuffer += formatIntoBuffer(outputBuffer, "%-40s = %d\n", node->name, emissionStartIndex);
            }
        }
    }
    return outputBuffer;
}

Instr *InstructionStream::getBranch()
{
    return checkedTable("getBranch  - no work table!\n")->getBranch();
}

bool InstructionStream::removeBranch()
{
    requireTable("getBranch  - no work table!\n");
    if (!workTablePtr->removeBranch()) {
        reportError("failed to delete branch from work table\n");
        return false;
    }
    branchType = 0;
    return true;
}

char *InstructionStream::getBranchTargetImmediate()
{
    char *branchLabelText = 0;
    requireTable("getBranch  - no work table!\n");
    Instr *branchInstruction = workTablePtr->getBranch();
    if (branchInstruction)
        branchLabelText = branchInstruction->getBranchTargetImmediate();
    else
        reportWarning("getBranchTargetImmediate:: failed to get branch\n");
    return branchLabelText;
}

RegisterEntry *InstructionStream::getBranchTargetRegister()
{
    RegisterEntry *registerEntry = 0;
    requireTable("getBranch  - no work table!\n");
    Instr *branchInstruction = workTablePtr->getBranch();
    if (branchInstruction)
        registerEntry = branchInstruction->getBranchTargetRegister();
    else
        reportWarning("getBranchTargetImmediate:: failed to get branch\n");
    return registerEntry;
}

unsigned char InstructionStream::getCodeMode()
{
    ScheduleTable *table = workTablePtr;
    if (table)
        return table->codeModeValue;
    reportError("getCodeMode - work table not set\n");
    return 0;
}

const char *InstructionStream::getCodeModeName()
{
    ScheduleTable *table = workTablePtr;
    if (table)
        return codeModeName(table->codeModeValue);
    reportError("getCodeMode - work table not set\n");
    return 0;
}

unsigned char InstructionStream::getRawSchedulingMode()
{
    ScheduleTable *table = workTablePtr;
    if (table)
        return table->getTableRawMode();
    reportError("getCodeMode - work table not set\n");
    return 0;
}

static inline bool isRawTable(ScheduleTable *table)
{
    return table->codeModeValue == 3;
}

static inline bool isFreeRawTable(ScheduleTable *table)
{
    return isRawTable(table) && (table->rawSchedulingMode == 1 || table->rawSchedulingMode == 3);
}

bool InstructionStream::usesFreeRawLayout()
{
    ScheduleTable *table = workTablePtr;
    if (!table)
        return false;
    return isFreeRawTable(table);
}

static inline bool isFixedRawTable(ScheduleTable *table)
{
    return isRawTable(table) && (table->rawSchedulingMode == 2 || table->rawSchedulingMode == 4);
}

bool InstructionStream::usesFixedRawLayout()
{
    ScheduleTable *table = workTablePtr;
    if (!table)
        return false;
    return isFixedRawTable(table);
}

unsigned char InstructionStream::getAllocMode()
{
    ScheduleTable *table = workTablePtr;
    if (table)
        return table->allocationMode;
    reportError("getAllocMode - work table not set\n");
    return 0;
}

bool InstructionStream::isOutsideRawMode()
{
    ScheduleTable *table = workTablePtr;
    if (table && table->codeModeValue == 3)
        return false;
    return true;
}

bool InstructionStream::hasSafeTrailingInstructions()
{
    if (workTablePtr)
        return workTablePtr->hasSafeTrailingInstructions();
    return false;
}

int InstructionStream::checkStartEndStatus()
{
    int streamStartType = startType;
    int validStartEndPair = 0;
    if (streamStartType == 1 || streamStartType == 2) {
        int streamEndType = endType;
        unsigned endTypeCount = 4;
        validStartEndPair = (unsigned)(streamEndType - 1) < endTypeCount;
    }
    return validStartEndPair;
}

bool InstructionStream::processCmdSynth(Instr *instruction)
{
    if (!instruction) {
        reportError("processCmdSynth - passed a null ptr\n");
        return false;
    }
    if (instruction->parsedLoopId > 0) {
        reportUserMessage("parseLid set for processCmdSynth\n");
        instruction->dumpInstructionContext();
        return false;
    }
    switch ((unsigned char)instruction->syntheticKind) {
    case 5:
        if (endType != 1) {
            reportError("CONT directive encountered - but end mode isn't null\n");
            return false;
        }
        endType = 2;
        break;
    case 8:
        if (!instruction->getImmediateOperand(0, &knownMinimumLoopCount) || !instruction->getImmediateOperand(1, &safeMinimumLoopCount)) {
            reportUserMessage("failed to decode the --LoopCS directive, one of the 2 expected operands failed to parse\n");
            instruction->dumpInstructionContext();
            return false;
        }
        if (knownMinimumLoopCount <= 0) {
            reportUserMessage("for --LoopCS, the minimum known count (arg 1) must be must be >0\n");
            instruction->dumpInstructionContext();
            return false;
        }
        if (safeMinimumLoopCount < 0) {
            reportUserMessage("for --LoopCS, the minimum safe count (arg 2) must be must be >=0\n");
            instruction->dumpInstructionContext();
            return false;
        }
        break;
    case 9:
        if (!instruction->getImmediateOperand(0, &extraLoopCount) || extraLoopCount < 0) {
            reportUserMessage("failed to get immediate field 0 (extra loop count) while processing --LoopExtra. value must be >=0\n");
            instruction->dumpInstructionContext();
            return false;
        }
        break;
    case 10:
        if (containsBarrier) {
            const char *labelName = labels.firstNodeName();
            const char *streamNameText = "UNNAMED_ISTREAM";
            if (labelName)
                streamNameText = labelName;
            reportError("multiple barriers in one flow..%s?\n", streamNameText);
            return false;
        }
        containsBarrier = 1;
        break;
    default:
        reportUserMessage("unexpected synthetic instruction encountered\n");
        break;
    case 12:
        if (!instruction->getImmediateOperand(0, &absoluteLoopCount) || absoluteLoopCount <= 2) {
            reportUserMessage("failed to get immediate field 0 (extra loop count) while processing --LoopAbs. value must be >=2\n");
            instruction->dumpInstructionContext();
            return false;
        }
        break;
    }
    return true;
}

bool InstructionStream::disposeInputTable()
{
    ScheduleTable *table = &sourceTable;
    if (table == workTablePtr) {
        reportError("can't dispose of input table while it's the work table\n");
        abortAfterError();
    }
    return table->dispose();
}

void InstructionStream::copySourceRecordsFrom(InstructionStream *sourceStream)
{
    optimizationRecordCount = sourceStream->optimizationRecordCount;
    for (int optimizationIndex = 0; optimizationIndex != optimizationRecordCount; optimizationIndex++)
        optimizationRecords[optimizationIndex] = sourceStream->optimizationRecords[optimizationIndex];
}
