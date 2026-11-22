#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "ol_s_inline.h"
#include "tPair.h"
#include "tPairList.h"

int getToEnd(Instr *in);
int getFromStart(Instr *in);
int guessKeyLen(Program *p, int key, int start, int end);

bool prepExtInfo(Program *program, instStream *stream, bool noExt)
{
    workTable *table = stream->getTable();
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        instruction->clearExternalTimingConstraints();
    table->externalMinimumLength = 0;
    if (noExt)
        return 1;
    bool changed = 0;
    int streamId = stream->streamId;
    regUse *registerUse;
    regUseList *registerUseList = &table->registerUses;
    for (registerUse = registerUseList->head; registerUse; registerUse = registerUse->next) {
        RegPoolEntry *reg = registerUse->registerEntry;
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            regUseSlot *registerUseSlot = &registerUse->slots[partIndex];
            if (!registerUseSlot) {
                reportError("failed to get rrp\n");
                return 0;
            }
            registerUseSlot->earliestExternalCycle = 0;
            registerUseSlot->latestExternalCycle = 30000;
            for (ol_s *operandLocation = registerUseSlot->inputLocations.head; operandLocation; operandLocation = operandLocation->next) {
                Instr *instruction = operandLocation->instruction;
                Operand *operand = &instruction->operands[operandLocation->operandIndex];
                OpSlot *part = operand->getPart(operandLocation->componentIndex);
                short best = 0;
                ol_s *location = part->operandLocations.head;
                if (location) {
                    for (; location; location = location->next) {
                        int key = location->pathKey;
                        if (!key) {
                            reportError("can't have a null minkey for ext\n");
                            return 0;
                        }
                        Instr *setter = location->instruction;
                        Operand *src = operandAtLocation(location);
                        instStream *setterStream = setter->getOwningStream();
                        if (!setterStream) {
                            reportError("failed to get the setters ist\n");
                            return 0;
                        }
                        int sid = setterStream->streamId;
                        bool same = sid == streamId;
                        int pathLength = guessKeyLen(program, key, sid, streamId);
                        if (pathLength < 0) {
                            reportError("minimal length .. bah\n");
                            return 0;
                        }
                        int distanceToEnd = getToEnd(setter);
                        if (distanceToEnd < 0) {
                            reportError("distance to end -ve..error\n");
                            return 0;
                        }
                        int total = distanceToEnd + pathLength;
                        int requiredDistance = src->outputLatency + operand->additionalLatency - total - 1;
                        if ((setter->hasRawTableOrImmediateToken() || same) && requiredDistance > best)
                            best = requiredDistance;
                    }
                    if (operand->externalBlockModeFlag)
                        instruction->raiseExternalInputPairedConstraint(best);
                    else
                        instruction->raiseExternalInputConstraint(best);
                } else {
                    reportError("no external setter for prepExtInfo.. register is %s.%d\n",
                                  reg->registerName(), partIndex);
                    return 0;
                }
            }
            for (ol_s *operandLocation = registerUseSlot->outputLocations.head; operandLocation; operandLocation = operandLocation->next) {
                Instr *instruction = operandLocation->instruction;
                if (!instruction) {
                    reportError("not get visible output\n");
                    return 0;
                }
                short best = 0;
                Operand *operand = &instruction->operands[operandLocation->operandIndex];
                olList *locationList = &operand->getPart(operandLocation->componentIndex)->operandLocations;
                ol_s *externalLocation = locationList->firstExternalLocation();
                if (!operandLocation->isExtUsed() || externalLocation) {
                    if (!externalLocation || operandLocation->isExtUsed()) {
                        for (; externalLocation; externalLocation = externalLocation->nextExternalLocation()) {
                            int key = externalLocation->pathKey;
                            if (!key) {
                                reportError("can't have a null minkey for ext\n");
                                return 0;
                            }
                            Instr *user = externalLocation->instruction;
                            Operand *dst = operandAtLocation(externalLocation);
                            if (!user || !dst) {
                                reportError("no setting instruction/operand\n");
                                return 0;
                            }
                            instStream *userStream = user->getOwningStream();
                            if (!userStream) {
                                reportError("failed to get the setters ist\n");
                                return 0;
                            }
                            int userPathLength = guessKeyLen(program, key, streamId, userStream->streamId);
                            if (userPathLength < 0) {
                                reportError("minimal length .. bah\n");
                                return 0;
                            }
                            int distanceFromStart = getFromStart(user);
                            if (distanceFromStart < 0) {
                                reportError("distance to end -ve..error\n");
                                return 0;
                            }
                            int total = distanceFromStart + userPathLength;
                            int userRequiredDistance = operand->outputLatency + dst->additionalLatency - total - 1;
                            if (user->hasRawTableOrImmediateToken() && userRequiredDistance > best)
                                best = userRequiredDistance;
                        }
                        if (operand->externalBlockModeFlag)
                            instruction->raiseExternalOutputPairedConstraint(best);
                        else
                            instruction->raiseExternalOutputConstraint(best);
                    } else {
                        reportError("operand doesn't think there's any external users, but - there's an entry on it's ext list\n");
                        operandLocation->dumpOperandContext();
                        locationList->validateOperandLocations();
                        return 0;
                    }
                } else {
                    reportError("operand is tagged as being used externally - but.. there's no ext entry on it's oll?\n");
                    operandLocation->dumpOperandContext();
                    return 0;
                }
            }
            if (reg->tracksHotOutputPaths && !noExt) {
                ol_s *operandLocation;
                for (operandLocation = registerUseSlot->externalInputLocations.head; operandLocation; operandLocation = operandLocation->next) {
                    int key = operandLocation->pathKey;
                    if (!key) {
                        reportError("can't have a null minkey for ext\n");
                        return 0;
                    }
                    Instr *instruction = operandLocation->instruction;
                    if (!instruction) {
                        reportError("not get visible input\n");
                        return 0;
                    }
                    instStream *ownerStream = instruction->getOwningStream();
                    if (!ownerStream) {
                        reportError("failed to get the setters ist\n");
                        return 0;
                    }
                    int sourceId = ownerStream->streamId;
                    int setterPathLength = guessKeyLen(program, key, sourceId, streamId);
                    if (setterPathLength < 0) {
                        reportError("minimal length .. bah\n");
                        return 0;
                    }
                    int setterDistanceToEnd = getToEnd(instruction);
                    if (setterDistanceToEnd < 0) {
                        reportError("distance to end -ve..error\n");
                        return 0;
                    }
                    int total = setterDistanceToEnd + setterPathLength;
                    int setterRequiredDistance = instruction->baseIssueSpacing - total - 1;
                    if (instruction->hasRawTableOrImmediateToken() && setterRequiredDistance > registerUseSlot->earliestExternalCycle) {
                        changed = 1;
                        registerUseSlot->earliestExternalCycle = setterRequiredDistance;
                    }
                }
                for (operandLocation = registerUseSlot->externalSourceLocations.head; operandLocation; operandLocation = operandLocation->next) {
                    int key = operandLocation->pathKey;
                    if (!key) {
                        reportError("can't have a null minkey for ext\n");
                        return 0;
                    }
                    Instr *instruction = operandLocation->instruction;
                    if (!instruction) {
                        reportError("not get visible input\n");
                        return 0;
                    }
                    instStream *consumerStream = instruction->getOwningStream();
                    if (!consumerStream) {
                        reportError("failed to get the setters ist\n");
                        return 0;
                    }
                    int consumerPathLength = guessKeyLen(program, key, streamId, consumerStream->streamId);
                    if (consumerPathLength < 0) {
                        reportError("minimal length .. bah\n");
                        return 0;
                    }
                    int consumerDistanceFromStart = getFromStart(instruction);
                    if (consumerDistanceFromStart < 0) {
                        reportError("distance from start -ve..error\n");
                        return 0;
                    }
                    int consumerRequiredDistance = consumerDistanceFromStart + consumerPathLength + 1;
                    if (instruction->hasRawTableOrImmediateToken() && consumerRequiredDistance < registerUseSlot->latestExternalCycle) {
                        changed = 1;
                        registerUseSlot->latestExternalCycle = consumerRequiredDistance;
                    }
                }
            }
            for (tPair *transitionPair = registerUseSlot->throughputPairs.head; transitionPair; transitionPair = transitionPair->next) {
                instStream *sourceStream = (instStream *)transitionPair->a;
                instStream *destinationStream = (instStream *)transitionPair->b;
                if (!sourceStream || !destinationStream) {
                    reportError("there's no src or dst in a tpair\n");
                    return 0;
                }
                workTable *scheduleTable = sourceStream->getTable();
                workTable *destinationTable = destinationStream->getTable();
                if (scheduleTable->codeModeValue == 3 && destinationTable->codeModeValue == 3) {
                    int sourceId = sourceStream->streamId;
                    int k1 = guessKeyLen(program, transitionPair->src, sourceId, streamId);
                    if (k1 < 0) {
                        reportError("failed to guess the key length\n");
                        return 0;
                    }
                    int k2 = guessKeyLen(program, transitionPair->dst, streamId, destinationStream->streamId);
                    if (k2 < 0) {
                        reportError("distance to dst is bad\n");
                        return 0;
                    }
                    Instr *writerInstruction = scheduleTable->linearInstructions.tail;
                    Operand *writerOperand = 0;
                    for (; writerInstruction; writerInstruction = writerInstruction->linearLink.previousInstruction)
                        if ((writerOperand = writerInstruction->findOutputOperand(reg, 1 << partIndex)) != 0)
                            break;
                    if (!writerOperand) {
                        reportError("can't get last writer\n");
                        return 0;
                    }
                    Instr *userInstruction = destinationTable->linearInstructions.head;
                    Operand *userOperand = 0;
                    for (; userInstruction; userInstruction = userInstruction->linearLink.nextInstruction)
                        if ((userOperand = userInstruction->findInputOperand(reg, 1 << partIndex)) != 0)
                            break;
                    if (!userOperand) {
                        reportError("can't get first user \n");
                        return 0;
                    }
                    int writerDistanceToEnd = getToEnd(writerInstruction);
                    if (writerDistanceToEnd < 0) {
                        reportError("<ve to end \n");
                        return 0;
                    }
                    int readerDistanceFromStart = getFromStart(userInstruction);
                    int total = writerDistanceToEnd + k1 + k2 + readerDistanceFromStart;
                    int crossStreamRequiredDistance = writerOperand->outputLatency + userOperand->additionalLatency - total - 1;
                    if (crossStreamRequiredDistance > table->externalMinimumLength)
                        table->externalMinimumLength = crossStreamRequiredDistance;
                }
            }
        }
    }
    if (changed) {
        for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                RegPoolEntry *registerEntry = isRegisterOperand(operand, 2) ? operand->assignedRegister : 0;
                if (registerEntry && registerEntry->tracksHotOutputPaths) {
                    regUse *currentRegisterUse = registerUseList->findByRegister(registerEntry);
                    if (!currentRegisterUse) {
                        reportError("failed to find regref\n");
                        exitAfterCleanup(-1);
                    }
                    for (int registerPartIndex = 0; registerPartIndex != registerEntry->partCount; registerPartIndex++)
                        if ((operand->componentMask >> registerPartIndex) & 1) {
                            regUseSlot *slot = &currentRegisterUse->slots[registerPartIndex];
                            instruction->raiseExternalInputPairedConstraint(slot->earliestExternalCycle);
                            int bound = instruction->baseIssueSpacing - slot->latestExternalCycle;
                            instruction->raiseExternalOutputPairedConstraint(bound);
                        }
                }
            }
    }
    Instr *instruction;
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        for (int hazardOperandIndex = 0; hazardOperandIndex != instruction->operandCount; hazardOperandIndex++) {
            Operand *operand = &instruction->operands[hazardOperandIndex];
            RegPoolEntry *reg = operand->assignedRegister;
            if (reg) {
                regUse *hazardRegisterUse = registerUseList->findByRegister(reg);
                if (isRegisterOperand(operand, 1))
                    operand->reductionHazardMask = hazardRegisterUse->hazardMask & operand->externalPartMask;
                if (isRegisterOperand(operand, 2))
                    operand->reductionHazardMask = hazardRegisterUse->sourcePathHazardMask & operand->externalPartMask;
                operand->reductionHazardMask |= hazardRegisterUse->hazardBroadcastMask;
            }
        }
    int endType = stream->getEndType();
    if ((unsigned char)endType == 2) {
        goto branchBound;
    }
    if ((unsigned char)endType == 3) {
        goto branchBound;
    }
success:
    return 1;
branchBound:
    for (instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        if (instruction->opcodeId() == 0xa5)
            instruction->raiseExternalOutputConstraint(2);
    goto success;
}

extern const char prepRawAddInstrError[] = "iTable - failed to add an instruction\n";
