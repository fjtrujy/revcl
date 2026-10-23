#ifndef MOVEREC_H
#define MOVEREC_H

#include "Instr.h"

bool detectMove(Instr *in, int &opc, unsigned char &mask, int &dstOp, int &srcOp);

class moveRec {
public:
    moveRec *unk_00;          // next free
    unsigned char unk_04;     // move type
    Instr *unk_08;            // inst
    RegPoolEntry *unk_0c;     // m_Reg
    cls0805c244 unk_10[4];    // rll

    moveRec(char type)
    {
        unk_04 = type;
        unk_08 = 0;
        unk_0c = 0;
    }
};

#endif
