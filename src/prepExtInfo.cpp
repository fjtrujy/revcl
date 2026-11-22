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

bool prepExtInfo(Program *p, instStream *s, bool noExt)
{
    workTable *wt = s->getTable();
    for (Instr *in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00)
        in->clearExternalTimingConstraints();
    wt->externalMinimumLength = 0;
    if (noExt)
        return 1;
    bool changed = 0;
    int id = s->streamId;
    regUse *ru;
    regUseList *ul = &wt->registerUses;
    for (ru = ul->head; ru; ru = ru->next) {
        RegPoolEntry *reg = ru->registerEntry;
        for (int k = 0; k != reg->partCount; k++) {
            regUseSlot *sl = &ru->slots[k];
            if (!sl) {
                reportError("failed to get rrp\n");
                return 0;
            }
            sl->earliestExternalCycle = 0;
            sl->latestExternalCycle = 30000;
            for (ol_s *ol = sl->inputLocations.head; ol; ol = ol->next) {
                Instr *in = ol->instruction;
                Operand *op = &in->operands[ol->operandIndex];
                OpSlot *part = op->getPart(ol->componentIndex);
                short best = 0;
                ol_s *e = part->unk_54.head;
                if (e) {
                    for (; e; e = e->next) {
                        int key = e->pathKey;
                        if (!key) {
                            reportError("can't have a null minkey for ext\n");
                            return 0;
                        }
                        Instr *setter = e->instruction;
                        Operand *src = operandAtLocation(e);
                        instStream *ss = setter->getOwningStream();
                        if (!ss) {
                            reportError("failed to get the setters ist\n");
                            return 0;
                        }
                        int sid = ss->streamId;
                        bool same = sid == id;
                        int kl = guessKeyLen(p, key, sid, id);
                        if (kl < 0) {
                            reportError("minimal length .. bah\n");
                            return 0;
                        }
                        int te = getToEnd(setter);
                        if (te < 0) {
                            reportError("distance to end -ve..error\n");
                            return 0;
                        }
                        int total = te + kl;
                        int d = src->outputLatency + op->additionalLatency - total - 1;
                        if ((setter->func_08055548() || same) && d > best)
                            best = d;
                    }
                    if (op->unk_0b)
                        in->raiseExternalInputPairedConstraint(best);
                    else
                        in->raiseExternalInputConstraint(best);
                } else {
                    reportError("no external setter for prepExtInfo.. register is %s.%d\n",
                                  reg->registerName(), k);
                    return 0;
                }
            }
            for (ol_s *ol = sl->outputLocations.head; ol; ol = ol->next) {
                Instr *in = ol->instruction;
                if (!in) {
                    reportError("not get visible output\n");
                    return 0;
                }
                short best = 0;
                Operand *op = &in->operands[ol->operandIndex];
                olList *xl = &op->getPart(ol->componentIndex)->unk_54;
                ol_s *e = xl->firstExternalLocation();
                if (!ol->isExtUsed() || e) {
                    if (!e || ol->isExtUsed()) {
                        for (; e; e = e->nextExternalLocation()) {
                            int key = e->pathKey;
                            if (!key) {
                                reportError("can't have a null minkey for ext\n");
                                return 0;
                            }
                            Instr *user = e->instruction;
                            Operand *dst = operandAtLocation(e);
                            if (!user || !dst) {
                                reportError("no setting instruction/operand\n");
                                return 0;
                            }
                            instStream *ss = user->getOwningStream();
                            if (!ss) {
                                reportError("failed to get the setters ist\n");
                                return 0;
                            }
                            int kl = guessKeyLen(p, key, id, ss->streamId);
                            if (kl < 0) {
                                reportError("minimal length .. bah\n");
                                return 0;
                            }
                            int fs = getFromStart(user);
                            if (fs < 0) {
                                reportError("distance to end -ve..error\n");
                                return 0;
                            }
                            int total = fs + kl;
                            int d = op->outputLatency + dst->additionalLatency - total - 1;
                            if (user->func_08055548() && d > best)
                                best = d;
                        }
                        if (op->unk_0b)
                            in->raiseExternalOutputPairedConstraint(best);
                        else
                            in->raiseExternalOutputConstraint(best);
                    } else {
                        reportError("operand doesn't think there's any external users, but - there's an entry on it's ext list\n");
                        ol->dumpOperandContext();
                        xl->validateOperandLocations();
                        return 0;
                    }
                } else {
                    reportError("operand is tagged as being used externally - but.. there's no ext entry on it's oll?\n");
                    ol->dumpOperandContext();
                    return 0;
                }
            }
            if (reg->unk_2c && !noExt) {
                ol_s *ol;
                for (ol = sl->externalInputLocations.head; ol; ol = ol->next) {
                    int key = ol->pathKey;
                    if (!key) {
                        reportError("can't have a null minkey for ext\n");
                        return 0;
                    }
                    Instr *in = ol->instruction;
                    if (!in) {
                        reportError("not get visible input\n");
                        return 0;
                    }
                    instStream *ss = in->getOwningStream();
                    if (!ss) {
                        reportError("failed to get the setters ist\n");
                        return 0;
                    }
                    int sourceId = ss->streamId;
                    int kl = guessKeyLen(p, key, sourceId, id);
                    if (kl < 0) {
                        reportError("minimal length .. bah\n");
                        return 0;
                    }
                    int te = getToEnd(in);
                    if (te < 0) {
                        reportError("distance to end -ve..error\n");
                        return 0;
                    }
                    int total = te + kl;
                    int d = in->baseIssueSpacing - total - 1;
                    if (in->func_08055548() && d > sl->earliestExternalCycle) {
                        changed = 1;
                        sl->earliestExternalCycle = d;
                    }
                }
                for (ol = sl->externalSourceLocations.head; ol; ol = ol->next) {
                    int key = ol->pathKey;
                    if (!key) {
                        reportError("can't have a null minkey for ext\n");
                        return 0;
                    }
                    Instr *in = ol->instruction;
                    if (!in) {
                        reportError("not get visible input\n");
                        return 0;
                    }
                    instStream *ss = in->getOwningStream();
                    if (!ss) {
                        reportError("failed to get the setters ist\n");
                        return 0;
                    }
                    int kl = guessKeyLen(p, key, id, ss->streamId);
                    if (kl < 0) {
                        reportError("minimal length .. bah\n");
                        return 0;
                    }
                    int fs = getFromStart(in);
                    if (fs < 0) {
                        reportError("distance from start -ve..error\n");
                        return 0;
                    }
                    int d = fs + kl + 1;
                    if (in->func_08055548() && d < sl->latestExternalCycle) {
                        changed = 1;
                        sl->latestExternalCycle = d;
                    }
                }
            }
            for (tPair *tp = sl->throughputPairs.head; tp; tp = tp->next) {
                instStream *a = (instStream *)tp->a;
                instStream *b = (instStream *)tp->b;
                if (!a || !b) {
                    reportError("there's no src or dst in a tpair\n");
                    return 0;
                }
                workTable *wa = a->getTable();
                workTable *wb = b->getTable();
                if (wa->codeModeValue == 3 && wb->codeModeValue == 3) {
                    int sourceId = a->streamId;
                    int k1 = guessKeyLen(p, tp->src, sourceId, id);
                    if (k1 < 0) {
                        reportError("failed to guess the key length\n");
                        return 0;
                    }
                    int k2 = guessKeyLen(p, tp->dst, id, b->streamId);
                    if (k2 < 0) {
                        reportError("distance to dst is bad\n");
                        return 0;
                    }
                    Instr *w = wa->linearInstructions.tail;
                    Operand *wo = 0;
                    for (; w; w = w->unk_1c.unk_04)
                        if ((wo = w->findOutputOperand(reg, 1 << k)) != 0)
                            break;
                    if (!wo) {
                        reportError("can't get last writer\n");
                        return 0;
                    }
                    Instr *u = wb->linearInstructions.head;
                    Operand *uo = 0;
                    for (; u; u = u->unk_1c.unk_00)
                        if ((uo = u->findInputOperand(reg, 1 << k)) != 0)
                            break;
                    if (!uo) {
                        reportError("can't get first user \n");
                        return 0;
                    }
                    int te = getToEnd(w);
                    if (te < 0) {
                        reportError("<ve to end \n");
                        return 0;
                    }
                    int fs = getFromStart(u);
                    int total = te + k1 + k2 + fs;
                    int d = wo->outputLatency + uo->additionalLatency - total - 1;
                    if (d > wt->externalMinimumLength)
                        wt->externalMinimumLength = d;
                }
            }
        }
    }
    if (changed) {
        for (Instr *in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00)
            for (int i = 0; i != in->operandCount; i++) {
                Operand *op = &in->operands[i];
                RegPoolEntry *r = isRegisterOperand(op, 2) ? op->assignedRegister : 0;
                if (r && r->unk_2c) {
                    regUse *u = ul->findByRegister(r);
                    if (!u) {
                        reportError("failed to find regref\n");
                        exitAfterCleanup(-1);
                    }
                    for (int k = 0; k != r->partCount; k++)
                        if ((op->componentMask >> k) & 1) {
                            regUseSlot *slot = &u->slots[k];
                            in->raiseExternalInputPairedConstraint(slot->earliestExternalCycle);
                            int bound = in->baseIssueSpacing - slot->latestExternalCycle;
                            in->raiseExternalOutputPairedConstraint(bound);
                        }
                }
            }
    }
    Instr *in;
    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00)
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *reg = op->assignedRegister;
            if (reg) {
                regUse *u = ul->findByRegister(reg);
                if (isRegisterOperand(op, 1))
                    op->unk_28 = u->hazardMask & op->externalPartMask;
                if (isRegisterOperand(op, 2))
                    op->unk_28 = u->unk_13 & op->externalPartMask;
                op->unk_28 |= u->hazardBroadcastMask;
            }
        }
    int endType = s->getEndType();
    if ((unsigned char)endType == 2) {
        goto branchBound;
    }
    if ((unsigned char)endType == 3) {
        goto branchBound;
    }
success:
    return 1;
branchBound:
    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00)
        if (in->opcodeId() == 0xa5)
            in->raiseExternalOutputConstraint(2);
    goto success;
}

extern const char prepRawAddInstrError[] = "iTable - failed to add an instruction\n";
