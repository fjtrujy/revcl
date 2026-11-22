#ifndef IINFO_SHARED_H
#define IINFO_SHARED_H

#include "Instr.h"
#include "Block.h"
#include "os.h"

Instr *getMove(int type, int);
int getMove(Block *block, Instr *in, int now);

#endif
