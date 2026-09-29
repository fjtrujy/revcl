#ifndef INSTR_H
#define INSTR_H

class instStream;

class Instr {
public:
    char unk_00[0x1c];
    Instr *unk_1c;  // linear list link
    Instr *unk_20;  // linear list link
    Instr *unk_24;  // ToDo/Done list next
    Instr *unk_28;  // ToDo/Done list prev

    instStream *func_08054168();  // owner instruction stream
    void leDump(int opnd, int mask);
};

#endif
