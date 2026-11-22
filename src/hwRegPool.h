#ifndef HWREGPOOL_H
#define HWREGPOOL_H

#include "RegPool.h"

class InstructionStream;
class ScheduleTable;

class hwRegPool : public RegPool {
public:
    hwRegPool();
    virtual ~hwRegPool();
    virtual bool assignExt(InstructionStream *s);                        // slot 2
    virtual bool discardDisjointExternalRegister(InstructionStream *s, RegisterEntry *reg); // slot 3
    virtual bool assignRegistersForWorkTable(ScheduleTable *t);                     // slot 4

    char reservedHardwarePoolStorage[0x20];
};

#endif
