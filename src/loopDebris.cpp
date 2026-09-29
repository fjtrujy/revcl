#include "loopDebris.h"

loopDebris::loopDebris()
{
    m_entryName = 0;
    m_exitName = 0;
}

loopDebris::~loopDebris()
{
    if (m_entryName)
        delete [] m_entryName;
    if (m_exitName)
        delete [] m_exitName;
    m_entryName = 0;
    m_exitName = 0;
}
