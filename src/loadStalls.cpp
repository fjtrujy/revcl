#pragma implementation "wtEntry.h"
#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"

bool loadStalls(Program *p)
{
    infoList *st = &p->proc.unk_74;
    for (stallNode *n = (stallNode *)st->unk_04; n; n = (stallNode *)n->unk_04) {
        Instr *in = n->unk_18.unk_00;
        if (!in) {
            func_0804ad38("no instruction in stall\n");
            return 0;
        }
        cls_unk30 *owner = in->unk_30;
        if (!owner) {
            func_0804ad38("instruction has no owner!\n");
            return 0;
        }
        if (owner->unk_09 != 3) {
            func_0804ad38("bad code mode for stall\n");
            return 0;
        }
        cls08058718 *loc = &in->unk_13e8;
        wtRow *ie = (wtRow *)owner->getIE_ck(loc->unk_04);
        if (!ie) {
            func_0804ad38("couldn't get ie for instruction\n");
            return 0;
        }
        if (ie->unk_00[loc->unk_02].unk_00 != in) {
            func_0804ad38("instruction wasn't where I thought it was\n");
            return 0;
        }
        for (int i = 0; i != 3; i++) {
            if (n->unk_24[i]) {
                if (!ie->addStall(i, n->unk_24[i])) {
                    func_0804ad38("failed to load a stall\n");
                    return 0;
                }
            }
        }
    }
    return 1;
}
