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

#include "Block.h"

unsigned char iInfo::func_0806ffb4()
{
    if (kind() == 3)
        return unk_19;
    return 0;
}

unsigned char iInfo::ramType()
{
    if (kind() == 4) {
        unsigned char t = unk_19;
        if ((unsigned char)(t - 1) <= 1)
            return t;
        func_0804ad38("loadstore instruction, but bad ramtype\n");
        func_0804adf0(-1);
    }
    return 0;
}

int iInfo::func_08070020()
{
    if (kind() == 2)
        return unk_19;
    return 0;
}

bool iInfo::func_08070044()
{
    return kind() == 0;
}

char iInfo::func_0807005c()
{
    return kind() == 0 && unk_19 == 0;
}

void iInfo::func_08070080()
{
    int i;
    for (i = 0; i != unk_44[i].unk_00; i++)
        ;
}

void iInfo::func_080700a4()
{
}

bool iInfo::check()
{
    bool noImplicitYet = true;
    iOpDesc *d = unk_44;
    for (int i = 0; d->unk_00; i++, d++) {
        if (d->unk_06) {
            if (!d->unk_02) {
                func_0804ad38("implicit web patch set on explicit operand\n");
                return false;
            }
            if (d->unk_01 != 1) {
                func_0804ad38("iInfo: check - patch set on non-input (%d)\n", d->unk_00);
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_01 != 2) {
                func_0804ad38("iInfo: check patch not point to output\n");
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_02) {
                func_0804ad38("implicit web patch links to implicit operand\n");
                return false;
            }
            if (d->unk_06 - 1 > i) {
                func_0804ad38("patched operand is after patch\n");
                return false;
            }
        }
        if (d->unk_02)
            noImplicitYet = false;
        else if (!noImplicitYet) {
            func_0804ad38("explicit operand after an implicit\n");
            return false;
        }
    }
    return true;
}

bool checkTable(iInfo *tab, int count)
{
    if (!tab) {
        func_0804ad38("checkTable - passed a null table\n");
        return false;
    }
    int i;
    for (i = 0; tab->unk_04; i++, tab++) {
        if (!tab->check()) {
            func_0804ad38("checkTable failed\n");
            return false;
        }
    }
    if (i != count) {
        func_0804ad38("instruction count mismatch %d!=%d\n", i, count);
        return false;
    }
    return true;
}

int iInfo::func_080701f4()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 1) & 1;
}

int iInfo::func_0807021c()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 3) & 1;
}

int iInfo::func_08070244()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 2) & 1;
}

int iInfo::func_0807026c()
{
    int r = 0;
    if (!(unk_18 & 0xf8)) {
        int k2 = kind() == 2 ? unk_19 : 0;
        if (!k2) {
            int rt;
            if (kind() == 4) {
                unsigned char t = unk_19;
                if ((unsigned char)(t - 1) <= 1)
                    rt = t;
                else {
                    func_0804ad38("loadstore instruction, but bad ramtype\n");
                    func_0804adf0(-1);
                    goto none;
                }
            } else
none:
                rt = 0;
            if (rt != 2) {
                int k3 = kind() == 3 ? unk_19 : 0;
                if (!k3)
                    r = 1;
            }
        }
    }
    return r;
}

int func_08070300(unsigned char x)
{
    int r = 0;
    if (x) {
        unsigned int f = x & 0x60;
        r = (f >> 5) + 1;
    }
    return r;
}

int func_0807031c(unsigned char)
{
    return 0;
}

unsigned char func_08070324(unsigned char x)
{
    return x >> 7;
}

Instr *getMove(int type, int);
