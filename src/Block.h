#ifndef BLOCK_H
#define BLOCK_H

#include "workTable.h"

// Preserve the historic Block* function signatures, but use workTable as the
// single source of truth for its actual fields and ABI layout. No Block
// instances are created; legacy functions operate on workTable objects.
class Block : public workTable {};

#endif
