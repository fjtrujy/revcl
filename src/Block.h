#ifndef BLOCK_H
#define BLOCK_H

class Instr;

class Block {
public:
    char unk_00[0x88];
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
