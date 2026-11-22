// Per-slot register use data, embedded in regUse (names guessed).
#ifndef REGUSESLOT_H
#define REGUSESLOT_H

#include "vcl_portability.h"

#include "Instr.h"
#include "tPairList.h"

struct regUseSlot {
    olList inputLocations;
    olList outputLocations;
    tPairList throughputPairs;
    olList externalInputLocations;
    olList sourceLocations;
    olList externalSourceLocations;
    short earliestExternalCycle;
    short latestExternalCycle;
    short throughputSums[4];
    short timingValues[13];
    VclPtrWord assignmentPayload;
    cls0805bcc4 registerLock;

    regUseSlot();
#ifdef VCL_REGUSESLOT_IMPL
    ~regUseSlot();
#else
    ~regUseSlot() {}
#endif
    bool optReset();
    bool addOutput(ol_s *ol);
    void validateOutputLocations();
    void validateInputLocations();
    cls0805bcc4 *getRegisterLock();
    ol_s *findPreviousOutputAtPosition(int pos, bool all);
};

#endif
