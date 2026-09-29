#ifndef BLOCK_H
#define BLOCK_H

class Instr;

class Block {
public:
    char unk_00[4];
    short unk_04;   // current height
    short unk_06;
    char unk_08[0x30];
    int unk_38;     // range width limit
    char unk_3c[0x88 - 0x3c];
    Instr *unk_88;  // linear list tail
    Instr *unk_8c;  // linear list head
    int unk_90;     // linear list count
    Instr *unk_94;  // ToDo list head
    Instr *unk_98;  // ToDo list tail
    int unk_9c;     // ToDo list count
    Instr *unk_a0;  // Done list head
    Instr *unk_a4;  // Done list tail
    int unk_a8;     // Done list count
};

#endif
