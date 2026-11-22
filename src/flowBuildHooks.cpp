#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "decodedLine.h"


bool noopFlowBuildHook(Program * unusedProgram)
{
    return true;
}

const char *getUniqueName(Program *p);
int checkedStringLength(const char *s);
bool expandRawTables(Program *p);
bool resolveStreamBranches(Program *p);
bool recalcFlowUIDs(Program *p);
bool buildUta(Program *prog, InstructionStream *s, DecodedLine *blocks, int count);
bool buildSynth(Program *prog, InstructionStream *s, DecodedLine *blocks, int count);
