#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "loadStalls_shared.h"

bool loadStalls(Program *program)
{
    infoList *stallList = &program->proc.stalls;
    for (stallNode *currentStall = (stallNode *)stallList->head; currentStall; currentStall = (stallNode *)currentStall->next) {
        Instr *userInstruction = currentStall->user.instruction;
        if (!userInstruction) {
            reportError("no instruction in stall\n");
            return 0;
        }
        InstructionOwnerTableView *owner = userInstruction->ownerTable;
        if (!owner) {
            reportError("instruction has no owner!\n");
            return 0;
        }
        if (owner->codeModeValue != 3) {
            reportError("bad code mode for stall\n");
            return 0;
        }
        InstructionPosition *loc = &userInstruction->position;
        ScheduleRow *workRow = (ScheduleRow *)owner->getIE_ck(loc->loopRow);
        if (!workRow) {
            reportError("couldn't get ie for instruction\n");
            return 0;
        }
        if (workRow->entries[loc->pipelineIndex].instruction != userInstruction) {
            reportError("instruction wasn't where I thought it was\n");
            return 0;
        }
        for (int stallTypeIndex = 0; stallTypeIndex != 3; stallTypeIndex++) {
            if (currentStall->stallByType[stallTypeIndex]) {
                if (!workRow->addStall(stallTypeIndex, currentStall->stallByType[stallTypeIndex])) {
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

