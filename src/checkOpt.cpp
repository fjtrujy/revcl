#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"
#include "Instr.h"
#include "RegPool.h"
#include "ol_s.h"
#include "ol_s_inline.h"

bool checkOpt1(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("null work table\n");
            return false;
        }
        if (wt != &s->unk_148) {
            func_0804ad38("work table isn't input table\n");
            return false;
        }
        s->getAllocMode();
    }
    return true;
}

bool checkOpt2(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("null work table\n");
            return false;
        }
        if (s->func_0806d908() != 4) {
            if (s->getTable() != s->unk_214) {
                func_0804ad38("work table isn't opt table\n");
                return false;
            }
            if (!wt->unk_0b) {
                func_0804ad38("locks aren't valid for opt table\n");
                return false;
            }
        } else {
            if (s->getTable() != &s->unk_148) {
                func_0804ad38("checkopt2 - synth table wasn't input\n");
                return false;
            }
        }
        if (s->getAllocMode() != 3) {
            func_0804ad38("table %s didn't have external alloc\n", s->func_0806d674());
            return false;
        }
    }
    return true;
}

bool checkOpt3(Program *p)
{
    func_0804acb0("checkOpt2()\n");
    func_0804acb0("OLL free count = %d\n", func_080586c4());
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("null work table\n");
            return false;
        }

        if (wt->isAbstract()) {
            if (wt->unk_2c) {
                func_0804ad38("table set for abstract list\n");
                return false;
            }
            for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
                if (in->unk_13e8.getPipe() != -1) {
                    func_0804ad38("pipe is set for non-inserted instruction\n");
                    return false;
                }
                if (in->unk_13e8.getRow() != -1) {
                    func_0804ad38("rline is set for non-inserted\n");
                    return false;
                }
                if (in->isConcreteMove()) {
                    func_0804ad38("there's a concrete move no abstract list\n");
                    return false;
                }
            }
        }
        if (wt->unk_09 == 3) {
            int start = wt->func_0807411c();
            int end = wt->func_080741c0();
            if (start < 0 || end < start) {
                func_0804ad38("invalid table range\n");
                return false;
            }
            int count = 0;
            Instr *prev = 0;
            for (int i = start; i != end; i++) {
                Instr **ie = ((cls_unk30 *)wt)->getIE_ck(i);
                if (!ie) {
                    func_0804ad38("failed to get ientry from table\n");
                    return false;
                }
                for (int j = 0; j != 2; j++) {
                    Instr *in = ie[j];
                    if (!in)
                        continue;
                    if (((int)in & 1) == 1) {
                        func_0804ad38("found a key in an instruction slot\n");
                        return false;
                    }
                    if (in->unk_1c.unk_04 != prev) {
                        const char *name = "NO_BASE_INST";
                        if (in->unk_34)
                            name = in->unk_34->unk_04;
                        func_0804ad38("linear list isn't in order at guid=%d %s prev=%p\n",
                                      in->unk_13e0, name, in->unk_1c.unk_04);
                        wt->dump();
                        return false;
                    }
                    prev = in;
                    if (in->unk_13e8.getRow() != i) {
                        func_0804ad38("instruction is very lost\n");
                        return false;
                    }
                    if (in->unk_13e8.getPipe() != j) {
                        func_0804ad38("instruction is quite lost\n");
                        return false;
                    }
                    if (in->isAbstractMove()) {
                        func_0804ad38("there's an abstract move in a raw table\n");
                        return false;
                    }
                    count++;
                }
            }
            int lcount = wt->unk_88.unk_08;
            if (count != lcount) {
                func_0804ad38("linear count and itable mismatch (%d != %d) for %s\n",
                              lcount, count, s->func_0806d674());
                wt->dump();
                return false;
            }
        }

        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
            for (int k = 0; k != in->unk_138b; k++) {
                Operand *op = &in->unk_38[k];
                RegPoolEntry *reg;
                if (in->unk_38[k].unk_0d) {
                    Operand *po = &in->unk_38[in->unk_38[k].unk_0d - 1];
                    if (po->unk_0d - 1 != k) {
                        func_0804ad38("badly linked web patch\n");
                        return false;
                    }
                    reg = in->unk_38[k].unk_18;
                    if (reg != po->unk_18) {
                        func_0804ad38("not same reg for iwp\n");
                        return false;
                    }
                } else
                    reg = in->unk_38[k].unk_18;
                RegPoolEntry *mreg = (RegPoolEntry *)in->unk_38[k].unk_10;
                if (reg && reg->unk_14 && !in->unk_38[k].unk_14) {
                    func_0804ad38("original reg not set\n");
                    return false;
                }
                if (mreg) {
                    if (!reg) {
                        func_0804ad38("original reg not set, but mapped base reg is\n");
                        return false;
                    }
                    if (mreg->unk_14) {
                        func_0804ad38("mapped base reg is set - but it's virtual\n");
                        return false;
                    }
                }
                if (reg && reg->unk_14 != 1 && mreg && reg != mreg) {
                    func_0804ad38("register is base, but mapped base register, it isn't.. reg=%s mreg=%s\n",
                                  reg->func_0805a8e0(), mreg->func_0805a8e0());
                    return false;
                }
                if (reg) {
                    for (unsigned n = 0; n != (unsigned)reg->unk_28; n++) {
                        OpSlot *slot = op->getPart(n);
                        if (!slot)
                            continue;
                        ol_s *o = slot->unk_54.unk_00;
                        bool seenExt = false;
                        int last = -1000;
                        for (; o; o = o->unk_00) {
                            if (o->isExt()) {
                                if (!seenExt)
                                    seenExt = true;
                            } else if (seenExt) {
                                func_0804ad38("found an internal oploc after an external one\n");
                                return false;
                            }
                            Instr *oi = o->unk_04;
                            if (!oi) {
                                func_0804ad38("oploc not tied to instruction in checkopt3\n");
                                return false;
                            }
                            if (!oi->func_08054168()) {
                                func_0804ad38("oploc instruction doesnt have ist set\n");
                                return false;
                            }
                            if (!o->isExt()) {
                                if (oi->func_08054168() != s) {
                                    func_0804ad38("internal oploc but instruction not from this stream\n");
                                    return false;
                                }
                                if (wt->unk_09 == 3) {
                                    if (__builtin_expect(oi->unk_13e8.getRow() < last, 0)) {
                                        func_0804ad38("local oplocs  aren't in order\n");
                                        return false;
                                    }
                                    last = oi->unk_13e8.getRow();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    func_0804acb0("checkOpt3()..ok\n");
    return true;
}

extern const char iInfoLoadstoreRamtypeError[] = "loadstore instruction, but bad ramtype\n";
extern const char iInfoExplicitAfterImplicitError[] = "explicit operand after an implicit\n";
extern const char iInfoPatchedAfterPatchError[] = "patched operand is after patch\n";
