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
        g_080de714[type] = (VclPtrWord)mr->nextFree;
        Instr *in = mr->instruction;
        if (!in) {
            reportError("getFreeMove - inst not set\n");
            exitAfterCleanup(-1);
        }
        if (in->moveRecordWord != (VclPtrWord)mr) {
            reportError(moveRecMismatchError, in->moveRecordWord, mr);
            exitAfterCleanup(-1);
        }
        if (!mr->moveRegister) {
            reportError("getFreeMove: m_Reg not set\n");
            exitAfterCleanup(-1);
        }
        in->operands[0].assignedRegister = mr->moveRegister;
        mr->moveRegister->moveReset();
        for (int i = 0; i != mr->moveRegister->partCount; i++) {
            RegisterPartLockList *l = &mr->partLocks[i];
            if (!l) {
                reportError(moveRecNullRllError);
                abortAfterError();
            }
            l->nextLockList = 0;
            l->reset();
            l->unk_29 = 0;
            mr->moveRegister->setPartPayload(i, (VclPtrWord)l);
        }
        in->nextMoveInstruction = 0;
        in->pendingLink.previousInstruction = 0;
        in->pendingLink.nextInstruction = 0;
        in->linearLink.previousInstruction = 0;
        in->linearLink.nextInstruction = 0;
        in->ownerTable = 0;
        in->position.clearInstructionPosition();
        in->resetOptimizationState();
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
    iInfo *ii = getOpcodeInfoById(opc);
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
    in->instructionSerial = serial;
    in->abstractMoveOpcode = opc;
    in->concreteMoveOpcode = 0;
    RegPoolEntry *reg = rp->getMoveReg(in);
    if (!reg) {
        reportError("failed to get move reg\n");
        exitAfterCleanup(-1);
    }
    in->moveRecordWord = (VclPtrWord)mr;
    Operand *op = in->getOp(0);
    if (!op) {
        reportError("failed to get move output!\n");
        exitAfterCleanup(1);
    }
    op->assignedRegister = reg;
    if (in->operandCount != 2) {
        reportError(moveRecSidefxError);
        exitAfterCleanup(-1);
    }
    for (int i = 0; i != reg->partCount; i++) {
        RegisterPartLockList *l = &mr->partLocks[i];
        l->nextLockList = 0;
        l->reset();
        l->unk_29 = 0;
        reg->setPartPayload(i, (VclPtrWord)l);
    }
    mr->instruction = in;
    mr->moveRegister = reg;
    return in;
}

bool addFreeMove(Instr *in)
{
    if (!in) {
        reportError("addFReeMove - null input\n");
        exitAfterCleanup(-1);
    }
    moveRec *mr = (moveRec *)in->moveRecordWord;
    if (!mr) {
        reportError("not a move to free\n");
        exitAfterCleanup(-1);
    }
    RegPoolEntry *reg = mr->moveRegister;
    if (!reg) {
        reportError("m_Reg not set in mr\n");
        exitAfterCleanup(-1);
    }
    reg->moveReset();
    for (int i = 0; i != reg->partCount; i++) {
        RegisterPartLockList *l = &mr->partLocks[i];
        l->nextLockList = 0;
        l->reset();
    }
    if (in != mr->instruction) {
        reportError("no instruction set in mr\n");
        exitAfterCleanup(-1);
    }
    in->pendingLink.previousInstruction = 0;
    in->pendingLink.nextInstruction = 0;
    in->linearLink.previousInstruction = 0;
    in->linearLink.nextInstruction = 0;
    in->ownerTable = 0;
    in->position.clearInstructionPosition();
    in->resetOptimizationState();
    if (!in->abstractMoveOpcode) {
        reportError(moveRecBaseMuidError);
        return false;
    }
    iInfo *ii = getOpcodeInfoById(in->abstractMoveOpcode);
    in->concreteMoveOpcode = 0;
    in->opcodeInfo = ii;
    in->permittedPipelineMask = ii->pipelineMask;
    in->opcodeResourceMask = ii->resourceMask;
    mr->nextFree = (moveRec *)g_080de714[mr->moveType];
    g_080de714[mr->moveType] = (VclPtrWord)mr;
    return true;
}

