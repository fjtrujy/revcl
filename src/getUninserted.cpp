#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

int getToEnd(Instr *in);
int getFromStart(Instr *in);

instStream *selectUninsertedStream(Program *program)
{
    instStream *stream = program->streamHead;
    instStream *best = 0;
    int minIn = 1000000;
    int maxOut = -1000000;
    for (; stream; stream = stream->nextStream) {
        if (stream->getCodeMode() == 1) {
            int in = stream->getUninsertedxInputs();
            int out = stream->getUninsertedxOutputs();
            if (in < 0) {
                reportError("failed to get uninserted inputs\n");
                return 0;
            }
            if (out < 0) {
                reportError("failed to get uninserted outputs\n");
                return 0;
            }
            if (in < minIn) {
                best = stream;
                minIn = in;
                maxOut = out;
            } else if (in == minIn && out > maxOut) {
                best = stream;
                maxOut = out;
            }
        }
    }
    return best;
}

int instStream::getUninsertedxInputs()
{
    int count = 0;
    if (!workTablePtr) {
        reportError("work table not set\n");
        return -1;
    }
    for (Instr *instruction = workTablePtr->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        int base = getFromStart(instruction);
        if (base < 0) {
            reportError("iStream::: getUninsertedxInputs - failed\n");
            return -1;
        }
        base += schedulingStatistics.reverseLatencyDepth - instruction->schedulingStatistics.reverseLatencyDepth;
        Operand *operand;
        for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
            RegPoolEntry *reg = registerOperand(operand, 1);
            if (!reg || !operand->externalPartMask)
                continue;
            for (int k = 0; k != reg->partCount; k++) {
                if ((operand->externalPartMask >> k) & 1) {
                ol_s *operandLocation = operand->parts[k].operandLocations.head;
                if (!operandLocation) {
                    reportError("no setter list found during getuninstxi!\n");
                    return -1;
                }
                for (; operandLocation; operandLocation = operandLocation->next) {
                    Instr *setter = operandLocation->instruction;
                    if (setter->getOwningStream() == this || setter->optimizedInstructionImageWord)
                        continue;
                    int te = getToEnd(setter);
                    if (te < 0) {
                        reportError("getUninsertedxInputs - failed to get to end for a setter\n");
                        return -1;
                    }
                    Operand *src = operandAtLocation(operandLocation);
                    if (te + base + operandLocation->cycleOffset + 1 < src->outputLatency + operand->additionalLatency) {
                        if (reg->partCount == 1)
                            count += 4;
                        else
                            count++;
                    }
                }
                }
            }
        }
    }
    return count;
}

int instStream::getUninsertedxOutputs()
{
    int count = 0;
    if (!workTablePtr) {
        reportError("work table not set\n");
        return -1;
    }
    for (Instr *instruction = workTablePtr->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        int base = getToEnd(instruction);
        if (base < 0) {
            reportError("iStream::: getUninsertedxOutputs - failed\n");
            return -1;
        }
        base += schedulingStatistics.forwardLatencyDepth - instruction->schedulingStatistics.forwardLatencyDepth;
        Operand *operand;
        for (int i = 0; (operand = instruction->getOp(i)) != 0; i++) {
            RegPoolEntry *reg = registerOperand(operand, 2);
            if (!reg || !operand->externalPartMask)
                continue;
            for (int k = 0; k != reg->partCount; k++) {
                if ((operand->externalPartMask >> k) & 1) {
                    for (ol_s *operandLocation = operand->parts[k].operandLocations.head; operandLocation; operandLocation = operandLocation->next) {
                        Instr *user = operandLocation->instruction;
                        if (user->getOwningStream() == this || user->optimizedInstructionImageWord)
                            continue;
                        int fs = getFromStart(user);
                        if (fs < 0) {
                            reportError("getUninsertedxOutputs - failed to get to end for a setter\n");
                            return -1;
                        }
                        Operand *src = operandAtLocation(operandLocation);
                        if (base + fs + operandLocation->cycleOffset + 1 < operand->outputLatency + src->additionalLatency) {
                            if (reg->partCount == 1)
                                count += 4;
                            else
                                count++;
                        }
                    }
                }
            }
        }
    }
    return count;
}

