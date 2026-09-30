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
