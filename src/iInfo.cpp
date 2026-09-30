#include "Instr.h"
#include "Block.h"
#include "os.h"

unsigned char iInfo::func_0806ffb4()
{
    if (kind() == 3)
        return unk_19;
    return 0;
}

unsigned char iInfo::ramType()
{
    if (kind() == 4) {
        unsigned char t = unk_19;
        if ((unsigned char)(t - 1) <= 1)
            return t;
        func_0804ad38("loadstore instruction, but bad ramtype\n");
        func_0804adf0(-1);
    }
    return 0;
}

unsigned char iInfo::func_08070020()
{
    if (kind() == 2)
        return unk_19;
    return 0;
}

bool iInfo::func_08070044()
{
    return kind() == 0;
}

char iInfo::func_0807005c()
{
    return kind() == 0 && unk_19 == 0;
}

void iInfo::func_08070080()
{
    int i;
    for (i = 0; i != unk_44[i].unk_00; i++)
        ;
}

void iInfo::func_080700a4()
{
}

bool iInfo::check()
{
    bool noImplicitYet = true;
    iOpDesc *d = unk_44;
    for (int i = 0; d->unk_00; i++, d++) {
        if (d->unk_06) {
            if (!d->unk_02) {
                func_0804ad38("implicit web patch set on explicit operand\n");
                return false;
            }
            if (d->unk_01 != 1) {
                func_0804ad38("iInfo: check - patch set on non-input (%d)\n", d->unk_00);
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_01 != 2) {
                func_0804ad38("iInfo: check patch not point to output\n");
                return false;
            }
            if (unk_44[d->unk_06 - 1].unk_02) {
                func_0804ad38("implicit web patch links to implicit operand\n");
                return false;
            }
            if (d->unk_06 - 1 > i) {
                func_0804ad38("patched operand is after patch\n");
                return false;
            }
        }
        if (d->unk_02)
            noImplicitYet = false;
        else if (!noImplicitYet) {
            func_0804ad38("explicit operand after an implicit\n");
            return false;
        }
    }
    return true;
}

bool checkTable(iInfo *tab, int count)
{
    if (!tab) {
        func_0804ad38("checkTable - passed a null table\n");
        return false;
    }
    int i;
    for (i = 0; tab->unk_04; i++, tab++) {
        if (!tab->check()) {
            func_0804ad38("checkTable failed\n");
            return false;
        }
    }
    if (i != count) {
        func_0804ad38("instruction count mismatch %d!=%d\n", i, count);
        return false;
    }
    return true;
}

int iInfo::func_080701f4()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 1) & 1;
}

int iInfo::func_0807021c()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 3) & 1;
}

int iInfo::func_08070244()
{
    unsigned int v = kind() == 3 ? unk_19 : 0;
    return (v >> 2) & 1;
}

char iInfo::func_0807026c()
{
    char r = 0;
    if (!(unk_18 & 0xf8)) {
        int k2 = kind() == 2 ? unk_19 : 0;
        if (!k2) {
            int rt;
            if (kind() == 4) {
                unsigned char t = unk_19;
                if ((unsigned char)(t - 1) <= 1)
                    rt = t;
                else {
                    func_0804ad38("loadstore instruction, but bad ramtype\n");
                    func_0804adf0(-1);
                    rt = 0;
                }
            } else
                rt = 0;
            if (rt != 2) {
                int k3 = kind() == 3 ? unk_19 : 0;
                if (!k3)
                    r = 1;
            }
        }
    }
    return r;
}

int func_08070300(unsigned char x)
{
    int r = 0;
    if (x) {
        unsigned int f = x & 0x60;
        r = (f >> 5) + 1;
    }
    return r;
}

int func_0807031c(unsigned char)
{
    return 0;
}

unsigned char func_08070324(unsigned char x)
{
    return x >> 7;
}

Instr *getMove(int type, int);

Instr *makeMove(Block *blk, RegPoolEntry *reg)
{
    int type = 0;
    Instr *mv = 0;
    if (blk->unk_b4 + blk->unk_b8 + blk->unk_b0 >= 2 * blk->unk_38)
        return 0;
    {
        switch (reg->unk_24) {
        case 8:
            type = 1;
            if (blk->unk_b8 >= blk->unk_38) {
                blk->unk_ad = 1;
                return mv;
            }
            break;
        case 9:
            type = 2;
            break;
        default:
            func_0804ad38("can't make move for reg %s\n", reg->func_0805a8e0());
            func_0804aeb0();
        }
        mv = getMove(type, 0);
        if (mv == 0) {
            func_0804ad38("failed to get a free move\n");
            func_0804aeb0();
        }
        Operand *in = mv->getOp(1);
        if (!in) {
            func_0804ad38("failed to get move input!\n");
            func_0804adf0(1);
        }
        Operand *out = mv->getOp(0);
        if (!out) {
            func_0804ad38("failed to get move output!\n");
            func_0804adf0(1);
        }
        RegPoolEntry *dup = out->unk_18;
        if (!dup) {
            func_0804ad38("failed to dup  a reg\n");
            func_0804aeb0();
        }
        mv->unk_141a = blk->unk_38;
        in->unk_18 = reg;
        in->unk_2b = 1;
        in->unk_10 = 0;
        out->unk_2b = 1;
        out->unk_10 = 0;
        mv->unk_13d1 = 1;
        mv->unk_138e = 0;
    }
    return mv;
}

