#ifndef INSTR_H
#define INSTR_H

class Instr {
public:
    char unk_00[0x1c];
    Instr *unk_1c;  // linear list link
    Instr *unk_20;  // linear list link
    Instr *unk_24;  // ToDo/Done list next
    Instr *unk_28;  // ToDo/Done list prev
};

#endif
