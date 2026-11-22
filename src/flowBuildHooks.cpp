#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "flowBlock.h"


bool func_08085a9c(Program *)
{
    return true;
}

const char *getUniqueName(Program *p);
int mstrlen(const char *s);
bool expandRawTables(Program *p);
bool resolveStreamBranches(Program *p);
bool recalcFlowUIDs(Program *p);
bool buildUta(Program *prog, instStream *s, flowBlock *blocks, int count);
bool buildSynth(Program *prog, instStream *s, flowBlock *blocks, int count);
