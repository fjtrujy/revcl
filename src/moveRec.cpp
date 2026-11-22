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

    moveRec *moveRecord = (moveRec *)freeMoveRecordsByType[type];
    if (moveRecord) {
        freeMoveRecordsByType[type] = (VclPtrWord)moveRecord->nextFree;
        Instr *ownedInstruction = moveRecord->instruction;
        if (!ownedInstruction) {
            reportError("getFreeMove - inst not set\n");
            exitAfterCleanup(-1);
        }
        if (ownedInstruction->moveRecordWord != (VclPtrWord)moveRecord) {
            reportError(moveRecMismatchError, ownedInstruction->moveRecordWord, moveRecord);
            exitAfterCleanup(-1);
        }
        if (!moveRecord->moveRegister) {
            reportError("getFreeMove: m_Reg not set\n");
            exitAfterCleanup(-1);
        }
        ownedInstruction->operands[0].assignedRegister = moveRecord->moveRegister;
        moveRecord->moveRegister->moveReset();
        for (int movePartIndex = 0; movePartIndex != moveRecord->moveRegister->partCount; movePartIndex++) {
            RegisterPartLockList *registerPartLocks = &moveRecord->partLocks[movePartIndex];
            if (!registerPartLocks) {
                reportError(moveRecNullRllError);
                abortAfterError();
            }
            registerPartLocks->nextLockList = 0;
            registerPartLocks->reset();
            registerPartLocks->calculateLoopStats = 0;
            moveRecord->moveRegister->setPartPayload(movePartIndex, (VclPtrWord)registerPartLocks);
        }
        ownedInstruction->nextMoveInstruction = 0;
        ownedInstruction->pendingLink.previousInstruction = 0;
        ownedInstruction->pendingLink.nextInstruction = 0;
        ownedInstruction->linearLink.previousInstruction = 0;
        ownedInstruction->linearLink.nextInstruction = 0;
        ownedInstruction->ownerTable = 0;
        ownedInstruction->position.clearInstructionPosition();
        ownedInstruction->resetOptimizationState();
        return ownedInstruction;
    }

    moveRecord = new moveRec(type);
    if (!moveRecord) {
        reportError("new : failed to get moveRec\n");
        exitAfterCleanup(-1);
    }
    short opc = -1;
    RegPool *registerPool = 0;
    if (type == 1) {
        opc = 0xb2;
        registerPool = GetRP(8);
    } else if (type == 2) {
        opc = 0xb4;
        registerPool = GetRP(9);
    } else {
        reportError("bad move type\n");
        exitAfterCleanup(-1);
    }
    iInfo *instructionInfo = getOpcodeInfoById(opc);
    if (!instructionInfo) {
        reportError("got here - no ii\n");
        exitAfterCleanup(-1);
    }
    Instr *newInstruction = new Instr(instructionInfo);
    if (!newInstruction) {
        reportError(moveRecNewInstrError);
        exitAfterCleanup(-1);
    }
    int serial = nextInstructionSerial++;
    newInstruction->instructionSerial = serial;
    newInstruction->abstractMoveOpcode = opc;
    newInstruction->concreteMoveOpcode = 0;
    RegPoolEntry *reg = registerPool->getMoveReg(newInstruction);
    if (!reg) {
        reportError("failed to get move reg\n");
        exitAfterCleanup(-1);
    }
    newInstruction->moveRecordWord = (VclPtrWord)moveRecord;
    Operand *operand = newInstruction->getOp(0);
    if (!operand) {
        reportError("failed to get move output!\n");
        exitAfterCleanup(1);
    }
    operand->assignedRegister = reg;
    if (newInstruction->operandCount != 2) {
        reportError(moveRecSidefxError);
        exitAfterCleanup(-1);
    }
    for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
        RegisterPartLockList *partLockList = &moveRecord->partLocks[partIndex];
        partLockList->nextLockList = 0;
        partLockList->reset();
        partLockList->calculateLoopStats = 0;
        reg->setPartPayload(partIndex, (VclPtrWord)partLockList);
    }
    moveRecord->instruction = newInstruction;
    moveRecord->moveRegister = reg;
    return newInstruction;
}

bool addFreeMove(Instr *freeMoveInstruction)
{
    if (!freeMoveInstruction) {
        reportError("addFReeMove - null input\n");
        exitAfterCleanup(-1);
    }
    moveRec *moveRecord = (moveRec *)freeMoveInstruction->moveRecordWord;
    if (!moveRecord) {
        reportError("not a move to free\n");
        exitAfterCleanup(-1);
    }
    RegPoolEntry *reg = moveRecord->moveRegister;
    if (!reg) {
        reportError("m_Reg not set in mr\n");
        exitAfterCleanup(-1);
    }
    reg->moveReset();
    for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
        RegisterPartLockList *registerPartLocks = &moveRecord->partLocks[partIndex];
        registerPartLocks->nextLockList = 0;
        registerPartLocks->reset();
    }
    if (freeMoveInstruction != moveRecord->instruction) {
        reportError("no instruction set in mr\n");
        exitAfterCleanup(-1);
    }
    freeMoveInstruction->pendingLink.previousInstruction = 0;
    freeMoveInstruction->pendingLink.nextInstruction = 0;
    freeMoveInstruction->linearLink.previousInstruction = 0;
    freeMoveInstruction->linearLink.nextInstruction = 0;
    freeMoveInstruction->ownerTable = 0;
    freeMoveInstruction->position.clearInstructionPosition();
    freeMoveInstruction->resetOptimizationState();
    if (!freeMoveInstruction->abstractMoveOpcode) {
        reportError(moveRecBaseMuidError);
        return false;
    }
    iInfo *instructionInfo = getOpcodeInfoById(freeMoveInstruction->abstractMoveOpcode);
    freeMoveInstruction->concreteMoveOpcode = 0;
    freeMoveInstruction->opcodeInfo = instructionInfo;
    freeMoveInstruction->permittedPipelineMask = instructionInfo->pipelineMask;
    freeMoveInstruction->opcodeResourceMask = instructionInfo->resourceMask;
    moveRecord->nextFree = (moveRec *)freeMoveRecordsByType[moveRecord->moveType];
    freeMoveRecordsByType[moveRecord->moveType] = (VclPtrWord)moveRecord;
    return true;
}

