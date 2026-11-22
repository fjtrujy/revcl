#ifndef SRED_SHARED_H
#define SRED_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"

extern int selectedCapPipelineProperties;
extern OpcodeDescriptor *selectedCapOpcodeInfo;
extern ScheduleTable *capInsertionWorkTable;
extern Instr *selectedCapOriginalInstruction;
extern RegisterEntry *selectedCapRegister;
extern int selectedCapMask;

#endif
