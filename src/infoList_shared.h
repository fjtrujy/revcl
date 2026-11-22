#ifndef INFOLIST_SHARED_H
#define INFOLIST_SHARED_H

#include "infoList.h"
#include "Instr.h"
#include "instStream.h"
#include "RegPool.h"
#include "workTable.h"
#include "ubsList.h"
#include "os.h"

extern char *stallNames_080b3244[3];
int getOpcodeId(iInfo *ii);
iInfo *findOpcodeInfoByMnemonic(const char *name, bool exact);

#endif
