#ifndef HWREGPOOL_H
#define HWREGPOOL_H

#include "RegPool.h"

class instStream;
class workTable;

class hwRegPool : public RegPool {
public:
    hwRegPool();
    virtual ~hwRegPool();
    virtual bool func_08088e60(instStream *s);                    // slot 2
    virtual bool func_08088e6c(instStream *s, RegPoolEntry *reg); // slot 3
    virtual bool func_08088e54(workTable *t);                     // slot 4

    char unk_30[0x20];
};

#endif
