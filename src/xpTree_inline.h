// Inline xpTree members, kept out of the #pragma interface header so that
// xpNode.cpp (#pragma implementation) does not emit them out of line.
#ifndef XPTREE_INLINE_H
#define XPTREE_INLINE_H

#include "xpNode.h"

inline ExpressionTree::~ExpressionTree()
{
    reset();
}

#endif
