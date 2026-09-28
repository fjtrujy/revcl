// Instruction list helpers: ToDo / Done / Linear lists (CIE group 1, after mList.cpp).

#include "Block.h"
#include "Instr.h"
#include "os.h"

bool UnlinkFromToDo(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromToDo - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_9c) {
        func_0804ad38("UnlinkFromToDo asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_24)
        n->unk_24->unk_28 = n->unk_28;
    else if (n == blk->unk_98)
        blk->unk_98 = n->unk_28;
    else {
        func_0804ad38("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    if (n->unk_28)
        n->unk_28->unk_24 = n->unk_24;
    else if (n == blk->unk_94)
        blk->unk_94 = n->unk_24;
    else {
        func_0804ad38("UnlinkFromToDono next.. but not tail!\n");
        return false;
    }
    blk->unk_9c--;
    n->unk_28 = 0;
    n->unk_24 = 0;
    return true;
}

bool UnlinkFromDone(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromDone - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_a8) {
        func_0804ad38("UnlinkFromDone asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_24)
        n->unk_24->unk_28 = n->unk_28;
    else if (n == blk->unk_a4)
        blk->unk_a4 = n->unk_28;
    else {
        func_0804ad38("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    if (n->unk_28)
        n->unk_28->unk_24 = n->unk_24;
    else if (n == blk->unk_a0)
        blk->unk_a0 = n->unk_24;
    else {
        func_0804ad38("UnlinkFromDoneno next.. but not tail!\n");
        return false;
    }
    blk->unk_a8--;
    n->unk_28 = 0;
    n->unk_24 = 0;
    return true;
}

bool UnlinkFromLinear(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("UnlinkFromLinear - passed a null pointer\n");
        return false;
    }
    if (!blk->unk_90) {
        func_0804ad38("UnlinkFromLinear asked to unlink - but zero count\n");
        return false;
    }
    if (n->unk_1c)
        n->unk_1c->unk_20 = n->unk_20;
    else if (n == blk->unk_8c)
        blk->unk_8c = n->unk_20;
    else {
        func_0804ad38("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    if (n->unk_20)
        n->unk_20->unk_1c = n->unk_1c;
    else if (n == blk->unk_88)
        blk->unk_88 = n->unk_1c;
    else {
        func_0804ad38("UnlinkFromLinearno next.. but not tail!\n");
        return false;
    }
    blk->unk_90--;
    n->unk_20 = 0;
    n->unk_1c = 0;
    return true;
}

bool AddToDo_Head(Block *blk, Instr *n)
{
    if (!n) {
        func_0804ad38("AddToDo_Head - passed a null pointer\n");
        return false;
    }
    if (n->unk_28 || n->unk_24) {
        func_0804ad38("AddToDo_Head - node already appears linked\n");
        return false;
    }
    if (blk->unk_94) {
        if (!blk->unk_98) {
            func_0804ad38("AddToDo_Headlist had no first, but last set!\n");
            func_0804adf0(-1);
            return false;
        }
        if (!blk->unk_9c) {
            func_0804ad38("AddToDo_Headlist had no head or tail, but count not zero\n");
            func_0804adf0(-1);
            return false;
        }
        n->unk_24 = blk->unk_94;
        blk->unk_94->unk_28 = n;
        blk->unk_94 = n;
    } else {
        if (blk->unk_9c) {
            func_0804ad38("AddToDo_Headno list head but count not zero\n");
            return false;
        }
        blk->unk_98 = n;
        blk->unk_94 = n;
    }
    blk->unk_9c++;
    return true;
}
