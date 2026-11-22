// Operand location node (instruction, operand index, field bit), CIE group 5.
#ifndef OL_S_H
#define OL_S_H
#pragma interface

class Instr;

struct OperandLocation {
    OperandLocation *next;
    Instr *instruction;
    int pathKey;
    int maximumPathKey;
    union {
        struct {
            char operandIndex;
            char componentIndex;
        };
        short packedOperandComponent;
    };
    unsigned char cycleOffset;
    union {
        unsigned char stateFlags;
        struct {
            unsigned char external : 1;
            unsigned char externalUsed : 1;
        } flags;
    };

    inline OperandLocation();
    inline ~OperandLocation();
    inline void reset();
    inline bool isExt();
    inline bool isExtUsed();
    void dumpInstructionContext();
    void dumpOperandContext();
    OperandLocation *nextInternalLocation();
    OperandLocation *nextExternalLocation();
    void copyLocationFrom(OperandLocation *src);
    void init(Instr *inst, int op, int slot, bool ext);
};

#endif
