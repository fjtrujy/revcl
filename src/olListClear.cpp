// Operand, CIE group 4. Separate TU from Instr.cpp: its .rodata (strings from
// 0x08090848, jump tables from 0x080915e4) follows Instr's jump tables.
#include "Instr.h"
#include "infoList.h"
#include "os.h"
#include "VUStr.h"

int func_0804bf50(unsigned int);
bool func_0804bfcc(const char *a, const char *b);
void func_0804c0c8(char *s);
int mstrlen(const char *s);


bool olList::clear()
{
    ol_s *p = unk_00;
    if (p) {
        ol_s *f = olFreeList;
        do {
            ol_s *n = p->unk_00;
            p->unk_00 = f;
            f = p;
            p = n;
        } while (p);
        olFreeList = f;
    }
    unk_00 = 0;
    unk_04 = 0;
    return true;
}
