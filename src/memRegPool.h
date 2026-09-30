#ifndef MEMREGPOOL_H
#define MEMREGPOOL_H

#include "RegPool.h"

class memRegPool : public RegPool {
public:
    memRegPool();
    virtual ~memRegPool();

    char unk_30[0x20];
};

#endif
