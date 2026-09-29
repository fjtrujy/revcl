// iHolder and infoList, CIE group 3.
#include "infoList.h"
#include "Instr.h"
#include "os.h"

int iHolder::func_08052380(ol_s *ol)
{
    if (!ol || !unk_00)
        return 0;
    int r = 0;
    if (unk_00 == ol->unk_04 && unk_04 == ol->unk_10)
        r = 1;
    return r;
}

void iHolder::func_080523b8()
{
    if (unk_00)
        unk_00->leDump(unk_04, unk_08);
    else
        func_0804ac50(" UNSET_INST\n");
}

bool iHolder::catInit(ol_s *ol)
{
    if (!ol) {
        func_0804ad38("iHolder - passed a null ol\n");
        return false;
    }
    if (!unk_00) {
        if (ol->unk_04) {
            unk_00 = ol->unk_04;
            unk_04 = ol->unk_10;
            unk_08 = 0;
        } else {
            func_0804ad38("iHolder:: init called, but ol_s not inst\n");
            return false;
        }
    } else if (unk_00 != ol->unk_04 || unk_04 != ol->unk_10) {
        func_0804ad38("iHolder::catInit - mismatch adding to an existing object\n");
        return false;
    }
    unk_08 |= 1 << ol->unk_11;
    return true;
}

infoList::infoList()
{
    unk_04 = 0;
    unk_08 = 0;
}
