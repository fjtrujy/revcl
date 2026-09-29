#include "iGroupLog.h"

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
