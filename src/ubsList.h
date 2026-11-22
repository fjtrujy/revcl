// Used-before-set list; vtable/dtors are emitted outside infoList.cpp.
#ifndef UBSLIST_H
#define UBSLIST_H

#include "infoList.h"

class stallList : public infoList {
};

class ubsList : public infoList {
public:
    bool addUBS(InstructionStream *entry, InstructionStream *use, RegisterEntry *registerEntry, unsigned char bits);
};

#endif
