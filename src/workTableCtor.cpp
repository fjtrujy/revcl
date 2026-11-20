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

