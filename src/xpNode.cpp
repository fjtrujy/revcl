#include "xpNode.h"

xpNode::xpNode()
{
    init();
}

xpNode::~xpNode()
{
    val.clear();
}

bool xpNode::func_0804e778()
{
    clearValue();
    return true;
}

void xpNode::func_0804e794()
{
    val.dump();
}

void xpNode::reset()
{
    init();
}
