// iHolder (instruction/operand/field holder) and infoList base class, CIE group 3.
#ifndef INFOLIST_H
#define INFOLIST_H

class Instr;
class infoNode;

struct ol_s {
    char unk_00[4];
    Instr *unk_04;  // instruction
    char unk_08[8];
    char unk_10;    // operand index
    char unk_11;    // field bit number
};

class iHolder {
public:
    Instr *unk_00;  // instruction
    int unk_04;     // operand index
    int unk_08;     // field bit mask

    int func_08052380(ol_s *ol);
    void func_080523b8();
    bool catInit(ol_s *ol);
};

class infoList {
public:
    infoList();
    virtual ~infoList();

    infoNode *unk_04;  // head
    int unk_08;        // count (guess)
};

#endif