bool detectMove(Instr *instruction, int &opc, unsigned char &mask, int &dstOp, int &srcOp)
{
    opc = -1;
    mask = 0;
    if (!instruction)
        return false;
    int opcodeId = getOpcodeId(instruction->opcodeInfo);
    srcOp = -1;
    dstOp = -1;
    unsigned int operandMask;
    switch (opcodeId) {
    case 0x99:
        opc = 0xb4;
        dstOp = 0;
        srcOp = 1;
        break;
    case 0x22:
    case 0x30:
        if (instruction->destinationMask & 8)
            return false;
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x44:
        if (instruction->hasLinkedOutputFrom(1))
            return false;
        if (instruction->operands[2].assignedRegister->registerIndex != 0)
            return false;
        opc = 0xb4;
    setOps:
        srcOp = 1;
        dstOp = 0;
        break;
    case 0x6c:
    case 0x6f:
        if (instruction->operands[2].assignedRegister->registerIndex != 32)
            return false;
        opc = 0xb2;
        goto setOps;
    case 0x6d:
    case 0x6e:
    case 0x70:
        if (!instruction->operands[2].immediateValue.getUInt(&operandMask))
            return false;
        if (operandMask)
            return false;
        dstOp = 0;
        srcOp = 1;
        opc = 0xb2;
        break;
    default:
        return false;
    }
    RegPoolEntry *registerEntry = instruction->operands[srcOp].assignedRegister;
    RegPoolEntry *destinationRegister = instruction->operands[dstOp].assignedRegister;
    if (!registerEntry || !destinationRegister) {
        reportError(moveDetectNoRegError);
        exitAfterCleanup(-1);
        return false;
    }
    if (registerEntry->registerClass != destinationRegister->registerClass) {
        reportError("move detect - bad types\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (opc < 0) {
        reportError("bad uid for move detection\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (instruction->operands[srcOp].componentMask != instruction->operands[dstOp].componentMask) {
        reportError("bad mod bits for a move\n");
        exitAfterCleanup(-1);
    }
    mask = instruction->operands[srcOp].componentMask;
    if (!mask) {
        reportWarning("modbits 0 for move..avoiding\n");
        return false;
    }
    return true;
}

bool resolveMovePipe(workTable *scheduleTable, Instr *instruction, int pipe)
{
    if (instruction->abstractMoveOpcode == 0xb2)
        instruction->concreteMoveOpcode = 0xb3;
    else if (instruction->abstractMoveOpcode == 0xb4)
        instruction->concreteMoveOpcode = pipe ? 0xb5 : 0xb6;
    else {
        reportError("resolve move: bad baseMuid\n");
        exitAfterCleanup(-1);
        return false;
    }
    if (instruction->permittedPipelineMask == 3)
        scheduleTable->vectorMoveCount--;
    else if (instruction->permittedPipelineMask & 1)
        scheduleTable->oddPipeMoveCount--;
    else
        scheduleTable->evenPipeMoveCount--;
    iInfo *instructionInfo = getOpcodeInfoById(instruction->concreteMoveOpcode);
    instruction->opcodeInfo = instructionInfo;
    instruction->permittedPipelineMask = instructionInfo->pipelineMask;
    instruction->opcodeResourceMask = instructionInfo->resourceMask;
    if (instruction->permittedPipelineMask == 3)
        scheduleTable->vectorMoveCount++;
    else if (instruction->permittedPipelineMask & 1)
        scheduleTable->oddPipeMoveCount++;
    else
        scheduleTable->evenPipeMoveCount++;
    return true;
}

int unresolveMove(Block *blk, Instr *instruction)
{
    iInfo *instructionInfo = getOpcodeInfoById(instruction->abstractMoveOpcode);
    if (instruction->permittedPipelineMask == 3)
        blk->vectorMoveCount--;
    else if (instruction->permittedPipelineMask & 1)
        blk->oddPipeMoveCount--;
    else
        blk->evenPipeMoveCount--;
    instruction->concreteMoveOpcode = 0;
    instruction->opcodeInfo = instructionInfo;
    instruction->permittedPipelineMask = instructionInfo->pipelineMask;
    instruction->opcodeResourceMask = instructionInfo->resourceMask;
    if (instruction->permittedPipelineMask == 3)
        blk->vectorMoveCount++;
    else if (instruction->permittedPipelineMask & 1)
        blk->oddPipeMoveCount++;
    else
        blk->evenPipeMoveCount++;
    return 1;
}

void adjustMoveCount(Block *blk, Instr *instruction, int moveCountDelta)
{
    if (instruction->permittedPipelineMask == 3)
        blk->vectorMoveCount += moveCountDelta;
    else if (instruction->permittedPipelineMask & 1)
        blk->oddPipeMoveCount += moveCountDelta;
    else
        blk->evenPipeMoveCount += moveCountDelta;
}
