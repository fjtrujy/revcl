#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"

bool optReset(Program *p);
int setRangeFrom(workTable *t);
bool fixupCloneFdeps(workTable *wt);

static inline void copyShort(short &to, const short &from)
{
    to = from;
}

workTable *prepUta(Program *p, instStream *s, int loopSize, char smart);
extern int nextReservedInstructionKey;

workTable *makeForceTable(Program *program, instStream *stream)
{
    workTable *scheduleTable = stream->getTable();
    int mode = scheduleTable->codeModeValue;
    if ((unsigned char)mode != 3) {
        const char *modeName = codeModeName(mode);
        reportError("it_old is not raw mode - %s\n", modeName);
        return 0;
    }
    workTable *nt = prepUta(program, stream, 0, 0);
    if (!nt) {
        reportError("failed to make a lock table for makeForceTable\n");
        return 0;
    }
    mode = nt->codeModeValue;
    if ((unsigned char)mode != 2) {
        const char *modeName = codeModeName(mode);
        reportError("it_new is not raw mode .. %s\n", modeName);
        return 0;
    }
    int start = scheduleTable->getRangeStart();
    int end = scheduleTable->getRangeEnd();
    nt->setRange(start, end);
    for (Instr *instruction = nt->pendingInstructions.head; instruction; instruction = nt->pendingInstructions.head) {
        Instr *clone = instruction->cloneImage;
        InstructionPosition pos;
        InstructionPosition *instructionPosition = &clone->position;
        pos.linearRow = instructionPosition->linearRow;
        pos.loopRow = instructionPosition->loopRow;
        pos.loopIteration = instructionPosition->loopIteration;
        pos.pipelineIndex = instructionPosition->pipelineIndex;
        if (!nt->forceInsert(instruction, &pos)) {
            reportError("failed to force an insert into table\n");
            delete nt;
            return 0;
        }
    }
    nt->changeMode(3);
    nt->setTableRawMode(scheduleTable->getTableRawMode());
    return nt;
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
