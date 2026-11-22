#ifndef STATCOUNTS_H
#define STATCOUNTS_H

class Instr;

class statCounts {
public:
    int resourceInstructionCounts[9];

    void clear();
    bool add(Instr *in);
    int get(int i);
};

#endif
