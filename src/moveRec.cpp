#include "moveRec.h"
#include "Processor.h"
#include "os.h"
#include "Block.h"
#include "workTable.h"

RegPool *GetRP(unsigned t);

extern const char moveRecMismatchError[];
extern const char moveRecNullRllError[];
extern const char moveRecSidefxError[];
extern const char moveRecNewInstrError[];
extern const char moveRecBaseMuidError[];
extern const char moveDetectNoRegError[];

Instr *getMove(int type, int)
{
    if (type == 0) {
        reportError("can't get null move\n");
        exitAfterCleanup(-1);
    }
    if (type > 2) {
        reportError("out of range for get move\n");
        exitAfterCleanup(-1);
    }

    moveRec *mr = (moveRec *)g_080de714[type];
    if (mr) {
        g_080de714[type] = (VclPtrWord)mr->unk_00;
        Instr *in = mr->unk_08;
        if (!in) {
            reportError("getFreeMove - inst not set\n");
            exitAfterCleanup(-1);
        }
        if (in->unk_1428 != (VclPtrWord)mr) {
            reportError(moveRecMismatchError, in->unk_1428, mr);
            exitAfterCleanup(-1);
        }
        if (!mr->unk_0c) {
            reportError("getFreeMove: m_Reg not set\n");
            exitAfterCleanup(-1);
        }
        in->operands[0].unk_18 = mr->unk_0c;
        mr->unk_0c->moveReset();
        for (int i = 0; i != mr->unk_0c->partCount; i++) {
            RegisterPartLockList *l = &mr->unk_10[i];
            if (!l) {
                reportError(moveRecNullRllError);
                abortAfterError();
            }
            l->unk_08 = 0;
            l->reset();
            l->unk_29 = 0;
            mr->unk_0c->func_0805a924(i, (VclPtrWord)l);
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
        reportError("new : failed to get moveRec\n");
        exitAfterCleanup(-1);
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
        reportError("bad move type\n");
        exitAfterCleanup(-1);
    }
    iInfo *ii = func_0806bf74(opc);
    if (!ii) {
        reportError("got here - no ii\n");
        exitAfterCleanup(-1);
    }
    Instr *in = new Instr(ii);
    if (!in) {
        reportError(moveRecNewInstrError);
        exitAfterCleanup(-1);
    }
    int serial = g_080b33a8++;
    in->unk_13e0 = serial;
    in->unk_142c = opc;
    in->unk_142e = 0;
    RegPoolEntry *reg = rp->getMoveReg(in);
    if (!reg) {
        reportError("failed to get move reg\n");
        exitAfterCleanup(-1);
    }
    in->unk_1428 = (VclPtrWord)mr;
    Operand *op = in->getOp(0);
    if (!op) {
        reportError("failed to get move output!\n");
        exitAfterCleanup(1);
    }
    op->unk_18 = reg;
    if (in->operandCount != 2) {
        reportError(moveRecSidefxError);
        exitAfterCleanup(-1);
    }
    for (int i = 0; i != reg->partCount; i++) {
        RegisterPartLockList *l = &mr->unk_10[i];
        l->unk_08 = 0;
        l->reset();
        l->unk_29 = 0;
        reg->func_0805a924(i, (VclPtrWord)l);
    }
    mr->unk_08 = in;
    mr->unk_0c = reg;
    return in;
}

bool addFreeMove(Instr *in)
{
    if (!in) {
        reportError("addFReeMove - null input\n");
        exitAfterCleanup(-1);
    }
    moveRec *mr = (moveRec *)in->unk_1428;
    if (!mr) {
        reportError("not a move to free\n");
        exitAfterCleanup(-1);
    }
    RegPoolEntry *reg = mr->unk_0c;
    if (!reg) {
        reportError("m_Reg not set in mr\n");
        exitAfterCleanup(-1);
    }
    reg->moveReset();
    for (int i = 0; i != reg->partCount; i++) {
        RegisterPartLockList *l = &mr->unk_10[i];
        l->unk_08 = 0;
        l->reset();
    }
    if (in != mr->unk_08) {
        reportError("no instruction set in mr\n");
        exitAfterCleanup(-1);
    }
    in->unk_24.unk_04 = 0;
    in->unk_24.unk_00 = 0;
    in->unk_1c.unk_04 = 0;
    in->unk_1c.unk_00 = 0;
    in->unk_30 = 0;
    in->unk_13e8.func_080586e0();
    in->func_08054b84();
    if (!in->unk_142c) {
        reportError(moveRecBaseMuidError);
        return false;
    }
    iInfo *ii = func_0806bf74(in->unk_142c);
    in->unk_142e = 0;
    in->opcodeInfo = ii;
    in->unk_138c = ii->unk_0e;
    in->unk_138d = ii->unk_10;
    mr->unk_00 = (moveRec *)g_080de714[mr->unk_04];
    g_080de714[mr->unk_04] = (VclPtrWord)mr;
    return true;
}

bool detectMove(Instr *in, int &opc, unsigned char &mask, int &dstOp, int &srcOp)
{
    opc = -1;
    mask = 0;
    if (!in)
        return false;
    int id = func_0806c72c(in->opcodeInfo);
    srcOp = -1;
    dstOp = -1;
    unsigned int v;
    switch (id) {
    case 0x99:
        opc = 0xb4;
        dstOp = 0;
        srcOp = 1;
        break;
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
        if (in->operands[2].unk_18->unk_30 != 0)
            return false;
        opc = 0xb4;
    setOps:
        srcOp = 1;
        dstOp = 0;
        break;
    case 0x6c:
    case 0x6f:
        if (in->operands[2].unk_18->unk_30 != 32)
            return false;
        opc = 0xb2;
        goto setOps;
    case 0x6d:
    case 0x6e:
    case 0x70:
        if (!in->operands[2].unk_1c.getUInt(&v))
            return false;
        if (v)
            return false;
        dstOp = 0;
        srcOp = 1;
        opc = 0xb2;
        break;
    default:
        return false;
    }
    RegPoolEntry *s = in->operands[srcOp].unk_18;
    RegPoolEntry *d = in->operands[dstOp].unk_18;
    if (!s || !d) {
        reportError(moveDetectNoRegError);
        exitAfterCleanup(-1);
        return false;
    }
    if (s->registerClass != d->registerClass) {
        reportError("move detect - bad types\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (opc < 0) {
        reportError("bad uid for move detection\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (in->operands[srcOp].unk_07 != in->operands[dstOp].unk_07) {
        reportError("bad mod bits for a move\n");
        exitAfterCleanup(-1);
    }
    mask = in->operands[srcOp].unk_07;
    if (!mask) {
        reportWarning("modbits 0 for move..avoiding\n");
        return false;
    }
    return true;
}

bool resolveMovePipe(workTable *wt, Instr *mv, int pipe)
{
    if (mv->unk_142c == 0xb2)
        mv->unk_142e = 0xb3;
    else if (mv->unk_142c == 0xb4)
        mv->unk_142e = pipe ? 0xb5 : 0xb6;
    else {
        reportError("resolve move: bad baseMuid\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (mv->unk_138c == 3)
        wt->unk_b0--;
    else if (mv->unk_138c & 1)
        wt->unk_b4--;
    else
        wt->unk_b8--;
    iInfo *ii = func_0806bf74(mv->unk_142e);
    mv->opcodeInfo = ii;
    mv->unk_138c = ii->unk_0e;
    mv->unk_138d = ii->unk_10;
    if (mv->unk_138c == 3)
        wt->unk_b0++;
    else if (mv->unk_138c & 1)
        wt->unk_b4++;
    else
        wt->unk_b8++;
    return true;
}

int unresolveMove(Block *blk, Instr *mv)
{
    iInfo *ii = func_0806bf74(mv->unk_142c);
    if (mv->unk_138c == 3)
        blk->unk_b0--;
    else if (mv->unk_138c & 1)
        blk->unk_b4--;
    else
        blk->unk_b8--;
    mv->unk_142e = 0;
    mv->opcodeInfo = ii;
    mv->unk_138c = ii->unk_0e;
    mv->unk_138d = ii->unk_10;
    if (mv->unk_138c == 3)
        blk->unk_b0++;
    else if (mv->unk_138c & 1)
        blk->unk_b4++;
    else
        blk->unk_b8++;
    return 1;
}

void adjustMoveCount(Block *blk, Instr *mv, int n)
{
    if (mv->unk_138c == 3)
        blk->unk_b0 += n;
    else if (mv->unk_138c & 1)
        blk->unk_b4 += n;
    else
        blk->unk_b8 += n;
}
