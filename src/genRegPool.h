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
    bool buildCandidateRegisterTimeline(RegPoolEntry *registerEntry, unsigned char mask);
    bool initializeAllocationTimeline(int start, int end);
    void dumpCandidateRegisterTimeline();
    int countCandidateTimelineRows();
    int calculateAllocationFitCost(int reg);
    bool mergeCandidateIntoAllocationTimeline(int reg);
    void dumpRegisterAllocationTimeline();
    int chooseLowestCostRegister();
    bool allocateRegisterUseInTimeline(regUse *ru);

    char registerPartAvailabilityMask[0x20];
};

#endif
