// Operand location node (instruction, operand index, field bit), CIE group 5.
#ifndef OL_S_H
#define OL_S_H
#pragma interface

class Instr;

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
    union {
        unsigned char unk_13;   // flags, bit0 = external
        struct {
            unsigned char external : 1;
            unsigned char externalUsed : 1;
        } flags;
    };

    inline ol_s();
    inline ~ol_s();
    inline void reset();
    inline bool isExt();
    inline bool isExtUsed();
    void func_08057fbc();
    void func_08057fec();
    ol_s *func_080583ac();
    ol_s *func_08058414();
    void func_08058054(ol_s *src);
    inline void init(Instr *inst, int op, int slot, bool ext);
};

#endif
