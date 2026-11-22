#ifndef LOOPDEBRIS_H
#define LOOPDEBRIS_H

#include "epiRec.h"

class InstructionStream;
class ScheduleTable;

class loopDebris {
public:
    loopDebris() throw();
    ~loopDebris() throw();
    bool init(const char *baseName, int a, bool flag);
    void validateLoopDebrisName();
    InstructionStream *getEnterStream();
    InstructionStream *getExitStream();
    InstructionStream *getMainStream();
    InstructionStream *getProStream(int i);
    InstructionStream *getEpiStream(int i);
    int getPrologueRegionCount();
    int getEpilogueRegionCount();
    epiRec *allocPro(int size);
    epiRec *allocEpi(int size);
    epiRec *allocMain(int size);
    epiRec *allocEnter(int size);
    epiRec *allocExit(int size);

    epiRec *getProRec(int i);
    epiRec *getEpiRec(int i);
    epiRec *findEpilogueByLoopKey(int key);

    ScheduleTable *sourceWorkTable;
    char *m_entryName;
    char *m_exitName;
    const char *m_baseName;
    char preserveAllocationMode;
    int requestedAllocationMode;
    int m_nPro;
    int m_nEpi;
    epiRec m_enter;
    epiRec m_main;
    epiRec m_pro[20];
    epiRec m_epi[20];
    epiRec m_exit;
};

#endif
