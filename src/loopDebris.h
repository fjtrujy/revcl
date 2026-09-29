#ifndef LOOPDEBRIS_H
#define LOOPDEBRIS_H

class instStream;

struct epiRec {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
    int unk_10;
    instStream *unk_14;
    int unk_18;
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
};

class loopDebris {
public:
    loopDebris();
    ~loopDebris();

    int unk_00;
    char *m_entryName;
    char *m_exitName;
    const char *m_baseName;
    char unk_10;
    int unk_14;
    int unk_18;
    int unk_1c;
    epiRec m_enter;
    epiRec m_exit;
    epiRec unk_60[20];
    epiRec unk_2e0[20];
    epiRec unk_560;
};

#endif
