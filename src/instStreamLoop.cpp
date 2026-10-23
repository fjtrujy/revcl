#include "instStream.h"

bool instStream::canLoop()
{
    int type;
    if (unk_2ce != 1 && (type = unk_2ce - 5))
        return false;
    instStream *s = unk_134.getStream(1);
    if (!s) {
        func_0804ad38("canLoop - bad return from outIST getFirst\n");
        func_0804adf0(-1);
    }
    return s == this;
}

unsigned char instStream::func_0806d908()
{
    workTable *wt = unk_218;
    if (wt)
        return wt->unk_09;
    func_0804ad38("getCodeMode - work table not set\n");
    return 0;
}

workTable::workTable()
{
    unk_2c = 0;
    unk_4c = 0;
    unk_24 = 0;
    unk_20 = 0;
    unk_0b = 0;
    unk_10 = 0;
    dispose();
}

