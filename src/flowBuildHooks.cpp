#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "decodedLine.h"


bool noopFlowBuildHook(Program *)
{
    return true;
}

const char *getUniqueName(Program *p);
int checkedStringLength(const char *s);
bool expandRawTables(Program *p);
bool resolveStreamBranches(Program *p);
bool recalcFlowUIDs(Program *p);
bool buildUta(Program *prog, instStream *s, DecodedLine *blocks, int count);
bool buildSynth(Program *prog, instStream *s, DecodedLine *blocks, int count);
