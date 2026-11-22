#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"
#include "Instr.h"
#include "RegPool.h"
#include "ol_s.h"
#include "ol_s_inline.h"

bool checkOpt1(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *scheduleTable = stream->getTable();
        if (!scheduleTable) {
            reportError("null work table\n");
            return false;
        }
        if (scheduleTable != &stream->sourceTable) {
            reportError("work table isn't input table\n");
            return false;
        }
        stream->getAllocMode();
    }
    return true;
}

bool checkOpt2(Program *program)
{
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *scheduleTable = stream->getTable();
        if (!scheduleTable) {
            reportError("null work table\n");
            return false;
        }
        if (stream->getCodeMode() != 4) {
            if (stream->getTable() != stream->optimizedTable) {
                reportError("work table isn't opt table\n");
                return false;
            }
            if (!scheduleTable->postOptimizationPrepared) {
                reportError("locks aren't valid for opt table\n");
                return false;
            }
        } else {
            if (stream->getTable() != &stream->sourceTable) {
                reportError("checkopt2 - synth table wasn't input\n");
                return false;
            }
        }
        if (stream->getAllocMode() != 3) {
            reportError("table %s didn't have external alloc\n", stream->streamName());
            return false;
        }
    }
    return true;
}

bool checkOpt3(Program *program)
{
    reportInfo("checkOpt2()\n");
    reportInfo("OLL free count = %d\n", countFreeLocationNodes());
    for (instStream *currentStream = program->streamHead; currentStream; currentStream = currentStream->nextStream) {
        workTable *scheduleTable = currentStream->getTable();
        if (!scheduleTable) {
            reportError("null work table\n");
            return false;
        }

        if (scheduleTable->isAbstract()) {
            if (scheduleTable->rows) {
                reportError("table set for abstract list\n");
                return false;
            }
            for (Instr *linearInstruction = scheduleTable->linearInstructions.head; linearInstruction; linearInstruction = linearInstruction->linearLink.nextInstruction) {
                if (linearInstruction->position.getPipe() != -1) {
                    reportError("pipe is set for non-inserted instruction\n");
                    return false;
                }
                if (linearInstruction->position.getRow() != -1) {
                    reportError("rline is set for non-inserted\n");
                    return false;
                }
                if (linearInstruction->isConcreteMove()) {
                    reportError("there's a concrete move no abstract list\n");
                    return false;
                }
            }
        }
        if (scheduleTable->codeModeValue == 3) {
            int start = scheduleTable->getRangeStart();
            int end = scheduleTable->getRangeEnd();
            if (start < 0 || end < start) {
                reportError("invalid table range\n");
                return false;
            }
            int count = 0;
            Instr *prev = 0;
            for (int rowIndex = start; rowIndex != end; rowIndex++) {
                Instr **instructionSlot = ((cls_unk30 *)scheduleTable)->getIE_ck(rowIndex);
                if (!instructionSlot) {
                    reportError("failed to get ientry from table\n");
                    return false;
                }
                for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
                    Instr *rowInstruction = instructionSlot[pipeIndex];
                    if (!rowInstruction)
                        continue;
                    if (((VclPtrWord)rowInstruction & 1) == 1) {
                        reportError("found a key in an instruction slot\n");
                        return false;
                    }
                    if (rowInstruction->linearLink.previousInstruction != prev) {
                        const char *name = "NO_BASE_INST";
                        if (rowInstruction->opcodeInfo)
                            name = rowInstruction->opcodeInfo->mnemonic;
                        reportError("linear list isn't in order at guid=%d %s prev=%p\n",
                                      rowInstruction->instructionSerial, name, rowInstruction->linearLink.previousInstruction);
                        scheduleTable->dump();
                        return false;
                    }
                    prev = rowInstruction;
                    if (rowInstruction->position.getRow() != rowIndex) {
                        reportError("instruction is very lost\n");
                        return false;
                    }
                    if (rowInstruction->position.getPipe() != pipeIndex) {
                        reportError("instruction is quite lost\n");
                        return false;
                    }
                    if (rowInstruction->isAbstractMove()) {
                        reportError("there's an abstract move in a raw table\n");
                        return false;
                    }
                    count++;
                }
            }
            int lcount = scheduleTable->linearInstructions.count;
            if (count != lcount) {
                reportError("linear count and itable mismatch (%d != %d) for %s\n",
                              lcount, count, currentStream->streamName());
                scheduleTable->dump();
                return false;
            }
        }

        for (Instr *linkedInstruction = scheduleTable->linearInstructions.head; linkedInstruction; linkedInstruction = linkedInstruction->linearLink.nextInstruction) {
            for (int operandIndex = 0; operandIndex != linkedInstruction->operandCount; operandIndex++) {
                Operand *operand = &linkedInstruction->operands[operandIndex];
                RegPoolEntry *registerEntry;
                if (linkedInstruction->operands[operandIndex].pairedOperandIndexPlusOne) {
                    Operand *pairedOperand = &linkedInstruction->operands[linkedInstruction->operands[operandIndex].pairedOperandIndexPlusOne - 1];
                    if (pairedOperand->pairedOperandIndexPlusOne - 1 != operandIndex) {
                        reportError("badly linked web patch\n");
                        return false;
                    }
                    registerEntry = linkedInstruction->operands[operandIndex].assignedRegister;
                    if (registerEntry != pairedOperand->assignedRegister) {
                        reportError("not same reg for iwp\n");
                        return false;
                    }
                } else
                    registerEntry = linkedInstruction->operands[operandIndex].assignedRegister;
                RegPoolEntry *mreg = (RegPoolEntry *)linkedInstruction->operands[operandIndex].forcedBaseRegister;
                if (registerEntry && registerEntry->isVirtual && !linkedInstruction->operands[operandIndex].originalRegister) {
                    reportError("original reg not set\n");
                    return false;
                }
                if (mreg) {
                    if (!registerEntry) {
                        reportError("original reg not set, but mapped base reg is\n");
                        return false;
                    }
                    if (mreg->isVirtual) {
                        reportError("mapped base reg is set - but it's virtual\n");
                        return false;
                    }
                }
                if (registerEntry && registerEntry->isVirtual != 1 && mreg && registerEntry != mreg) {
                    reportError("register is base, but mapped base register, it isn't.. reg=%s mreg=%s\n",
                                  registerEntry->registerName(), mreg->registerName());
                    return false;
                }
                if (registerEntry) {
                    for (unsigned partIndex = 0; partIndex != (unsigned)registerEntry->partCount; partIndex++) {
                        OpSlot *slot = operand->getPart(partIndex);
                        if (!slot)
                            continue;
                        ol_s *location = slot->operandLocations.head;
                        bool seenExt = false;
                        int last = -1000;
                        for (; location; location = location->next) {
                            if (location->isExt()) {
                                if (!seenExt)
                                    seenExt = true;
                            } else if (seenExt) {
                                reportError("found an internal oploc after an external one\n");
                                return false;
                            }
                            Instr *locationInstruction = location->instruction;
                            if (!locationInstruction) {
                                reportError("oploc not tied to instruction in checkopt3\n");
                                return false;
                            }
                            if (!locationInstruction->getOwningStream()) {
                                reportError("oploc instruction doesnt have ist set\n");
                                return false;
                            }
                            if (!location->isExt()) {
                                if (locationInstruction->getOwningStream() != currentStream) {
                                    reportError("internal oploc but instruction not from this stream\n");
                                    return false;
                                }
                                if (scheduleTable->codeModeValue == 3) {
                                    if (__builtin_expect(locationInstruction->position.getRow() < last, 0)) {
                                        reportError("local oplocs  aren't in order\n");
                                        return false;
                                    }
                                    last = locationInstruction->position.getRow();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    reportInfo("checkOpt3()..ok\n");
    return true;
}

extern const char iInfoLoadstoreRamtypeError[] = "loadstore instruction, but bad ramtype\n";
extern const char iInfoExplicitAfterImplicitError[] = "explicit operand after an implicit\n";
extern const char iInfoPatchedAfterPatchError[] = "patched operand is after patch\n";
