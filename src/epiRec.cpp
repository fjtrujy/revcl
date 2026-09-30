#include "epiRec.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"

extern short g_streamSerial;

bool epiRec::init(int size, const char *name)
{
    reset();
    unk_14 = new instStream;
    if (!unk_14) {
        func_0804ad38("setup: failed to get iStream\n");
        func_0804adf0(-1);
    }
    mNode *n = unk_14->unk_110.func_0804b2d4(name);
    if (!n) {
        func_0804ad38("failed to add a name to iStream\n");
        func_0804adf0(-1);
    }
    workTable *wt = &unk_14->unk_148;
    if (!wt->init(3, size)) {
        func_0804ad38("failed to init it_pro for size %d\n", size);
        func_0804adf0(-1);
    }
    instStream *s = unk_14;
    int serial = g_streamSerial;
    unk_18 = wt;
    s->unk_2c2 = serial;
    wt->unk_10 = unk_14;
    unk_14->unk_218 = &unk_14->unk_148;
    return true;
}

void epiRec::func_08067df0()
{
}

const char *epiRec::getName()
{
    if (!unk_14)
        return "!UNNAMED!";
    return unk_14->func_0806d674();
}
