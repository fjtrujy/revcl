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
            row.unk_00[0] = i;
            int r = emuInst(p, &row, -1, true, true);
            row.unk_00[0] = 0;
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
            if (r->unk_08[0]) {
                for (int k = 0; k != 2; k++)
                    if (r->unk_00[k]) {
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
