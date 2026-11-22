#ifndef MOVEREC_H
#define MOVEREC_H

#include "Instr.h"

bool detectMove(Instr *in, int &opc, unsigned char &mask, int &dstOp, int &srcOp);

class moveRec {
public:
    moveRec *nextFree;
    unsigned char moveType;
    Instr *instruction;
    RegPoolEntry *moveRegister;
    RegisterPartLockList partLocks[4];

    moveRec(char type)
    {
        moveType = type;
        instruction = 0;
        moveRegister = 0;
    }
};

#endif
