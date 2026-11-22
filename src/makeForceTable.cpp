#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(ScheduleTable *t);
bool fixupCloneFdeps(ScheduleTable *wt);

static inline void copyShort(short &destination, const short &from)
{
    destination = from;
}

ScheduleTable *prepUta(Program *p, InstructionStream *s, int loopSize, char smart);
extern int nextReservedInstructionKey;

ScheduleTable *makeForceTable(Program *program, InstructionStream *stream)
{
    ScheduleTable *scheduleTable = stream->getTable();
    int mode = scheduleTable->codeModeValue;
    if ((unsigned char)mode != 3) {
        const char *modeName = codeModeName(mode);
        reportError("it_old is not raw mode - %s\n", modeName);
        return 0;
    }
    ScheduleTable *newWorkTable = prepUta(program, stream, 0, 0);
    if (!newWorkTable) {
        reportError("failed to make a lock table for makeForceTable\n");
        return 0;
    }
    mode = newWorkTable->codeModeValue;
    if ((unsigned char)mode != 2) {
        const char *modeName = codeModeName(mode);
        reportError("it_new is not raw mode .. %s\n", modeName);
        return 0;
    }
    int start = scheduleTable->getRangeStart();
    int end = scheduleTable->getRangeEnd();
    newWorkTable->setRange(start, end);
    for (Instr *instruction = newWorkTable->pendingInstructions.head; instruction; instruction = newWorkTable->pendingInstructions.head) {
        Instr *clone = instruction->cloneImage;
        InstructionPosition pos;
        InstructionPosition *instructionPosition = &clone->position;
        pos.linearRow = instructionPosition->linearRow;
        pos.loopRow = instructionPosition->loopRow;
        pos.loopIteration = instructionPosition->loopIteration;
        pos.pipelineIndex = instructionPosition->pipelineIndex;
        if (!newWorkTable->forceInsert(instruction, &pos)) {
            reportError("failed to force an insert into table\n");
            delete newWorkTable;
            return 0;
        }
    }
    newWorkTable->changeMode(3);
    newWorkTable->setTableRawMode(scheduleTable->getTableRawMode());
    return newWorkTable;
}

extern const char lpWorkSavedTableError[] = "there's already a saved work table\n";
extern const char lpWorkHtNewError[] = "can't have htnew before minip\n";
extern const char lpWorkCloneTableError[] = "failed to locally clone table\n";
extern const char lpWorkNoTableError[] = "no work table set for make_lpWork \n";
extern const char lpWorkDisposeNoTableError[] = "no work table set for dispose lp table\n";
extern const char lpWorkDisposeNoSavedError[] = "there's no saved work table for dispose\n";
extern const char lsqChainHopeError[] = "well, let's hope never wind up in this function!\n";
extern const char lsqChainLinkError[] = "failed to link parts together\n";
extern const char lsqChainNotInputError[] = "isn't an input, and this code is ..not taking any chances!\n";
extern const char lsqChainAddError[] = "failed to add the new instruction\n";
extern const char lsqChainNewInstrError[] = "failed to get new instruction\n";
