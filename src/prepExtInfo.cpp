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
    for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        in->func_08055f4c();
    wt->unk_44 = 0;
    if (noExt)
        return 1;
    bool changed = 0;
    int id = s->unk_2c0;
    regUse *ru;
    regUseList *ul = &wt->unk_14;
    for (ru = ul->unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        for (int k = 0; k != reg->unk_28; k++) {
            regUseSlot *sl = &ru->unk_20[k];
            if (!sl) {
                func_0804ad38("failed to get rrp\n");
                return 0;
            }
            sl->unk_30 = 0;
            sl->unk_32 = 30000;
            for (ol_s *ol = sl->unk_00.unk_00; ol; ol = ol->unk_00) {
                Instr *in = ol->unk_04;
                Operand *op = &in->unk_38[ol->unk_10];
                OpSlot *part = op->getPart(ol->unk_11);
                short best = 0;
                ol_s *e = part->unk_54.unk_00;
                if (e) {
                    for (; e; e = e->unk_00) {
                        int key = e->unk_08;
                        if (!key) {
                            func_0804ad38("can't have a null minkey for ext\n");
                            return 0;
                        }
                        Instr *setter = e->unk_04;
                        Operand *src = func_0805802c(e);
                        instStream *ss = setter->func_08054168();
                        if (!ss) {
                            func_0804ad38("failed to get the setters ist\n");
                            return 0;
                        }
                        int sid = ss->unk_2c0;
                        bool same = sid == id;
                        int kl = guessKeyLen(p, key, sid, id);
                        if (kl < 0) {
                            func_0804ad38("minimal length .. bah\n");
                            return 0;
                        }
                        int te = getToEnd(setter);
                        if (te < 0) {
                            func_0804ad38("distance to end -ve..error\n");
                            return 0;
                        }
                        int total = te + kl;
                        int d = src->unk_0a + op->unk_29 - total - 1;
                        if ((setter->func_08055548() || same) && d > best)
                            best = d;
                    }
                    if (op->unk_0b)
                        in->func_08055fbc(best);
                    else
                        in->func_08055fdc(best);
                } else {
                    func_0804ad38("no external setter for prepExtInfo.. register is %s.%d\n",
                                  reg->func_0805a8e0(), k);
                    return 0;
                }
            }
            for (ol_s *ol = sl->unk_08.unk_00; ol; ol = ol->unk_00) {
                Instr *in = ol->unk_04;
                if (!in) {
                    func_0804ad38("not get visible output\n");
                    return 0;
                }
                short best = 0;
                Operand *op = &in->unk_38[ol->unk_10];
                olList *xl = &op->getPart(ol->unk_11)->unk_54;
                ol_s *e = xl->func_080583d8();
                if (!ol->isExtUsed() || e) {
                    if (!e || ol->isExtUsed()) {
                        for (; e; e = e->func_08058414()) {
                            int key = e->unk_08;
                            if (!key) {
                                func_0804ad38("can't have a null minkey for ext\n");
                                return 0;
                            }
                            Instr *user = e->unk_04;
                            Operand *dst = func_0805802c(e);
                            if (!user || !dst) {
                                func_0804ad38("no setting instruction/operand\n");
                                return 0;
                            }
                            instStream *ss = user->func_08054168();
                            if (!ss) {
                                func_0804ad38("failed to get the setters ist\n");
                                return 0;
                            }
                            int kl = guessKeyLen(p, key, id, ss->unk_2c0);
                            if (kl < 0) {
                                func_0804ad38("minimal length .. bah\n");
                                return 0;
                            }
                            int fs = getFromStart(user);
                            if (fs < 0) {
                                func_0804ad38("distance to end -ve..error\n");
                                return 0;
                            }
                            int total = fs + kl;
                            int d = op->unk_0a + dst->unk_29 - total - 1;
                            if (user->func_08055548() && d > best)
                                best = d;
                        }
                        if (op->unk_0b)
                            in->func_08055ffc(best);
                        else
                            in->func_0805601c(best);
                    } else {
                        func_0804ad38("operand doesn't think there's any external users, but - there's an entry on it's ext list\n");
                        ol->func_08057fec();
                        xl->func_08057e9c();
                        return 0;
                    }
                } else {
                    func_0804ad38("operand is tagged as being used externally - but.. there's no ext entry on it's oll?\n");
                    ol->func_08057fec();
                    return 0;
                }
            }
            if (reg->unk_2c && !noExt) {
                ol_s *ol;
                for (ol = sl->unk_18.unk_00; ol; ol = ol->unk_00) {
                    int key = ol->unk_08;
                    if (!key) {
                        func_0804ad38("can't have a null minkey for ext\n");
                        return 0;
                    }
                    Instr *in = ol->unk_04;
                    if (!in) {
                        func_0804ad38("not get visible input\n");
                        return 0;
                    }
                    instStream *ss = in->func_08054168();
                    if (!ss) {
                        func_0804ad38("failed to get the setters ist\n");
                        return 0;
                    }
                    int sourceId = ss->unk_2c0;
                    int kl = guessKeyLen(p, key, sourceId, id);
                    if (kl < 0) {
                        func_0804ad38("minimal length .. bah\n");
                        return 0;
                    }
                    int te = getToEnd(in);
                    if (te < 0) {
                        func_0804ad38("distance to end -ve..error\n");
                        return 0;
                    }
                    int total = te + kl;
                    int d = in->unk_13cf - total - 1;
                    if (in->func_08055548() && d > sl->unk_30) {
                        changed = 1;
                        sl->unk_30 = d;
                    }
                }
                for (ol = sl->unk_28.unk_00; ol; ol = ol->unk_00) {
                    int key = ol->unk_08;
                    if (!key) {
                        func_0804ad38("can't have a null minkey for ext\n");
                        return 0;
                    }
                    Instr *in = ol->unk_04;
                    if (!in) {
                        func_0804ad38("not get visible input\n");
                        return 0;
                    }
                    instStream *ss = in->func_08054168();
                    if (!ss) {
                        func_0804ad38("failed to get the setters ist\n");
                        return 0;
                    }
                    int kl = guessKeyLen(p, key, id, ss->unk_2c0);
                    if (kl < 0) {
                        func_0804ad38("minimal length .. bah\n");
                        return 0;
                    }
                    int fs = getFromStart(in);
                    if (fs < 0) {
                        func_0804ad38("distance from start -ve..error\n");
                        return 0;
                    }
                    int d = fs + kl + 1;
                    if (in->func_08055548() && d < sl->unk_32) {
                        changed = 1;
                        sl->unk_32 = d;
                    }
                }
            }
            for (tPair *tp = sl->unk_10.head; tp; tp = tp->next) {
                instStream *a = (instStream *)tp->a;
                instStream *b = (instStream *)tp->b;
                if (!a || !b) {
                    func_0804ad38("there's no src or dst in a tpair\n");
                    return 0;
                }
                workTable *wa = a->getTable();
                workTable *wb = b->getTable();
                if (wa->unk_09 == 3 && wb->unk_09 == 3) {
                    int sourceId = a->unk_2c0;
                    int k1 = guessKeyLen(p, tp->src, sourceId, id);
                    if (k1 < 0) {
                        func_0804ad38("failed to guess the key length\n");
                        return 0;
                    }
                    int k2 = guessKeyLen(p, tp->dst, id, b->unk_2c0);
                    if (k2 < 0) {
                        func_0804ad38("distance to dst is bad\n");
                        return 0;
                    }
                    Instr *w = wa->unk_88.unk_04;
                    Operand *wo = 0;
                    for (; w; w = w->unk_1c.unk_04)
                        if ((wo = w->func_080557a4(reg, 1 << k)) != 0)
                            break;
                    if (!wo) {
                        func_0804ad38("can't get last writer\n");
                        return 0;
                    }
                    Instr *u = wb->unk_88.unk_00;
                    Operand *uo = 0;
                    for (; u; u = u->unk_1c.unk_00)
                        if ((uo = u->func_08055720(reg, 1 << k)) != 0)
                            break;
                    if (!uo) {
                        func_0804ad38("can't get first user \n");
                        return 0;
                    }
                    int te = getToEnd(w);
                    if (te < 0) {
                        func_0804ad38("<ve to end \n");
                        return 0;
                    }
                    int fs = getFromStart(u);
                    int total = te + k1 + k2 + fs;
                    int d = wo->unk_0a + uo->unk_29 - total - 1;
                    if (d > wt->unk_44)
                        wt->unk_44 = d;
                }
            }
        }
    }
    if (changed) {
        for (Instr *in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
            for (int i = 0; i != in->unk_138b; i++) {
                Operand *op = &in->unk_38[i];
                RegPoolEntry *r = isRegisterOperand(op, 2) ? op->unk_18 : 0;
                if (r && r->unk_2c) {
                    regUse *u = ul->func_0805b5dc(r);
                    if (!u) {
                        func_0804ad38("failed to find regref\n");
                        func_0804adf0(-1);
                    }
                    for (int k = 0; k != r->unk_28; k++)
                        if ((op->unk_07 >> k) & 1) {
                            regUseSlot *slot = &u->unk_20[k];
                            in->func_08055fbc(slot->unk_30);
                            int bound = in->unk_13cf - slot->unk_32;
                            in->func_08055ffc(bound);
                        }
                }
            }
    }
    Instr *in;
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        for (int i = 0; i != in->unk_138b; i++) {
            Operand *op = &in->unk_38[i];
            RegPoolEntry *reg = op->unk_18;
            if (reg) {
                regUse *u = ul->func_0805b5dc(reg);
                if (isRegisterOperand(op, 1))
                    op->unk_28 = u->unk_12 & op->unk_27;
                if (isRegisterOperand(op, 2))
                    op->unk_28 = u->unk_13 & op->unk_27;
                op->unk_28 |= u->unk_14;
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
    for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
        if (in->func_08055ac4() == 0xa5)
            in->func_0805601c(2);
    goto success;
}

extern const char prepRawAddInstrError[] = "iTable - failed to add an instruction\n";
