// Used-before-set list; vtable/dtors are emitted outside infoList.cpp.
#ifndef UBSLIST_H
#define UBSLIST_H

#include "infoList.h"

class ubsList : public infoList {
public:
    bool addUBS(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits);
};

#endif
