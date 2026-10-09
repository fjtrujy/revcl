#ifndef LOOPDEBRIS_H
#define LOOPDEBRIS_H

#pragma interface

#include "epiRec.h"

class instStream;
class workTable;

class loopDebris {
public:
    loopDebris() throw();
    ~loopDebris() throw();
    bool init(const char *baseName, int a, bool flag);
    void func_0806770c();
    instStream *getEnterStream();
    instStream *getExitStream();
    instStream *getMainStream();
    instStream *getProStream(int i);
    instStream *getEpiStream(int i);
    int getNumPro();
    int getNumEpi();
    epiRec *allocPro(int size);
    epiRec *allocEpi(int size);
    epiRec *allocMain(int size);
    epiRec *allocEnter(int size);
    epiRec *allocExit(int size);

    epiRec *getProRec(int i)
    {
        if (i < 0 || i >= m_nPro)
            return 0;
        return &m_pro[i];
    }
    epiRec *getEpiRec(int i)
    {
        if (i < 0 || i >= m_nEpi)
            return 0;
        return &m_epi[i];
    }
    epiRec *func_08067c70(int key)
    {
        for (int i = 0; i != m_nEpi; i++)
            if (m_epi[i].unk_08 == key)
                return &m_epi[i];
        return 0;
    }

    workTable *unk_00;
    char *m_entryName;
    char *m_exitName;
    const char *m_baseName;
    char unk_10;
    int unk_14;
    int m_nPro;
    int m_nEpi;
    epiRec m_enter;
    epiRec m_main;
    epiRec m_pro[20];
    epiRec m_epi[20];
    epiRec m_exit;
};

#endif
