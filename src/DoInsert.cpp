#define WORK_TABLE_CTOR_EMISSION __attribute__((weak,section(".gnu.linkonce.t._ZN9workTableC2Ev")))
#include "os.h"
#include "workTable.h"
#include "Instr.h"

#undef WORK_TABLE_CTOR_EMISSION

class Block;
bool setLocks(Instr *in);
bool UnlinkFromToDo(Block *blk, Instr *in);
bool AddDone_Tail(Block *blk, Instr *in);

extern const char doInsertLocksError[];
extern const char doInsertUnlinkError[];

bool DoInsert(workTable *const wt, Instr *in, cls08058718 *pos)
{
    short b = in->unk_142c;
    if (__builtin_expect(b != 0, 1) && !(b = in->unk_142e)) {
        func_0804ad38("undo insert currmuid not set\n");
        return false;
    }
    short r = pos->unk_04;
    wtRow *row = &wt->unk_2c[r];
    cls08058718 *s = &in->unk_13e8;
    b = pos->unk_00;
    s->unk_04 = r;
    char a = pos->unk_03;
    s->unk_03 = a;
    char p = pos->getPipe();
    s->unk_00 = b;
    s->unk_02 = p;
    row->unk_00[p].unk_00 = in;
    b = setLocks(in);
    if (!(char)b) {
        func_0804ad38(doInsertLocksError);
        return false;
    }
    in->unk_1396 = wt->unk_40;
    wt->unk_40++;
    b = UnlinkFromToDo((Block *)wt, in);
    if (!(char)b) {
        func_0804ad38(doInsertUnlinkError);
        func_0804aeb0();
    }
    a = AddDone_Tail((Block *)wt, in);
    if (!a) {
        func_0804ad38("failed to add to done\n");
        func_0804aeb0();
    }
    return true;
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

