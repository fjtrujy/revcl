// Operand, CIE group 4. Separate TU from Instr.cpp: its .rodata (strings from
// 0x08090848, jump tables from 0x080915e4) follows Instr's jump tables.
#include "Instr.h"
#include "infoList.h"
#include "os.h"
#include "VUStr.h"

int countSetBits(unsigned int);
bool asciiCaseEqual(const char *a, const char *b);
void removeTrailingComponentSuffix(char *s);
int mstrlen(const char *s);


bool olList::clear()
{
    ol_s *p = head;
    if (p) {
        ol_s *f = olFreeList;
        do {
            ol_s *n = p->next;
            p->next = f;
            f = p;
            p = n;
        } while (p);
        olFreeList = f;
    }
    head = 0;
    count = 0;
    return true;
}
