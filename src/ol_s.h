// Operand location node (instruction, operand index, field bit), CIE group 5.
#ifndef OL_S_H
#define OL_S_H
#pragma interface

#include "Instr.h"

struct ol_s {
    ol_s *unk_00;   // next
    Instr *unk_04;  // instruction
    int unk_08;
    int unk_0c;
    union {
        struct {
            char unk_10;    // operand index
            char unk_11;    // field bit number
        };
        short unk_10_11;    // operand index and bit compared together
    };
    unsigned char unk_12;
    unsigned char unk_13;   // flags, bit0 = external

    inline ol_s();
    inline void reset();
    inline bool isExt();
    void func_08057fbc();
    void func_08057fec();
    ol_s *func_080583ac();
    ol_s *func_08058414();
    void func_08058054(ol_s *src)
    {
        reset();
        if (src) {
            unk_13 = src->unk_13;
            unk_04 = src->unk_04;
            unk_10 = src->unk_10;
            unk_11 = src->unk_11;
            unk_12 = src->unk_12;
        }
    }
    void init(Instr *inst, int op, int slot, bool ext)
    {
        reset();
        if (ext)
            unk_13 = 1;
        unk_04 = inst;
        unk_10 = op;
        unk_11 = slot;
    }
};

#endif
