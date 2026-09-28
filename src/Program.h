#ifndef PROGRAM_H
#define PROGRAM_H

// Large object living on main's stack (>= 0x101a8 bytes). Name is a guess.
struct Program {
    char unk_00[0x70];
    int unk_70;
    char unk_74[0x08];
    int unk_7c;
};

#endif
