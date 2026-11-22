#ifndef GENREGPOOL_H
#define GENREGPOOL_H

#include "RegPool.h"

struct regUse;

class genRegPool : public RegPool {
public:
    genRegPool();
    virtual ~genRegPool();
    virtual bool assignExt(instStream *s);                        // slot 2
    virtual bool discardDisjointExternalRegister(instStream *s, RegPoolEntry *reg); // slot 3
    virtual bool assignRegistersForWorkTable(workTable *t);                     // slot 4

    void clearRegisterAllocationMask();
    bool build_tTab(RegPoolEntry *reg, unsigned char mask);
    bool initAlloc(int start, int end);
    void dumpTTab();
    int countTTab();
    int fitCost(int reg);
    bool set_t_in_aTab(int reg);
    void dumpATab();
    int bestReg();
    bool set_in_aTab(regUse *ru);

    char registerPartAvailabilityMask[0x20];
};

#endif