int getToEnd(Instr *instruction)
{
    workTable *table = (workTable *)instruction->ownerTable;
    if (!table) {
        reportError("instruction had no owner for getToEnd()\n");
        return -1;
    }
    instStream *stream = table->ownerStream;
    if (!stream) {
        reportError("failed to get owner owner\n");
        return -1;
    }
    Instr *img = instruction;
    if (instruction->optimizedInstructionImageWord) {
        img = (Instr *)instruction->optimizedInstructionImageWord;
        if (!img->ownerTable) {
            reportError("image had no owner\n");
            return -1;
        }
        if (stream != img->getOwningStream()) {
            reportError("our image was in different stream\n");
            return -1;
        }
        table = (workTable *)img->ownerTable;
        if (table->codeModeValue != 3) {
            reportError("had opt image...but it wasn't vsm\n");
            return -1;
        }
    }
    switch (table->codeModeValue) {
    case 1:
        return instruction->schedulingStatistics.reverseLatencyDepth;
    case 3: {
        int len = table->getRawOrSyntheticLogicalLength();
        if (len < 0) {
            reportError("logical len<0 for table \n");
            return -1;
        }
        int et = img->scheduledCycle;
        if (et < 0) {
            reportError("exec time not set\n");
            return -1;
        }
        int te = len - et - 1;
        if (te < 0) {
            reportError("te<0... for getLLen.. eTim=%d tSiz=%d\n", et, len, te);
            table->dump();
            reportError("instruction is\n");
            img->dumpOperandDiagnostics(false);
            break;
        }
        return te;
    }
    case 4:
        return 0;
    default:
        const char *modeName = codeModeName(table->codeModeValue);
        reportError("bad code mode for getToEnd()..mode=%s\n", modeName);
        break;
    }
    return -1;
}

int getFromStart(Instr *instruction)
{
    workTable *table = (workTable *)instruction->ownerTable;
    if (!table) {
        reportError("instruction had no owner for getToEnd()\n");
        return -1;
    }
    instStream *stream = table->ownerStream;
    if (!stream) {
        reportError("failed to get owner owner\n");
        return -1;
    }
    Instr *img = instruction;
    if (instruction->optimizedInstructionImageWord) {
        img = (Instr *)instruction->optimizedInstructionImageWord;
        if (!img->ownerTable) {
            reportError("image had no owner\n");
            return -1;
        }
        if (stream != img->getOwningStream()) {
            reportError("our image was in different stream\n");
            return -1;
        }
        table = (workTable *)img->ownerTable;
        if (table->codeModeValue != 3) {
            reportError("had opt image...but it wasn't vsm\n");
            return -1;
        }
    }
    switch (table->codeModeValue) {
    case 1:
        return instruction->schedulingStatistics.forwardLatencyDepth;
    case 3: {
        int len = table->getRawOrSyntheticLogicalLength();
        if (len < 0) {
            reportError("logical len<0 for table \n");
            return -1;
        }
        int et = img->scheduledCycle;
        if (et < 0) {
            reportError("exec time not set\n");
            return -1;
        }
        if (et < len)
            return et;
        reportError("tick after table end\n");
        return -1;
    }
    case 4:
        return 0;
    default:
        reportError("bad code mode for getFromStart()\n");
        break;
    }
    return -1;
}

bool optReset(Program *p);
bool setRange(int start, int end, int lsz);

bool loadLocks(Program *program, workTable *table)
{
    if (!table) {
        reportError("load locks to registers - failed\n");
        return false;
    }
    if (!table->postOptimizationPrepared) {
        reportError("locks aren't valid for loadlocks\n");
        return false;
    }
    if (!optReset(program)) {
        reportError("failed to opt reset\n");
        return false;
    }
    setRange(table->getRangeStart(), table->getRangeEnd(), table->getLoopSize());
    for (regUse *registerUse = table->registerUses.head; registerUse; registerUse = registerUse->next) {
        RegPoolEntry *reg = registerUse->registerEntry;
        for (int i = 0; i != reg->partCount; i++) {
            regUseSlot *registerUseSlot = &registerUse->slots[i];
            if (!registerUseSlot) {
                reportError("failed to get rrp\n");
                exitAfterCleanup(-1);
            }
            RegisterPartLockList *rll = (RegisterPartLockList *)registerUseSlot->assignmentPayload;
            if (!rll) {
                reportError("no rll set in regref\n");
                exitAfterCleanup(-1);
            }
            RegisterPartLockList *cur = (RegisterPartLockList *)reg->getPartPayload(i);
            if (!cur)
                reg->setPartPayload(i, (VclPtrWord)rll);
            else if (rll != cur) {
                reportError("rll mismatch\n");
                exitAfterCleanup(-1);
            }
        }
    }
    return true;
}
