#ifndef SRED_SHARED_H
#define SRED_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"

extern int selectedCapPipelineProperties;
extern iInfo *selectedCapOpcodeInfo;
extern workTable *capInsertionWorkTable;
extern Instr *selectedCapOriginalInstruction;
extern RegPoolEntry *selectedCapRegister;
extern int selectedCapMask;

#endif
