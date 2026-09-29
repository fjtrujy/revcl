#ifndef EPIREC_H
#define EPIREC_H

class instStream;
class workTable;

struct epiRec {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
    int unk_10;
    instStream *unk_14;
    workTable *unk_18;
    int unk_1c;

    epiRec()
    {
        unk_08 = -1;
        unk_0c = -1;
        unk_04 = -1;
        unk_00 = -1;
        unk_14 = 0;
        unk_1c = 0;
    }
    ~epiRec() {}
    void reset()
    {
        unk_08 = -1;
        unk_0c = -1;
        unk_04 = -1;
        unk_00 = -1;
        unk_14 = 0;
        unk_1c = 0;
    }
    bool init(int size, const char *name);
    void func_08067df0();
    const char *getName();
};

#endif
