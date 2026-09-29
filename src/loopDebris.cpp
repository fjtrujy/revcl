#include "loopDebris.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"

char *dupString(const char *s);

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

bool loopDebris::init(const char *baseName, int a, char flag)
{
    char buf[4096];
    if (!baseName) {
        func_0804ad38("loopDebris:: init - no base name set!\n");
        return false;
    }
    unk_10 = flag;
    m_entryName = 0;
    m_exitName = 0;
    unk_00 = 0;
    m_nPro = 0;
    m_nEpi = 0;
    unk_14 = a;
    m_baseName = baseName;
    func_0804adbc(buf, "%s_ENTRY_POINT", baseName);
    m_entryName = dupString(buf);
    func_0804adbc(buf, "%s_EXIT_POINT", baseName);
    m_exitName = dupString(buf);
    return true;
}

void loopDebris::func_0806770c()
{
    int i;
    if (!m_entryName) {
        func_0804ad38("LoopDebris - no entry name\n");
        func_0804adf0(-1);
    }
    for (i = 0; i != m_nPro; i++)
        ;
    for (i = 0; i != m_nEpi; i++)
        ;
}

instStream *loopDebris::getEnterStream()
{
    epiRec *r = &m_enter;
    if (!r) {
        func_0804ad38("failed to get enter epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getExitStream()
{
    epiRec *r = &m_exit;
    if (!r) {
        func_0804ad38("failed to get exit epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getMainStream()
{
    epiRec *r = &m_main;
    if (!r) {
        func_0804ad38("failed to get main epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getProStream(int i)
{
    epiRec *r = getProRec(i);
    if (!r) {
        func_0804ad38("failed to get pro epirec\n");
        return 0;
    }
    return r->unk_14;
}

instStream *loopDebris::getEpiStream(int i)
{
    epiRec *r = getEpiRec(i);
    if (!r) {
        func_0804ad38("failed to get pro epirec\n");
        return 0;
    }
    return r->unk_14;
}

epiRec *loopDebris::allocPro(int size)
{
    char buf[4096];
    epiRec *r = 0;
    if (m_nPro < 16) {
        const char *name = "NOTSET";
        if (m_baseName)
            name = m_baseName;
        func_0804adbc(buf, "%s_PRO%d", name, m_nPro);
        r = &m_pro[m_nPro];
        r->reset();
        if (!r->init(size, buf)) {
            func_0804ad38("failed to init for pro\n");
            return 0;
        }
        r->unk_14->unk_2c4 = 2;
        if (unk_10) {
            r->unk_18->func_08073774(2);
            r->unk_18->unk_08 = unk_14;
        } else {
            r->unk_18->func_08073774(1);
            r->unk_18->unk_08 = 2;
        }
        m_nPro++;
    } else
        func_0804ad38("allocPro: overrun\n");
    return r;
}
