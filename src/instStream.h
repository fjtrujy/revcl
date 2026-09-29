// Instruction stream (owner of instructions); layout mostly unknown.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

class instStream {
public:
    char unk_00[0x260];
    char unk_260;      // set when stream takes part in a stall
    char unk_261[0x5f];
    short unk_2c0;     // stream id
};

#endif
