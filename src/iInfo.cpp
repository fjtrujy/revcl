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
