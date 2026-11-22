#ifndef MEMREGPOOL_H
#define MEMREGPOOL_H

#include "RegPool.h"

class InstructionStream;
class ScheduleTable;

class memRegPool : public RegPool {
public:
    memRegPool();
    virtual ~memRegPool();
    virtual bool assignExt(InstructionStream *s);                        // slot 2
    virtual bool discardDisjointExternalRegister(InstructionStream *s, RegisterEntry *reg); // slot 3
    virtual bool assignRegistersForWorkTable(ScheduleTable *t);                     // slot 4

    char reservedMemoryPoolStorage[0x20];
};

#endif
