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
        workTable *wt = stream->getTable();
        if (!wt) {
            reportError("null work table\n");
            return false;
        }
        if (wt != &stream->sourceTable) {
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
        workTable *wt = stream->getTable();
        if (!wt) {
            reportError("null work table\n");
            return false;
        }
        if (stream->getCodeMode() != 4) {
            if (stream->getTable() != stream->optimizedTable) {
                reportError("work table isn't opt table\n");
                return false;
            }
            if (!wt->postOptimizationPrepared) {
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
    for (instStream *s = program->streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        if (!wt) {
            reportError("null work table\n");
            return false;
        }

        if (wt->isAbstract()) {
            if (wt->rows) {
                reportError("table set for abstract list\n");
                return false;
            }
            for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
                if (in->position.getPipe() != -1) {
                    reportError("pipe is set for non-inserted instruction\n");
                    return false;
                }
                if (in->position.getRow() != -1) {
                    reportError("rline is set for non-inserted\n");
                    return false;
                }
                if (in->isConcreteMove()) {
                    reportError("there's a concrete move no abstract list\n");
                    return false;
                }
            }
        }
        if (wt->codeModeValue == 3) {
            int start = wt->getRangeStart();
            int end = wt->getRangeEnd();
            if (start < 0 || end < start) {
                reportError("invalid table range\n");
                return false;
            }
            int count = 0;
            Instr *prev = 0;
            for (int i = start; i != end; i++) {
                Instr **ie = ((cls_unk30 *)wt)->getIE_ck(i);
                if (!ie) {
                    reportError("failed to get ientry from table\n");
                    return false;
                }
                for (int j = 0; j != 2; j++) {
                    Instr *in = ie[j];
                    if (!in)
                        continue;
                    if (((VclPtrWord)in & 1) == 1) {
                        reportError("found a key in an instruction slot\n");
                        return false;
                    }
                    if (in->linearLink.previousInstruction != prev) {
                        const char *name = "NO_BASE_INST";
                        if (in->opcodeInfo)
                            name = in->opcodeInfo->mnemonic;
                        reportError("linear list isn't in order at guid=%d %s prev=%p\n",
                                      in->instructionSerial, name, in->linearLink.previousInstruction);
                        wt->dump();
                        return false;
                    }
                    prev = in;
                    if (in->position.getRow() != i) {
                        reportError("instruction is very lost\n");
                        return false;
                    }
                    if (in->position.getPipe() != j) {
                        reportError("instruction is quite lost\n");
                        return false;
                    }
                    if (in->isAbstractMove()) {
                        reportError("there's an abstract move in a raw table\n");
                        return false;
                    }
                    count++;
                }
            }
            int lcount = wt->linearInstructions.count;
            if (count != lcount) {
                reportError("linear count and itable mismatch (%d != %d) for %s\n",
                              lcount, count, s->streamName());
                wt->dump();
                return false;
            }
        }

        for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
            for (int k = 0; k != in->operandCount; k++) {
                Operand *operand = &in->operands[k];
                RegPoolEntry *reg;
                if (in->operands[k].pairedOperandIndexPlusOne) {
                    Operand *po = &in->operands[in->operands[k].pairedOperandIndexPlusOne - 1];
                    if (po->pairedOperandIndexPlusOne - 1 != k) {
                        reportError("badly linked web patch\n");
                        return false;
                    }
                    reg = in->operands[k].assignedRegister;
                    if (reg != po->assignedRegister) {
                        reportError("not same reg for iwp\n");
                        return false;
                    }
                } else
                    reg = in->operands[k].assignedRegister;
                RegPoolEntry *mreg = (RegPoolEntry *)in->operands[k].forcedBaseRegister;
                if (reg && reg->isVirtual && !in->operands[k].originalRegister) {
                    reportError("original reg not set\n");
                    return false;
                }
                if (mreg) {
                    if (!reg) {
                        reportError("original reg not set, but mapped base reg is\n");
                        return false;
                    }
                    if (mreg->isVirtual) {
                        reportError("mapped base reg is set - but it's virtual\n");
                        return false;
                    }
                }
                if (reg && reg->isVirtual != 1 && mreg && reg != mreg) {
                    reportError("register is base, but mapped base register, it isn't.. reg=%s mreg=%s\n",
                                  reg->registerName(), mreg->registerName());
                    return false;
                }
                if (reg) {
                    for (unsigned n = 0; n != (unsigned)reg->partCount; n++) {
                        OpSlot *slot = operand->getPart(n);
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
                            Instr *oi = location->instruction;
                            if (!oi) {
                                reportError("oploc not tied to instruction in checkopt3\n");
                                return false;
                            }
                            if (!oi->getOwningStream()) {
                                reportError("oploc instruction doesnt have ist set\n");
                                return false;
                            }
                            if (!location->isExt()) {
                                if (oi->getOwningStream() != s) {
                                    reportError("internal oploc but instruction not from this stream\n");
                                    return false;
                                }
                                if (wt->codeModeValue == 3) {
                                    if (__builtin_expect(oi->position.getRow() < last, 0)) {
                                        reportError("local oplocs  aren't in order\n");
                                        return false;
                                    }
                                    last = oi->position.getRow();
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
