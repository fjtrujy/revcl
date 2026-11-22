// Deferred inline helpers of RegWeb; emitted out of line via #pragma implementation in RegWeb.cpp.
#ifndef REGWEB_INLINE_H
#define REGWEB_INLINE_H

#pragma interface

#include "RegWeb.h"
#include "Program.h"
#include "instStream.h"

bool resetPoolOptimizationState(Program *p);

inline bool resetRegisterWebs()
{
    regWebCount = 0;
    regWebUsed = 0;
    return true;
}

inline bool RegWeb::collectTableRegisterWebs(workTable *scheduleTable)
{
    if (!scheduleTable) {
        reportError("RegWeb::AddiTable - passed a null pointer\n");
        return false;
    }
    for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        if (!collectInstructionRegisterWebs(instruction)) {
            reportError("RegWeb::AddiTable: failed to add an isn\n");
            instruction->dumpOperandDiagnostics(false);
            scheduleTable->dump();
            return false;
        }
    }
    return true;
}

inline bool RegWeb::collectInstructionRegisterWebs(Instr *instruction)
{
    if (!instruction)
        return false;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        RegPoolEntry *registerEntry = operand->assignedRegister;
        if (!registerEntry || !operand->isForwardable())
            continue;
        RegWeb *registerWeb = operand->registerWeb;
        if (!operand->getRootRegisterWeb()) {
            registerWeb = createRegisterWeb(registerEntry);
            if (!registerWeb) {
                reportError("failed to get a web entry for forwardable operand\n");
                return false;
            }
            operand->registerWeb = registerWeb;
        }
        registerWeb->componentMask |= operand->componentMask;
        if (operand->pairedOperandIndexPlusOne && operand->direction == 1) {
            if (!merge(registerWeb, instruction->operands[operand->pairedOperandIndexPlusOne - 1].getRootRegisterWeb())) {
                reportError("failed to merge web for implicit web patch\n");
                return false;
            }
        }
        if (operand->forcedBaseRegister) {
            RegPoolEntry *forcedRegister = operand->forcedBaseRegister;
            if (forcedRegister->isVirtual) {
                reportError("can't have a virtual forced register\n");
                return false;
            }
            if (!setForcedRegisterMapping(registerWeb, forcedRegister)) {
                reportError("setfMap failed when binding to base for basereg %s, reg %s\n",
                              forcedRegister->registerName(), registerEntry->registerName());
                return false;
            }
            if (!registerEntry->isVirtual && registerEntry != forcedRegister) {
                reportError("register isn't virtual - but doesn't match base for forced base reg\n");
                return false;
            }
        }
        if (isRegisterOperand(operand, 1)) {
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++) {
                bool partUsed = (operand->componentMask >> partIndex) & 1;
                if (!partUsed)
                    continue;
                ol_s *sourceLocation = operand->parts[partIndex].operandLocations.head;
                if (!sourceLocation) {
                    reportError("found input operand part with empty olList..\n");
                    reportError("this should have been caught by now..\n");
                    reportError("stopping\n");
                    instruction->dumpInstructionContext(operandIndex, partIndex);
                    return false;
                }
                do {
                    Operand *sourceOperand = operandAtLocation(sourceLocation);
                    if (!sourceOperand) {
                        reportError("getoploc - empty!\n");
                        instruction->dumpInstructionContext(operandIndex, partIndex);
                        sourceLocation->dumpInstructionContext();
                    }
                    if (!sourceOperand->getRootRegisterWeb()) {
                        sourceOperand->registerWeb = createRegisterWeb(registerEntry);
                        if (!sourceOperand->registerWeb) {
                            reportError("failed to get a web entry for forwardable operand\n");
                            return false;
                        }
                    }
                    sourceOperand->registerWeb->componentMask |= sourceOperand->componentMask;
                    if (!merge(sourceOperand->getRootRegisterWeb(), registerWeb)) {
                        reportError("failed to merge web entries\n");
                        return false;
                    }
                    sourceLocation = sourceLocation->next;
                } while (sourceLocation);
            }
        }
    }
    return true;
}

inline RegWeb *RegWeb::createRegisterWeb(RegPoolEntry *registerEntry)
{
    if (regWebCount >= 0x4000) {
        reportError("oh no! ran out of regweb entries\n");
        return 0;
    }
    if (!registerEntry) {
        reportError("GetNewWeb  - old_vrg not set\n");
        return 0;
    }
    RegWeb *registerWeb = &regWebs[regWebCount];
    registerWeb->assignedRegister = 0;
    registerWeb->virtualRegister = registerEntry;
    registerWeb->componentMask = 0;
    registerWeb->parentWeb = 0;
    registerWeb->memberCount = 1;
    registerWeb->webIndex = regWebCount;
    registerWeb->forcedRegister = 0;
    regWebCount++;
    return registerWeb;
}