bool detectMove(Instr *in, int &opc, unsigned char &mask, int &dstOp, int &srcOp)
{
    opc = -1;
    mask = 0;
    if (!in)
        return false;
    int id = getOpcodeId(in->opcodeInfo);
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
        if (in->destinationMask & 8)
            return false;
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x44:
        if (in->hasLinkedOutputFrom(1))
            return false;
        if (in->operands[2].assignedRegister->registerIndex != 0)
            return false;
        opc = 0xb4;
    setOps:
        srcOp = 1;
        dstOp = 0;
        break;
    case 0x6c:
    case 0x6f:
        if (in->operands[2].assignedRegister->registerIndex != 32)
            return false;
        opc = 0xb2;
        goto setOps;
    case 0x6d:
    case 0x6e:
    case 0x70:
        if (!in->operands[2].immediateValue.getUInt(&v))
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
    RegPoolEntry *s = in->operands[srcOp].assignedRegister;
    RegPoolEntry *d = in->operands[dstOp].assignedRegister;
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
    if (in->operands[srcOp].componentMask != in->operands[dstOp].componentMask) {
        reportError("bad mod bits for a move\n");
        exitAfterCleanup(-1);
    }
    mask = in->operands[srcOp].componentMask;
    if (!mask) {
        reportWarning("modbits 0 for move..avoiding\n");
        return false;
    }
    return true;
}

bool resolveMovePipe(workTable *wt, Instr *mv, int pipe)
{
    if (mv->abstractMoveOpcode == 0xb2)
        mv->concreteMoveOpcode = 0xb3;
    else if (mv->abstractMoveOpcode == 0xb4)
        mv->concreteMoveOpcode = pipe ? 0xb5 : 0xb6;
    else {
        reportError("resolve move: bad baseMuid\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (mv->permittedPipelineMask == 3)
        wt->vectorMoveCount--;
    else if (mv->permittedPipelineMask & 1)
        wt->oddPipeMoveCount--;
    else
        wt->evenPipeMoveCount--;
    iInfo *ii = getOpcodeInfoById(mv->concreteMoveOpcode);
    mv->opcodeInfo = ii;
    mv->permittedPipelineMask = ii->pipelineMask;
    mv->opcodeResourceMask = ii->resourceMask;
    if (mv->permittedPipelineMask == 3)
        wt->vectorMoveCount++;
    else if (mv->permittedPipelineMask & 1)
        wt->oddPipeMoveCount++;
    else
        wt->evenPipeMoveCount++;
    return true;
}

int unresolveMove(Block *blk, Instr *mv)
{
    iInfo *ii = getOpcodeInfoById(mv->abstractMoveOpcode);
    if (mv->permittedPipelineMask == 3)
        blk->vectorMoveCount--;
    else if (mv->permittedPipelineMask & 1)
        blk->oddPipeMoveCount--;
    else
        blk->evenPipeMoveCount--;
    mv->concreteMoveOpcode = 0;
    mv->opcodeInfo = ii;
    mv->permittedPipelineMask = ii->pipelineMask;
    mv->opcodeResourceMask = ii->resourceMask;
    if (mv->permittedPipelineMask == 3)
        blk->vectorMoveCount++;
    else if (mv->permittedPipelineMask & 1)
        blk->oddPipeMoveCount++;
    else
        blk->evenPipeMoveCount++;
    return 1;
}

void adjustMoveCount(Block *blk, Instr *mv, int n)
{
    if (mv->permittedPipelineMask == 3)
        blk->vectorMoveCount += n;
    else if (mv->permittedPipelineMask & 1)
        blk->oddPipeMoveCount += n;
    else
        blk->evenPipeMoveCount += n;
}
