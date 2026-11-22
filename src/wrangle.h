#ifndef WRANGLE_H
#define WRANGLE_H

class Instr;

struct wrangleCand {
    Instr *candidateInstruction;
    short candidateRow;
    char candidatePipe;
};

#endif
