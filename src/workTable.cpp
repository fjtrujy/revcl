#include "RegPool.h"
#include "os.h"
#include "workTable.h"

RegPool *GetRP(unsigned t);

void regUseList::func_0805b44c()
{
    if (func_0805b0cc()) {
        for (int i = 0; i != 12; i++) {
            RegPool *rp = GetRP(i);
            if (rp)
                func_0805b494(i);
        }
    }
}

int regUseList::func_0805b494(int regClass)
{
    int n = 0;
    regUse *head = unk_04;
    regUse *ru;
    for (ru = head; ru; ru = ru->unk_04) {
        RegPoolEntry *reg = ru->unk_08;
        if (!reg) {
            func_0804ad38("found reg ref with no reg!\n");
            func_0804adf0(-1);
            return 0;
        }
        if (reg->unk_24 == regClass)
            n++;
    }
    if (n <= 0)
        return 0;
    for (ru = head; ru; ru = ru->unk_04) {
        if (ru->unk_08->unk_24 == regClass)
            ru->func_0805b51c();
    }
    return n;
}
