#ifndef MEMREGPOOL_H
#define MEMREGPOOL_H

#include "RegPool.h"

class instStream;
class workTable;

class memRegPool : public RegPool {
public:
    memRegPool();
    virtual ~memRegPool();
    virtual bool assignExt(instStream *s);                        // slot 2
    virtual bool discardDisjointExternalRegister(instStream *s, RegPoolEntry *reg); // slot 3
    virtual bool assignRegistersForWorkTable(workTable *t);                     // slot 4

    char unk_30[0x20];
};

#endif
