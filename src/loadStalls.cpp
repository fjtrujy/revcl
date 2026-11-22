#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "loadStalls_shared.h"

bool loadStalls(Program *p)
{
    infoList *st = &p->proc.stalls;
    for (stallNode *n = (stallNode *)st->head; n; n = (stallNode *)n->next) {
        Instr *in = n->user.instruction;
        if (!in) {
            reportError("no instruction in stall\n");
            return 0;
        }
        cls_unk30 *owner = in->ownerTable;
        if (!owner) {
            reportError("instruction has no owner!\n");
            return 0;
        }
        if (owner->codeModeValue != 3) {
            reportError("bad code mode for stall\n");
            return 0;
        }
        InstructionPosition *loc = &in->position;
        wtRow *ie = (wtRow *)owner->getIE_ck(loc->loopRow);
        if (!ie) {
            reportError("couldn't get ie for instruction\n");
            return 0;
        }
        if (ie->entries[loc->pipelineIndex].instruction != in) {
            reportError("instruction wasn't where I thought it was\n");
            return 0;
        }
        for (int i = 0; i != 3; i++) {
            if (n->stallByType[i]) {
                if (!ie->addStall(i, n->stallByType[i])) {
                    reportError("failed to load a stall\n");
                    return 0;
                }
            }
        }
    }
    return 1;
}

extern const char wtRowGetIPMaxError[] = "iEntry::GetI() - p>=MAX_IPIPES\n";
extern const char wtRowAddStallLenError[] = "can't add a zero or -ve length stall\n";

