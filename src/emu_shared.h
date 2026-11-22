#ifndef EMU_SHARED_H
#define EMU_SHARED_H

#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "os.h"
#include "RegPool.h"
#include "ol_s_inline.h"

int emulateInstructionRow(Program *p, wtRow *row, int line, bool uta, bool commit);
bool advanceProcessorCycle(Program *p, bool flag);

static inline int clampByte(int v)
{
    return v > 255 ? 255 : v;
}

#endif
