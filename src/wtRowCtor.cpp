#pragma implementation "wtEntry.h"
#include "Program.h"
#include "workTable.h"
#include "infoList.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
// wtRow methods are compiled in this same translation unit to reproduce
// the original constant-pool alignment and linked data layout.
#include "loadStalls_shared.h"

wtRow::wtRow()
{
    unk_0c = 0;
    unk_08 = 0;
    unk_0a = 0;
    unk_09 = 0;
}

wtRow::~wtRow()
{
}