int unresolveMove(Block *blk, Instr *mv);
bool UnlinkFromToDo(Block *blk, Instr *n);
void func_08073bb0(Block *blk, Instr *mv);
void adjustMoveCount(Block *blk, Instr *mv, int);
bool addFreeMove(Instr *mv);

bool undoMove(Block *blk, Instr *mv)
{
    if (mv->unk_142e)
        unresolveMove(blk, mv);
    Operand *out = mv->getOp(0);
    Operand *in = mv->getOp(1);
    RegPoolEntry *orig = in->unk_18;
    RegPoolEntry *dup = out->unk_18;
    if (!UnlinkFromToDo(blk, mv))
        func_0804ad38("failed to unlink from todolist!\n");
    for (Instr *i = blk->unk_94; i; i = (Instr *)i->unk_24.unk_00) {
        for (int n = 0; n != i->unk_138b; n++) {
            Operand *op = &i->unk_38[n];
            RegPoolEntry *r = op->unk_18;
            if (r == dup) {
                op->unk_18 = orig;
                if (op->isInReg()) {
                    for (int p = 0; p != r->unk_28; p++) {
                        if ((bool)((op->unk_07 >> p) & 1)) {
                            cls0805bcc4 *lk = &op->unk_2c[p];
                            cls0805bcc4 *parent = lk->getParent();
                            cls0805c244 *newList = (cls0805c244 *)orig->func_0805a908(p);
                            cls0805c244 *oldList = (cls0805c244 *)dup->func_0805a908(p);
                            if ((bool)((out->unk_07 >> p) & 1) && parent == &out->unk_2c[p])
                                parent = in->unk_2c[p].getParent();
                            if (!oldList->removeInput(lk)) {
                                func_0804ad38("failed to deregister input lock for undo move\n");
                                return true;
                            }
                            if (!newList->addInput(parent, lk, op->unk_29, op->unk_0b)) {
                                func_0804ad38("undo move - failed to register input\n");
                                return true;
                            }
                        }
                    }
                }
                if (op->isOutReg()) {
                    for (int p = 0; p != r->unk_28; p++) {
                        if ((bool)((op->unk_07 >> p) & 1)) {
                            cls0805c244 *newList = (cls0805c244 *)orig->func_0805a908(p);
                            cls0805c244 *oldList = (cls0805c244 *)dup->func_0805a908(p);
                            if (!oldList->removeOutput(&op->unk_2c[p])) {
                                func_0804ad38("failed to deregister output lock for undo move\n");
                                return true;
                            }
                            if (!newList->addOutput(&op->unk_2c[p], i->unk_13cf, op->unk_0a, op->unk_0b)) {
                                func_0804ad38("undo move - failed to register output\n");
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    for (int p = 0; p != dup->unk_28; p++) {
        if ((bool)((in->unk_07 >> p) & 1)) {
            cls0805c244 *l = (cls0805c244 *)orig->func_0805a908(p);
            if (!l) {
                func_0804ad38("failed to get lock list for old reg in move undo\n");
                func_0804aeb0();
            }
            if (!in->unk_2c[p].getParent()) {
                func_0804ad38("undoMove: failed to get parent for old lock!\n");
                func_0804aeb0();
            }
            if (!l->removeInput(&in->unk_2c[p])) {
                func_0804ad38("undoMove : failed to register input lock\n");
                func_0804aeb0();
            }
        }
    }
    if (mv->unk_138b != 2) {
        func_0804ad38("move with >2 operands\n");
        func_0804aeb0();
    }
    func_08073bb0(blk, mv);
    adjustMoveCount(blk, mv, -1);
    addFreeMove(mv);
    return true;
}

bool undoMovesTo(Block *blk, Instr *stop)
{
    bool done;
    Instr *i = blk->unk_98;
    do {
        Instr *prev = i->unk_24.unk_04;
        done = (i == stop);
        if (i->unk_1428) {
            if (!undoMove(blk, i)) {
                func_0804ad38("failed to undo move !\n");
                func_0804aeb0();
            }
        }
        i = prev;
    } while (!done);
    return true;
}