inline bool RegWeb::merge(RegWeb *firstWeb, RegWeb *secondWeb)
{
    if (!firstWeb) {
        reportError("RegWeb::merge.. w_a was null!\n");
        return false;
    }
    unsigned char firstMask = 0, secondMask = 0;
    while (firstWeb->parentWeb) {
        firstMask |= firstWeb->componentMask;
        if (firstWeb->memberCount) {
            reportError("Merge: inconsistency - nUsed on chase\n");
            exitAfterCleanup(-1);
            return false;
        }
        if (firstWeb->forcedRegister) {
            reportError("non-terminal web a had mapping\n");
            exitAfterCleanup(-1);
            return false;
        }
        firstWeb = firstWeb->parentWeb;
    }
    if (!firstWeb->memberCount) {
        reportError("hit a terminal empty strand in web\n");
        return false;
    }
    if (!secondWeb) {
        reportError("RegWeb::merge .. b was null\n");
        return false;
    }
    while (secondWeb->parentWeb) {
        secondMask |= secondWeb->componentMask;
        if (secondWeb->memberCount) {
            reportError("Merge: inconsistency - nUsed on chase for w_b\n");
            return false;
        }
        if (secondWeb->forcedRegister) {
            reportError("web b had mapping\n");
            exitAfterCleanup(-1);
            return false;
        }
        secondWeb = secondWeb->parentWeb;
    }
    if (!secondWeb->memberCount) {
        reportError("hit a terminal empty strand in web\n");
        return false;
    }
    if (firstWeb == secondWeb)
        return true;
    if (secondWeb->assignedRegister || firstWeb->assignedRegister) {
        reportError("one or both strands had register attached!\n");
        return false;
    }
    unsigned char combinedMask = firstMask | secondMask | secondWeb->componentMask | firstWeb->componentMask;
    firstWeb->componentMask = combinedMask;
    secondWeb->componentMask = combinedMask;
    if (firstWeb->forcedRegister) {
        if (!setForcedRegisterMapping(secondWeb, firstWeb->forcedRegister)) {
            reportError("failed to load mapping from a into b\n");
            return false;
        }
        firstWeb->forcedRegister = 0;
    }
    secondWeb->memberCount += firstWeb->memberCount;
    firstWeb->parentWeb = secondWeb;
    firstWeb->memberCount = 0;
    return true;
}

inline RegWeb *RegPoolEntry::getRootRegisterWeb()
{
    RegWeb *rootWeb = registerWeb;
    if (!rootWeb)
        return 0;
    while (rootWeb->parentWeb) {
        if (rootWeb->memberCount) {
            reportError(" INTERNAL Register::GetWeb found non terminal count not at end!! for '%s' count=%d\n",
                          registerName(), rootWeb->memberCount);
            exitAfterCleanup(-1);
        }
        rootWeb = rootWeb->parentWeb;
    }
    if (!rootWeb->memberCount) {
        reportError("INTERNAL Register::terminal entry had zero count!\n");
        exitAfterCleanup(-1);
    }
    return rootWeb;
}

inline RegWeb *Operand::getRootRegisterWeb()
{
    RegWeb *rootWeb = registerWeb;
    if (!rootWeb)
        return 0;
    while (rootWeb->parentWeb) {
        if (rootWeb->memberCount) {
            reportError("INTERNAL : Operand::GetWeb found non terminal count!\n");
            exitAfterCleanup(-1);
        }
        rootWeb = rootWeb->parentWeb;
    }
    if (!rootWeb->memberCount) {
        reportError("INTERNAL : Operand::terminal entry had zero count!\n");
        exitAfterCleanup(-1);
    }
    return rootWeb;
}

inline bool assignRegisterWebs()
{
    int allocatedWebCount = 0;
    char virtualRegisterName[128];
    int webIndex;
    for (webIndex = 0; webIndex != regWebCount; webIndex++) {
        RegWeb *registerWeb = &regWebs[webIndex];
        if (registerWeb->memberCount && registerWeb->forcedRegister) {
            RegPoolEntry *registerEntry = registerWeb->forcedRegister;
            if (registerEntry->isVirtual == 1) {
                reportError("forced to a non-base register\n");
                return false;
            }
            if (registerEntry->registerWeb) {
                if (!RegWeb::merge(registerEntry->registerWeb, registerWeb)) {
                    reportError("during mapped merge..failed for %s\n", registerEntry->registerName());
                    return false;
                }
            } else
                registerEntry->registerWeb = registerWeb;
        }
    }
    for (webIndex = 0; webIndex != regWebCount; webIndex++) {
        RegWeb *registerWeb = &regWebs[webIndex];
        if (registerWeb->memberCount) {
            RegPoolEntry *registerEntry = registerWeb->virtualRegister;
            if (!registerEntry) {
                reportError("GatherWeb:: oh no - dReg not set\n");
                exitAfterCleanup(-1);
                return false;
            }
            if (registerWeb->assignedRegister) {
                reportError("found register assign at unexpected place!\n");
                exitAfterCleanup(-1);
            }
            if (registerWeb->forcedRegister)
                registerWeb->assignedRegister = registerWeb->forcedRegister;
            else {
                RegPool *registerPool = registerEntry->ownerPool;
                if (!registerPool) {
                    reportError("Gather: register had no pool\n");
                    return false;
                }
                formatIntoBuffer(virtualRegisterName, "%s__%d", registerEntry->registerName(), registerEntry->uniqueNameSerial);
                registerWeb->assignedRegister = registerPool->createVirtualRegister(virtualRegisterName);
                if (!registerWeb->assignedRegister) {
                    reportError("GatherWeb: failed to get '%s' from %s\n", virtualRegisterName, registerPool->poolName);
                    return false;
                }
                registerWeb->assignedRegister->registerWeb = registerWeb;
                registerEntry->uniqueNameSerial++;
                allocatedWebCount++;
            }
        }
    }
    regWebUsed = allocatedWebCount;
    return true;
}

