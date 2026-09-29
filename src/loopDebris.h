#ifndef LOOPDEBRIS_H
#define LOOPDEBRIS_H

class instStream;
class workTable;

struct epiRec {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
    int unk_10;
    instStream *unk_14;
    workTable *unk_18;
    int unk_1c;

    epiRec()
    {
        unk_08 = -1;
        unk_0c = -1;
        unk_04 = -1;
        unk_00 = -1;
        unk_14 = 0;
        unk_1c = 0;
    }
    ~epiRec() {}
    void reset()
    {
        unk_08 = -1;
        unk_0c = -1;
        unk_04 = -1;
        unk_00 = -1;
        unk_14 = 0;
        unk_1c = 0;
    }
    bool init(int size, const char *name);
};

class loopDebris {
public:
    loopDebris();
    ~loopDebris();
    bool init(const char *baseName, int a, char flag);
    void func_0806770c();
    instStream *getEnterStream();
    instStream *getExitStream();
    instStream *getMainStream();
    instStream *getProStream(int i);
    instStream *getEpiStream(int i);
    epiRec *allocPro(int size);

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

    int unk_00;
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
