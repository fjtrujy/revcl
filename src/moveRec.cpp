#include "moveRec.h"
#include "Processor.h"
#include "os.h"

RegPool *GetRP(unsigned t);

Instr *getMove(int type, int)
{
    if (type == 0) {
        func_0804ad38("can't get null move\n");
        func_0804adf0(-1);
    }
    if (type > 2) {
        func_0804ad38("out of range for get move\n");
        func_0804adf0(-1);
    }

    moveRec *mr = (moveRec *)g_080de714[type];
    if (mr) {
        g_080de714[type] = (int)mr->unk_00;
        Instr *in = mr->unk_08;
        if (!in) {
            func_0804ad38("getFreeMove - inst not set\n");
            func_0804adf0(-1);
        }
        if (in->unk_1428 != (int)mr) {
            func_0804ad38("getFreeMove - mr not same %x!=%x\n", in->unk_1428, mr);
            func_0804adf0(-1);
        }
        if (!mr->unk_0c) {
            func_0804ad38("getFreeMove: m_Reg not set\n");
            func_0804adf0(-1);
        }
        in->unk_38[0].unk_18 = mr->unk_0c;
        mr->unk_0c->moveReset();
        for (int i = 0; i != mr->unk_0c->unk_28; i++) {
            cls0805c244 *l = &mr->unk_10[i];
            if (!l) {
                func_0804ad38("getFreeLocalMove() - null rll pointer in cached move record\n");
                func_0804aeb0();
            }
            l->unk_08 = 0;
            l->reset();
            l->unk_29 = 0;
            mr->unk_0c->func_0805a924(i, (int)l);
        }
        in->unk_2c = 0;
        in->unk_24.unk_04 = 0;
        in->unk_24.unk_00 = 0;
        in->unk_1c.unk_04 = 0;
        in->unk_1c.unk_00 = 0;
        in->unk_30 = 0;
        in->unk_13e8.func_080586e0();
        in->func_08054b84();
        return in;
    }

    mr = new moveRec(type);
    if (!mr) {
        func_0804ad38("new : failed to get moveRec\n");
        func_0804adf0(-1);
    }
    short opc = -1;
    RegPool *rp = 0;
    if (type == 1) {
        opc = 0xb2;
        rp = GetRP(8);
    } else if (type == 2) {
        opc = 0xb4;
        rp = GetRP(9);
    } else {
        func_0804ad38("bad move type\n");
        func_0804adf0(-1);
    }
    iInfo *ii = func_0806bf74(opc);
    if (!ii) {
        func_0804ad38("got here - no ii\n");
        func_0804adf0(-1);
    }
    Instr *in = new Instr(ii);
    if (!in) {
        func_0804ad38("failed to get a new instruction for move\n");
        func_0804adf0(-1);
    }
    int serial = g_080b33a8++;
    in->unk_13e0 = serial;
    in->unk_142c = opc;
    in->unk_142e = 0;
    RegPoolEntry *reg = rp->getMoveReg(in);
    if (!reg) {
        func_0804ad38("failed to get move reg\n");
        func_0804adf0(-1);
    }
    in->unk_1428 = (int)mr;
    Operand *op = in->getOp(0);
    if (!op) {
        func_0804ad38("failed to get move output!\n");
        func_0804adf0(1);
    }
    op->unk_18 = reg;
    if (in->unk_138b != 2) {
        func_0804ad38("add code to handle sidefx for this move target\n");
        func_0804adf0(-1);
    }
    for (int i = 0; i != reg->unk_28; i++) {
        cls0805c244 *l = &mr->unk_10[i];
        l->unk_08 = 0;
        l->reset();
        l->unk_29 = 0;
        reg->func_0805a924(i, (int)l);
    }
    mr->unk_08 = in;
    mr->unk_0c = reg;
    return in;
}

bool addFreeMove(Instr *in)
{
    if (!in) {
        func_0804ad38("addFReeMove - null input\n");
        func_0804adf0(-1);
    }
    moveRec *mr = (moveRec *)in->unk_1428;
    if (!mr) {
        func_0804ad38("not a move to free\n");
        func_0804adf0(-1);
    }
    RegPoolEntry *reg = mr->unk_0c;
    if (!reg) {
        func_0804ad38("m_Reg not set in mr\n");
        func_0804adf0(-1);
    }
    reg->moveReset();
    for (int i = 0; i != reg->unk_28; i++) {
        cls0805c244 *l = &mr->unk_10[i];
        l->unk_08 = 0;
        l->reset();
    }
    if (in != mr->unk_08) {
        func_0804ad38("no instruction set in mr\n");
        func_0804adf0(-1);
    }
    in->unk_24.unk_04 = 0;
    in->unk_24.unk_00 = 0;
    in->unk_1c.unk_04 = 0;
    in->unk_1c.unk_00 = 0;
    in->unk_30 = 0;
    in->unk_13e8.func_080586e0();
    in->func_08054b84();
    if (!in->unk_142c) {
        func_0804ad38("baseMuid is not set for free move\n");
        return false;
    }
    iInfo *ii = func_0806bf74(in->unk_142c);
    in->unk_142e = 0;
    in->unk_34 = ii;
    in->unk_138c = ii->unk_0e;
    in->unk_138d = ii->unk_10;
    mr->unk_00 = (moveRec *)g_080de714[mr->unk_04];
    g_080de714[mr->unk_04] = (int)mr;
    return true;
}

bool detectMove(Instr *in, int *opc, unsigned char *mask, int *dstOp, int *srcOp)
{
    *opc = -1;
    *mask = 0;
    if (!in)
        return false;
    int id = func_0806c72c(in->unk_34);
    *srcOp = *dstOp = -1;
    unsigned int v;
    switch (id) {
    case 0x22:
    case 0x30:
        if (in->unk_138f & 8)
            return false;
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x44:
        if (in->func_08055828(1))
            return false;
        if (in->unk_38[2].unk_18->unk_30 != 0)
            return false;
        *opc = 0xb4;
        *srcOp = 1;
        *dstOp = 0;
        break;
    case 0x6c:
    case 0x6f:
        if (in->unk_38[2].unk_18->unk_30 != 32)
            return false;
        *opc = 0xb2;
        *srcOp = 1;
        *dstOp = 0;
        break;
    case 0x6d:
    case 0x6e:
    case 0x70:
        if (!in->unk_38[2].unk_1c.getUInt(&v))
            return false;
        if (v)
            return false;
        *dstOp = 0;
        *srcOp = 1;
        *opc = 0xb2;
        break;
    case 0x99:
        *opc = 0xb4;
        *dstOp = 0;
        *srcOp = 1;
        break;
    default:
        return false;
    }
    RegPoolEntry *d = in->unk_38[*dstOp].unk_18;
    RegPoolEntry *s = in->unk_38[*srcOp].unk_18;
    if (!s || !d) {
        func_0804ad38("move detect error - reg not set\n");
        func_0804adf0(-1);
        return false;
    }
    if (s->unk_24 != d->unk_24) {
        func_0804ad38("move detect - bad types\n");
        func_0804adf0(-1);
        return false;
    }
    if (*opc < 0) {
        func_0804ad38("bad uid for move detection\n");
        func_0804adf0(-1);
        return false;
    }
    if (in->unk_38[*srcOp].unk_07 != in->unk_38[*dstOp].unk_07) {
        func_0804ad38("bad mod bits for a move\n");
        func_0804adf0(-1);
    }
    *mask = in->unk_38[*srcOp].unk_07;
    if (!*mask) {
        func_0804ac7c("modbits 0 for move..avoiding\n");
        return false;
    }
    return true;
}