inline bool RegWeb::applyRegisterWebToOperand(Operand *operand)
{
    if (!operand)
        return true;
    RegWeb *registerWeb = operand->getRootRegisterWeb();
    if (registerWeb) {
        if (!registerWeb->assignedRegister) {
            reportError("patch operand - web has no reg\n");
            return false;
        }
        RegPoolEntry *registerEntry = registerWeb->assignedRegister;
        operand->assignedRegister = registerEntry;
        if (operand->forcedBaseRegister && operand->forcedBaseRegister != registerEntry) {
            reportError("not forcing to a base reg after all - in PatchOperands %s isn't %s\n",
                          operand->forcedBaseRegister->registerName(), registerEntry->registerName());
            int actualVirtualFlag = operand->assignedRegister->isVirtual;
            int forcedVirtualFlag = operand->forcedBaseRegister->isVirtual;
            reportError("have %d %d\n", forcedVirtualFlag, actualVirtualFlag);
            return false;
        }
    }
    return true;
}

inline bool RegWeb::applyRegisterWebsToInstruction(Instr *instruction)
{
    if (!instruction)
        return true;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        Operand *operand = &instruction->operands[operandIndex];
        if (!applyRegisterWebToOperand(operand))
            return false;
    }
    return true;
}

inline bool RegWeb::applyRegisterWebsToTable(workTable *scheduleTable)
{
    if (scheduleTable) {
        for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            if (!applyRegisterWebsToInstruction(instruction)) {
                reportError("patchiTable - failed\n");
                instruction->dumpOperandDiagnostics(false);
                goto fail;
            }
        }
        return true;
    }
    reportError("RegWeb::PatchITable - passed a null pointer\n");
    return false;
fail:
    return false;
}

inline void RegWeb::checkRegisterWebInvariants()
{
    for (int webIndex = 0; webIndex != regWebCount; webIndex++) {
        if (regWebs[webIndex].memberCount && !regWebs[webIndex].virtualRegister) {
            reportError("Dump: dReg not set\n");
            exitAfterCleanup(-1);
        }
    }
}

inline bool RegWeb::setForcedRegisterMapping(RegWeb *registerWeb, RegPoolEntry *forcedRegister)
{
    while (registerWeb->parentWeb)
        registerWeb = registerWeb->parentWeb;
    if (!forcedRegister) {
        reportError("regweb::setfMap - asked to set a null mapping\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (forcedRegister->isVirtual) {
        reportError("regweb:: setfMap - can't setfmap to virtual!\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (registerWeb->forcedRegister) {
        if (registerWeb->forcedRegister != forcedRegister) {
            reportError("regweb:: set map - can't map to different base registers..\n");
            reportError("regweb:: try to map both %s and %s\n",
                          registerWeb->forcedRegister->registerName(), forcedRegister->registerName());
            return false;
        }
    } else
        registerWeb->forcedRegister = forcedRegister;
    return true;
}

inline bool buildRegisterWebs(Program *program)
{
    resetRegisterWebs();
    if (!resetPoolOptimizationState(program)) {
        reportError("flow:: reset the webs\n");
        return false;
    }
    instStream *stream;
    for (stream = program->streamHead; stream; stream = stream->nextStream)
        RegWeb::clearTableRegisterWebs(stream->getWorkTable());
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!RegWeb::collectTableRegisterWebs(stream->getWorkTable())) {
            reportError("flow: AddIStream failed for %s\n", stream->streamName());
            return false;
        }
    }
    if (!assignRegisterWebs()) {
        reportError("flow::GatherWeb() failed\n");
        return false;
    }
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        if (!RegWeb::applyRegisterWebsToTable(stream->getWorkTable())) {
            reportError("flow: failed to patch ist %s\n", stream->streamName());
            return false;
        }
    }
    return true;
}

inline bool RegWeb::clearTableRegisterWebs(workTable *scheduleTable)
{
    for (Instr *instruction = scheduleTable->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++)
            instruction->operands[operandIndex].registerWeb = 0;
    return true;
}

#endif
