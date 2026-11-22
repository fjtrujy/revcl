#include "Processor.h"
#include "Instr.h"
#include "genRegPool.h"
#include "hwRegPool.h"
#include "memRegPool.h"
#include "os.h"
#include "Program.h"

extern int errorOccurred;
extern bool quietDiagnostics;

bool equalAsciiCaseInsensitive(char a, char b);
bool isIdentifierAlphabetCharacter(char c);
char *dupString(const char *s);

Processor::Processor()
{
    uniqueName = 0;
    nextInstructionSerial = 0;
    newSyntax = 0;
    processorInstructionTable = 0;
    cpuName = "Nameless";
    globalName = "";
    sourceBuildType = 0;
    preferExistingPhysicalRegisters = 0;
    mpgTargetAddress = -1;
    instructionsPerRawLine = 0;
    uniqueAll = 0;
    decodeMode = 0;
    decodeSubMode = 0;
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        processorRegisterPools[poolIndex] = 0;
    for (int moveTypeIndex = 0; moveTypeIndex != 3; moveTypeIndex++)
        freeMoveRecordsByType[moveTypeIndex] = 0;
}

Processor::~Processor()
{
}

bool Processor::initializeProcessor(CpuDef *cpu)
{
    if (!cpu) {
        reportError("null pointer got into init for a cpu\n");
        return false;
    }
    if (!cpu->name) {
        reportError("must name a cpu\n");
        return false;
    }
    PoolDef *pools = cpu->pools;
    iInfo *iinfos = cpu->iinfos;
    if (!checkTable(iinfos, 0xd6)) {
        reportError("couldnt init cpu '%s' - the idefs were bad\n", cpu->name);
        return false;
    }
    processorDeviceTable = cpu->devtab;
    instructionsPerRawLine = cpu->instructionsPerRawLine;
    if (instructionsPerRawLine < 1 || instructionsPerRawLine > 2) {
        reportError("processor '%s' has too many instructions per raw line\n", cpu->name);
        return false;
    }
    cpuName = cpu->name;
    if (!pools) {
        reportError("processor: init - passed a null strap table\n");
        return false;
    }
    if (!iinfos) {
        reportError("passed a null itable\n");
        return false;
    }
    processorInstructionTable = iinfos;
    for (int poolDefinitionIndex = 0; pools[poolDefinitionIndex].type; poolDefinitionIndex++) {
        PoolDef *poolDefinition = &pools[poolDefinitionIndex];
        RegPool *pool;
        switch (poolDefinition->type) {
        case 1:
            pool = new genRegPool;
            break;
        case 2:
            pool = new hwRegPool;
            break;
        case 3:
            pool = new memRegPool;
            break;
        default:
            reportError("Processor::Init - bad type_id found\n");
            return false;
        }
        if (!pool)
            reportError("Processor::Init  - failed to get a regpool '%s'\n", poolDefinition->name);
        if (!pool->initializeRegisterPool((VclPtrWord)poolDefinition->prefix, poolDefinition->name, poolDefinition->cls, poolDefinition->virtualRegisterParts)) {
            reportError("failed to init reg pool '%s'\n", poolDefinition->name);
            delete pool;
            return false;
        }
        if (poolDefinition->zeroReg) {
            RegPoolEntry *registerEntry = pool->findPhysicalByName(poolDefinition->zeroReg);
            if (!registerEntry) {
                reportError("failed to get zero reg for pool %s\n", poolDefinition->name);
                delete pool;
                return false;
            }
            pool->zeroRegister = registerEntry;
        }
        if (poolDefinition->extReg) {
            RegPoolEntry *registerEntry = pool->findPhysicalByName(poolDefinition->extReg);
            if (!registerEntry) {
                reportError("failed to get ext reg for pool %s\n", poolDefinition->name);
                delete pool;
                return false;
            }
            pool->externalRegister = registerEntry;
        }
        if (processorRegisterPools[poolDefinition->cls]) {
            reportError("already initialised reg type %d\n", poolDefinition->cls);
            delete pool;
            return false;
        }
        processorRegisterPools[poolDefinition->cls] = pool;
    }
    if (!cpu->dstSpecs) {
        reportError("procDec has no dstSpecs loaded\n");
        return false;
    }
    processorDestinationSpecs = cpu->dstSpecs;
    if (!cpu->srcSpecs) {
        reportError("procDec has no srcSpecs loaded\n");
        return false;
    }
    processorSourceSpecs = cpu->srcSpecs;
    return true;
}

