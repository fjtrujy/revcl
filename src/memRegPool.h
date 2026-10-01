#ifndef MEMREGPOOL_H
#define MEMREGPOOL_H

#include "RegPool.h"

class instStream;
class workTable;

class memRegPool : public RegPool {
public:
    memRegPool();
    virtual ~memRegPool();
    virtual bool func_08089104(instStream *s);                    // slot 2
    virtual bool func_08089110(instStream *s, RegPoolEntry *reg); // slot 3
    virtual bool func_080890f8(workTable *t);                     // slot 4

    char unk_30[0x20];
};

#endif
