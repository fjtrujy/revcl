// Processor description (CIE group 18).
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "mList.h"
#include "ubsList.h"

class stallList : public infoList {
};

class iInfo;
class RegPool;

extern int g_080b33a8;
extern iInfo *g_080de6c0;
extern RegPool *g_080de6e0[12];
extern int g_080de714[3];

class Processor {
public:
    Processor();
    ~Processor();

    char unk_00;
    mList unk_04;
    const char *unk_18;
    const char *unk_1c;    // name, default "Nameless"
    mList unk_20;
    int unk_34;
    int unk_38;
    char unk_3c;
    char unk_3d;
    char unk_3e;
    int unk_40;
    int unk_44;
    int unk_48;
    mList unk_4c;
    char unk_60;
    char unk_61[3];
    char unk_64;
    char unk_65;
    ubsList unk_68;
    stallList unk_74;
};

#endif