bool Processor::clearProcessorLists()
{
    visibilityScopes.clearNodes();
    trailingLabels.clearNodes();
    exportedSymbols.clearNodes();
    return true;
}

void Processor::dumpRegisterPools()
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex])
            processorRegisterPools[poolIndex]->Dump();
}

int Processor::gatherInflight(ol_s **out)
{
    int inflightCount = 0;
    for (int poolIndex = 0; poolIndex != 12; poolIndex++) {
        if (processorRegisterPools[poolIndex]) {
            int gatheredCount = processorRegisterPools[poolIndex]->gatherInflight(out + inflightCount);
            if (gatheredCount < 0) {
                reportError("error during inflight gather\n");
                return -1;
            }
            inflightCount += gatheredCount;
        }
    }
    return inflightCount;
}

bool Processor::clearAllRegisterAssignmentsFirstPass()
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->clearRegisterAssignmentsFirstPass()) {
            reportError("failed to alloc reset an rpool\n");
            return false;
        }
    return true;
}

bool Processor::clearAllRegisterAssignmentsSecondPass()
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->clearRegisterAssignmentsSecondPass()) {
            reportError("failed to alloc reset an rpool\n");
            return false;
        }
    return true;
}

void Processor::validateOpcodeTableCount()
{
    if (!processorInstructionTable) {
        reportError("iInfos not set in processor\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quietDiagnostics;
    int opcodeCount = 0;
    for (iInfo *opcodeInfo = processorInstructionTable; opcodeInfo->mnemonic; opcodeInfo++) {
        opcodeInfo->scanOpcodeOperandDescriptors();
        opcodeCount++;
    }
    if (opcodeCount != 0xd6) {
        reportError("error processing inst table\n");
        exitAfterCleanup(-1);
    }
    errorOccurred = saved;
}

void dumpProcessorRegisterPools(Program *program)
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex])
            processorRegisterPools[poolIndex]->Dump();
}

