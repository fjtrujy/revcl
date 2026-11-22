#ifndef EPIREC_H
#define EPIREC_H

class InstructionStream;
class ScheduleTable;
class Instr;

struct epiRec {
    int regionEndIndex;
    int regionStartIndex;
    int loopKey;
    int sourceLoopId;
    int reservedLoopRegionStateWord;
    InstructionStream *stream;
    ScheduleTable *table;
    Instr *conditionalBranch;

    epiRec()
    {
        loopKey = -1;
        sourceLoopId = -1;
        regionStartIndex = -1;
        regionEndIndex = -1;
        stream = 0;
        conditionalBranch = 0;
    }
    ~epiRec() {}
    void reset()
    {
        loopKey = -1;
        sourceLoopId = -1;
        regionStartIndex = -1;
        regionEndIndex = -1;
        stream = 0;
        conditionalBranch = 0;
    }
    bool init(int size, const char *name);
    void noopLoopRegionHook();
    const char *getName();
};

#endif
