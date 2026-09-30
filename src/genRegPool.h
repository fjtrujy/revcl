#ifndef GENREGPOOL_H
#define GENREGPOOL_H

#include "RegPool.h"

class genRegPool : public RegPool {
public:
    genRegPool();
    virtual ~genRegPool();

    char unk_30[0x20];
};

#endif