bool parseReset(Program *program)
{
    program->proc.vuDirectiveSeen = false;
    program->proc.exportedSymbols.clearNodes();
    program->proc.trailingLabels.clearNodes();
    program->proc.visibilityScopes.clearNodes();
    program->proc.decodeMode = 0;
    program->proc.decodeSubMode = 0;
    program->proc.savedDecodeMode = 0;
    program->proc.savedDecodeSubMode = 0;
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->clearVirtualRegistersForParse()) {
            reportError("Processor::parseReset() - failed to reset %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

bool statReset(Program *program)
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->resetPoolPartTiming()) {
            reportError("Processor::statReset() - failed to stat %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

iInfo *getOpcodeInfoById(int opcodeId)
{
    return &processorInstructionTable[opcodeId];
}

iInfo *findOpcodeInfoByMnemonic(const char *tok, bool all)
{
    if (!tok) {
        reportError("Proc::FindInstruction() - null token passed\n");
        return 0;
    }
    if (!processorInstructionTable) {
        reportError("Proc::FindInstruction() - no instruction set loaded!\n");
        return 0;
    }
    iInfo *opcodeInfo = processorInstructionTable;
    if (!all)
        opcodeInfo += 12;
    for (; opcodeInfo->mnemonic; opcodeInfo++) {
        const char *inputCursor = tok;
        const char *mnemonicCursor = opcodeInfo->mnemonic;
        while (*inputCursor && *mnemonicCursor && equalAsciiCaseInsensitive(*inputCursor, *mnemonicCursor)) {
            inputCursor++;
            mnemonicCursor++;
        }
        if (*mnemonicCursor)
            continue;
        if (!*inputCursor)
            return opcodeInfo;
        if (*inputCursor == '.' && opcodeInfo->destinationMaskMode) {
            do {
                inputCursor++;
                if (!*inputCursor)
                    return opcodeInfo;
            } while (*inputCursor != '.');
        }
    }
    return 0;
}

bool optReset(Program *program)
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->clearPoolPartPayloads()) {
            reportError("Processor::optReset() - failed to reset %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

bool advanceProcessorCycle(Program *program, bool flag)
{
    program->emulatedCycleCount++;
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->cycleRegisters(flag)) {
            reportError("Processor::Cycle() - failed to cycle %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

bool resetProcessorThroughput(Program *program)
{
    program->emulatedCycleCount = 0;
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->resetPoolThroughputState()) {
            reportError("Processor::emuReset() - failed to reset %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

inline RegPool *lookupProcessorRegisterPool(unsigned poolIndex)
{
    if (poolIndex < 12)
        return processorRegisterPools[poolIndex];
    reportWarning("Processor::GetRP - out of range register type (%d)\n", poolIndex);
    return 0;
}

RegPoolEntry *findPhysicalRegister(unsigned type, const char *name)
{
    RegPool *pool = lookupProcessorRegisterPool(type);
    if (!pool) {
        reportError("getBaseReg from - not find pool %d\n", type);
        exitAfterCleanup(-1);
    }
    return pool->findPhysicalByName(name);
}

RegPoolEntry *findPhysicalRegister(unsigned type, int index)
{
    RegPool *pool = lookupProcessorRegisterPool(type);
    if (!pool) {
        reportError("getBaseReg from - not find pool %d\n", type);
        exitAfterCleanup(-1);
    }
    return pool->findByRegisterIndex(index);
}

RegPoolEntry *findRegisterByGlobalIndex(int idx)
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++) {
        RegPool *pool = processorRegisterPools[poolIndex];
        if (pool) {
            RegPoolEntry *registerEntry = pool->findByRegisterIndex(idx);
            if (registerEntry)
                return registerEntry;
        }
    }
    return 0;
}

RegPoolEntry *getOperandZeroRegister(int kind)
{
    RegPool *pool = lookupProcessorRegisterPool(registerClassForOperandKind(kind));
    if (!pool) {
        reportError("getRP failed for operand getZeroReg\n");
        exitAfterCleanup(-1);
    }
    return pool->zeroRegister;
}

RegPoolEntry *getPoolZeroRegister(unsigned type)
{
    RegPool *pool = lookupProcessorRegisterPool(type);
    if (!pool) {
        reportError("getRP failed getZeroReg\n");
        exitAfterCleanup(-1);
    }
    return pool->zeroRegister;
}

RegPool *getRegisterPoolByIndex(unsigned poolIndex)
{
    if (poolIndex > 11) {
        reportWarning("Processor::GetRP - out of range register type (%d)\n", poolIndex);
        return 0;
    }
    return processorRegisterPools[poolIndex];
}

bool flowReset(Program *program)
{
    program->proc.usedBeforeSet.clear();
    program->proc.stalls.clear();
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->resetPoolFlowState()) {
            reportError("Processor::flowReset() - failed to reset %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

void reportUBS(Program *program)
{
    ubsList *ubs = &program->proc.usedBeforeSet;
    if (ubs->count) {
        reportInfo("uh-oh, VCL thinks the following operands were used before set:\n");
        ubs->dumpEntries();
    }
}

void reportStalls(Program *program)
{
    infoList *stallRecords = &program->proc.stalls;
    if (stallRecords->count) {
        reportInfo("VCL thinks the following instructions caused stalls:\n");
        stallRecords->dumpEntries();
    }
}

int checkDispatchEntry(Instr **pipelineEntries)
{
    if (!pipelineEntries) {
        reportError("checkDispatchEntry - passed a null pointer\n");
        exitAfterCleanup(-1);
    }
    for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
        Instr *instruction = getInstructionInPipe(pipelineEntries, pipeIndex);
        if (!instruction)
            continue;
        if (!(((unsigned char)instruction->permittedPipelineMask >> pipeIndex) & 1))
            return -1;
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
            RegPoolEntry *outputRegister = registerOperand(&instruction->operands[operandIndex], 2);
            if (!outputRegister)
                continue;
            for (int pairedPipeIndex = pipeIndex + 1; pairedPipeIndex <= 1; pairedPipeIndex++) {
                Instr *pairedInstruction = getInstructionInPipe(pipelineEntries, pairedPipeIndex);
                if (!pairedInstruction)
                    continue;
                if (!(((unsigned char)pairedInstruction->permittedPipelineMask >> pairedPipeIndex) & 1))
                    return -1;
                for (int pairedOperandIndex = 0; pairedOperandIndex != pairedInstruction->operandCount; pairedOperandIndex++) {
                    RegPoolEntry *pairedOutputRegister = registerOperand(&pairedInstruction->operands[pairedOperandIndex], 2);
                    if (pairedOutputRegister && pairedOutputRegister == outputRegister)
                        return -1;
                }
            }
        }
    }
    return 0;
}

bool wouldPair(Instr **pipelineEntries, Instr *candidateInstruction, unsigned pipeIndex)
{
    if (!pipelineEntries || !candidateInstruction || pipeIndex > 1) {
        reportError("wouldPair - bad args\n");
        exitAfterCleanup(-1);
    }
    int originalSlotIndex;
    for (originalSlotIndex = 0; originalSlotIndex != 2; originalSlotIndex++)
        if (pipelineEntries[originalSlotIndex] == candidateInstruction) {
            pipelineEntries[originalSlotIndex] = (Instr *)candidateInstruction->replacedInstructionWord;
            break;
        }
    if (pipelineEntries[pipeIndex] != (Instr *)candidateInstruction->replacedInstructionWord) {
        if (originalSlotIndex != 2)
            pipelineEntries[originalSlotIndex] = candidateInstruction;
        return false;
    }
    pipelineEntries[pipeIndex] = candidateInstruction;
    int dispatchStatus = checkDispatchEntry(pipelineEntries);
    pipelineEntries[pipeIndex] = (Instr *)candidateInstruction->replacedInstructionWord;
    if (originalSlotIndex != 2)
        pipelineEntries[originalSlotIndex] = candidateInstruction;
    return dispatchStatus == 0;
}

const char *deviceNameByIndex(int deviceIndex)
{
    const char **deviceNames = (const char **)processorDeviceTable;
    if (!deviceNames)
        return "NO_DEVTAB";
    if ((unsigned)deviceIndex > 8)
        return "UNKNOWN_DEV";
    return deviceNames[deviceIndex];
}

bool isVuRegister(RegPoolEntry *registerEntry)
{
    return registerEntry->registerClass == 8 || registerEntry->registerClass == 9;
}

bool resetPoolOptimizationState(Program *program)
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->optReset()) {
            reportError("Processor::optReset() - failed to reset %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

static inline int iinfoId(iInfo *opcodeInfo)
{
    if (!opcodeInfo)
        return -1;
    int opcodeIndex = ((char *)opcodeInfo - (char *)processorInstructionTable) / sizeof(iInfo);
    if (opcodeIndex < 0)
        return -1;
    if (opcodeIndex >= 0xd6)
        opcodeIndex = -1;
    return opcodeIndex;
}

int getOpcodeId(iInfo *opcodeInfo)
{
    return iinfoId(opcodeInfo);
}

aaDesc *findAddressAliasDescriptor(Instr *instruction)
{
    aaDesc *foundDescriptor;
    if (!instruction) {
        foundDescriptor = 0;
        return foundDescriptor;
    }
    if (!instruction->opcodeInfo) {
        foundDescriptor = 0;
        return foundDescriptor;
    }
    int opcodeId = iinfoId(instruction->opcodeInfo);
    if (opcodeId < 0) {
        reportError("couldn't deduce instruction uid\n");
        exitAfterCleanup(-1);
    }
    if (!processorSourceSpecs) {
        foundDescriptor = 0;
        return foundDescriptor;
    }
    aaDesc *candidateDescriptor = (aaDesc *)processorSourceSpecs;
    if (candidateDescriptor->opcodeId) {
        do {
            if (candidateDescriptor->opcodeId == opcodeId) {
                foundDescriptor = candidateDescriptor;
                return foundDescriptor;
            }
            candidateDescriptor++;
        } while (candidateDescriptor->opcodeId);
    }
    foundDescriptor = 0;
    return foundDescriptor;
}

aaUse *findAddressAliasUse(Instr *instruction)
{
    if (!instruction)
        return 0;
    if (!instruction->opcodeInfo)
        return 0;
    int opcodeId = iinfoId(instruction->opcodeInfo);
    if (opcodeId < 0) {
        reportError("couldn't deduce instruction uid\n");
        exitAfterCleanup(-1);
    }
    if (!processorSourceSpecs)
        return 0;
    for (aaUse *candidateUse = (aaUse *)processorDestinationSpecs; candidateUse->opcodeId; candidateUse++)
        if (candidateUse->opcodeId == opcodeId)
            return candidateUse;
    return 0;
}

bool emitReset(Program *program)
{
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && !processorRegisterPools[poolIndex]->emitReset()) {
            reportError("Processor::emitReset() - failed to reset %s\n",
                          processorRegisterPools[poolIndex]->poolName);
            return false;
        }
    return true;
}

char *emitStats(Program *program, char *outputBuffer)
{
    outputBuffer += formatIntoBuffer(outputBuffer, "; register stats:\n");
    for (int poolIndex = 0; poolIndex != 12; poolIndex++)
        if (processorRegisterPools[poolIndex] && processorRegisterPools[poolIndex]->virtualPartCount) {
            outputBuffer = processorRegisterPools[poolIndex]->emitStats(outputBuffer);
            if (!outputBuffer) {
                reportError("Processor::emitStats() - failed to emit stats for %s\n",
                              processorRegisterPools[poolIndex]->poolName);
                return 0;
            }
        }
    return outputBuffer;
}

int getPoolsByClassID(int poolClass, RegPool **matchingPools, int maximumPools)
{
    if (!matchingPools) {
        reportError("Processor :: passed a null table pointer to getPoolsByClassID()\n");
        return -1;
    }
    int matchedPoolCount = 0;
    for (int poolIndex = 0; poolIndex != 12; poolIndex++) {
        RegPool *pool = processorRegisterPools[poolIndex];
        if (pool && pool->poolKind == poolClass) {
            if (matchedPoolCount >= maximumPools) {
                reportError("Processor:: more pools than space provided in getPoolsByClassID()\n");
                return -1;
            }
            matchingPools[matchedPoolCount++] = pool;
        }
    }
    return matchedPoolCount;
}

bool Processor::setUniqueName(const char *name)
{
    if (uniqueName)
        delete[] uniqueName;
    uniqueName = 0;
    if (name) {
        uniqueName = dupString(name);
        for (char *nameCursor = uniqueName; *nameCursor; nameCursor++)
            if (!isIdentifierAlphabetCharacter(*nameCursor))
                *nameCursor = '_';
    }
    return true;
}

const char *Processor::getUniqueName()
{
    return uniqueName ? uniqueName : "FLOW";
}

int nextInstructionSerial = 1;
bool staggerMemoryAccess = false;
iInfo *processorInstructionTable;
RegPool *processorRegisterPools[13];
VclPtrWord freeMoveRecordsByType[3];
void *processorDeviceTable;
void *processorSourceSpecs;
void *processorDestinationSpecs;
