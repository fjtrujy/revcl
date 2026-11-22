#ifndef GENREGPOOL_H
#define GENREGPOOL_H

#include "RegPool.h"

struct RegisterUse;

class genRegPool : public RegPool {
public:
    genRegPool();
    virtual ~genRegPool();
    virtual bool assignExt(InstructionStream *s);                        // slot 2
    virtual bool discardDisjointExternalRegister(InstructionStream *s, RegisterEntry *reg); // slot 3
    virtual bool assignRegistersForWorkTable(ScheduleTable *t);                     // slot 4

    void clearRegisterAllocationMask();
    bool buildCandidateRegisterTimeline(RegisterEntry *registerEntry, unsigned char mask);
    bool initializeAllocationTimeline(int start, int end);
    void dumpCandidateRegisterTimeline();
    int countCandidateTimelineRows();
    int calculateAllocationFitCost(int reg);
    bool mergeCandidateIntoAllocationTimeline(int reg);
    void dumpRegisterAllocationTimeline();
    int chooseLowestCostRegister();
    bool allocateRegisterUseInTimeline(RegisterUse *ru);

    char registerPartAvailabilityMask[0x20];
};

#endif
