#ifndef STATCOUNTS_H
#define STATCOUNTS_H

class Instr;

class statCounts {
public:
    int unk_00[9];

    void clear();
    bool add(Instr *in);
    int get(int i);
};

#endif
