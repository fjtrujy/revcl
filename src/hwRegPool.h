#ifndef HWREGPOOL_H
#define HWREGPOOL_H

#include "RegPool.h"

class hwRegPool : public RegPool {
public:
    hwRegPool();
    virtual ~hwRegPool();

    char unk_30[0x20];
};

#endif
