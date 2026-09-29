#include <stdlib.h>
#include "iGroupLog.h"

int func_0804ad38(const char *fmt, ...);

Instr **iGroupLog::table;
int iGroupLog::count;
int iGroupLog::size;

iGroupLog::iGroupLog()
{
    table = 0;
    count = 0;
    size = 0;
}

iGroupLog::~iGroupLog()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
}

int iGroupLog::init()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
    return 1;
}

int iGroupLog::reset()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
    return 1;
}

int iGroupLog::add(Instr *ins)
{
    if (!ins) {
        func_0804ad38("iGroup - add instruction - no instruction to add!\n");
        return 0;
    }
    if (count > size) {
        func_0804ad38("overflow for the iGroup log\n");
        return 0;
    }
    if (count == size) {
        size = count * 2 + 0x2000;
        int bytes = size * sizeof(Instr *);
        table = (Instr **)realloc(table, bytes);
        if (!table) {
            func_0804ad38("iGroup - failed to realloc table to %d\n", bytes);
            return 0;
        }
    }
    table[count] = ins;
    return count++;
}
