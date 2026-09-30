#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"

const char *codeModeName(int mode);
int emuInst(Program *p, wtRow *row, int line, bool a, bool b);

bool emuTable(Program *p, workTable *wt)
{
    if (!wt) {
        func_0804ac7c("emu - passed a null table ptr!\n");
        return true;
    }
    if (wt->func_080741b4() || wt->unk_3c) {
        func_0804ad38("mlid is set, or loop size is set for emu..which is now kinda tricky..\n");
        return false;
    }
    wt->unk_0c = -1;
    switch (wt->unk_09) {
    case 1:
    case 4: {
        Instr *i = wt->unk_88.unk_00;
        wtRow row;
        for (; i; i = i->unk_1c.unk_00) {
            row.func_080721f8();
            row.unk_00[0].unk_00 = i;
            int r = emuInst(p, &row, -1, true, true);
            row.unk_00[0].unk_00 = 0;
            if (r < 0) {
                func_0804ad38("failed to uta emu instruction\n");
                i->func_08054008(false);
                return false;
            }
        }
        break;
    }
    case 3: {
        int skipped = 0;
        for (int line = wt->func_0807411c(); line != wt->func_080741c0(); line++) {
            wtRow *r = wt->func_080724f0(line);
            if (!r) {
                func_0804ad38("emuuta - failed to get an ie\n");
                return false;
            }
            if (r->unk_08) {
                for (int k = 0; k != 2; k++)
                    if (r->unk_00[k].unk_00) {
                        func_0804ad38("m_bSkip set .. but line contains instructions\n");
                        return false;
                    }
                skipped++;
                continue;
            }
            if (emuInst(p, r, line, false, true) < 0) {
                func_0804ad38("failed to Emu an inst\n");
                r->func_0807222c(-1);
                return false;
            }
        }
        wt->unk_0c = p->unk_80;
        skipped += p->unk_80;
        if (skipped < wt->getPhysicalLen()) {
            func_0804ad38("logical len < physical len.. %d<%d!\n", p->unk_80, wt->getPhysicalLen());
            wt->func_08072c70();
            return false;
        }
        break;
    }
    default:
        func_0804ad38("emu - can't emu table from %s of type %s\n",
                      wt->ownerName(), codeModeName(wt->unk_09));
        break;
    }
    return true;
}

bool func_080642ec(Program *p, instStream *s)
{
    int count;
    Instr *i;
    workTable *wt;

    wt = s->getWorkTable();
    int linear = wt->unk_c8;
    int iCount = wt->unk_90;
    if (linear != iCount) {
        func_0804ad38("linear count not same as iCount %d!=%d\n", linear, iCount);
        return false;
    }
    if (!emuTable(p, wt)) {
        func_0804ad38("Processor : failed to emu itable from '%s'\n", s->func_0806d674());
        return false;
    }
    count = 0;
    for (i = wt->unk_88.unk_00; i; i = i->unk_1c.unk_00) {
        count++;
        for (int op = 0; op != i->unk_138b; op++) {
            Operand *o = &i->unk_38[op];
            RegPoolEntry *reg = o->unk_18;
            if (!reg)
                continue;
            regUse *ru = wt->unk_14.func_0805b5dc(reg);
            if (!ru) {
                ru = wt->unk_14.AddNewRegRef(reg);
                if (!ru) {
                    func_0804ad38("failed to add reg ref\n");
                    return false;
                }
            }
            if (o->isReg2())
                ru->unk_0f |= o->unk_07;
            if (o->isReg())
                ru->unk_0e |= o->unk_07;
            if (o->isReg2()) {
                if (o->unk_27) {
                    func_0804ad38("operand had xbits set for output!\n");
                    return false;
                }
            }
            ru->unk_0c |= o->unk_27;
            for (int k = 0; k != reg->unk_28; k++) {
                if ((o->unk_27 >> k) & 1) {
                    if (ru->unk_15 == -1)
                        ru->unk_15 = o->unk_0b;
                    else if (ru->unk_15 != (unsigned char)o->unk_0b)
                        func_0804ac7c("differing block modes for external input for %s..taking first encountered\n",
                                      reg->func_0805a8e0());
                    if (!ru->unk_20[k].unk_00.func_080580fc(i, op, k, false)) {
                        func_0804ad38("failed to add oploc user!\n");
                        return false;
                    }
                }
                if (((o->unk_07 >> k) & 1) && reg->unk_2c && o->isReg2()) {
                    if (!ru->unk_20[k].unk_20.func_080580fc(i, op, k, false)) {
                        func_0804ad38("failed to add a local setter for a thruput check\n");
                        func_0804ad38("register is %s\n", reg->func_0805a8e0());
                        return false;
                    }
                }
            }
        }
    }
    if (count != wt->unk_c8) {
        func_0804ad38("icount mismatch (%d apparently in list, but %d in table?)\n", count, wt->unk_c8);
        return false;
    }
    for (regUse *ru = wt->unk_14.unk_04; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (!reg) {
            func_0804ad38("a reg ref had no register!\n");
            return false;
        }
        for (int part = 0; part != reg->unk_28; part++) {
            rPart *rp = &reg->unk_44[part];
            regUseSlot *slot = &ru->unk_20[part];
            slot->unk_3c[0] = rp->unk_00[4];
            slot->unk_3c[1] = rp->unk_00[5];
            slot->unk_3c[2] = rp->unk_00[6];
            slot->unk_3c[3] = rp->unk_00[7];
            slot->unk_3c[4] = rp->unk_00[8];
            slot->unk_3c[5] = rp->unk_00[9];
            slot->unk_3c[6] = rp->unk_00[10];
            slot->unk_3c[7] = rp->unk_00[11];
            slot->unk_3c[8] = rp->unk_00[12];
            slot->unk_3c[9] = rp->unk_00[13];
            slot->unk_3c[10] = rp->unk_00[14];
            slot->unk_3c[11] = rp->unk_00[15];
            slot->unk_3c[12] = rp->unk_00[16];
            slot->unk_34[1] = rp->unk_00[1];
            slot->unk_34[0] = rp->unk_00[0];
            slot->unk_34[3] = rp->unk_00[2];
            slot->unk_34[2] = rp->unk_00[3];
            if ((ru->unk_0f >> part) & 1) {
                ol_s *out = &rp->unk_24;
                if (out->unk_04 && !slot->addOutput(out)) {
                    func_0804ad38("failed to add an output to an rrp\n");
                    return false;
                }
                for (int j = 0; j != 4; j++) {
                    ol_s *ol = rp->func_0805a968(j);
                    if (!ol) {
                        func_0804ad38("failed to get a pool entry\n");
                        return false;
                    }
                    if (ol->unk_12 && !slot->addOutput(ol)) {
                        func_0804ad38("failed to add a pending output to an rrp\n");
                        return false;
                    }
                }
                if (slot->unk_08.unk_04 <= 0) {
                    func_0804ad38("no or -ve outputs during out build..nOut=%d!\n", slot->unk_08.unk_04);
                    return false;
                }
            }
        }
    }
    return true;
}
