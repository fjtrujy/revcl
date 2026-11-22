#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "decodedLine.h"

Instr *addXinFor(instStream *s, RegPoolEntry *r);
Instr *addXoutFor(instStream *s, RegPoolEntry *r);
bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag);
RegPoolEntry *getTagRegister(RegPool *pool, char *name);
int getPoolsByClassID(int cls, RegPool **tab, int max);
RegPool *GetRP(unsigned t);

static inline bool addNewInputInl(Instr *instruction, RegPoolEntry *registerEntry, int bits)
{
    if (!addNewOperand(instruction, registerEntry, bits, 0x37, 1, false)) {
        reportError("add new input failed\n");
        return false;
    }
    return true;
}

static inline bool addNewOutputInl(Instr *instruction, RegPoolEntry *registerEntry, int bits, bool f)
{
    if (!addNewOperand(instruction, registerEntry, bits, 0x37, 2, f)) {
        reportError("add new output failed\n");
        return false;
    }
    return true;
}


bool bindDependencies(Program *program)
{
    RegPool *pools[8];
    instStream *stream;
    int hardwarePoolCount = getPoolsByClassID(3, pools, 8);
    if (hardwarePoolCount < 0) {
        reportError("bindDependencies() - Processor failed to enumerate hardware register classes\n");
        return false;
    }
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *scheduleTable = stream->getTable();
        for (Instr *streamInstruction = scheduleTable->linearInstructions.head; streamInstruction; streamInstruction = streamInstruction->linearLink.nextInstruction) {
            int bits = streamInstruction->getRamBits();
            if (!streamInstruction->branchLabelTags)
                continue;
            if (!streamInstruction->ramAccessType) {
                reportUserMessage("tagString found for non memory instruction\n");
                return false;
            }
            if (bits) {
                char *tag = streamInstruction->branchLabelTags;
                RegPool *registerPool = GetRP(10);
                if (!registerPool) {
                    reportError("failed to get a hw pool\n");
                    return false;
                }
                if (registerPool->poolKind != 3) {
                    reportError("umm - not a hardware register\n");
                    return false;
                }
                while (*tag) {
                    while (*tag == ':')
                        tag++;
                    char *end = tag;
                    while (*end && *end != ':')
                        end++;
                    if (*end)
                        *end++ = 0;
                    RegPoolEntry *registerEntry = getTagRegister(registerPool, tag);
                    if (!registerEntry) {
                        reportError("failed to create a tag register\n");
                        return false;
                    }
                    switch ((unsigned char)streamInstruction->ramAccessType) {
                    case 1:
                        if (!addNewInputInl(streamInstruction, registerEntry, bits)) {
                            reportError("failed to add input operand for load\n");
                            return false;
                        }
                        break;
                    case 2:
                        if (!addNewOutputInl(streamInstruction, registerEntry, bits, true)) {
                            reportError("failed to add output operand for store\n");
                            return false;
                        }
                        break;
                    default:
                        reportError("unknown ramdir!\n");
                        return false;
                    }
                    tag = end;
                }
            }
            delete streamInstruction->branchLabelTags;
            streamInstruction->branchLabelTags = 0;
        }
    }
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        bool xin = stream->startType == 2;
        bool xout = stream->endType == 3;
        if (xin) {
            RegPoolEntry *sticky39 = findRegisterByGlobalIndex(0x39);
            if (!addXinFor(stream, sticky39)) {
                reportError("failed to add the fpu sticky flags as xin\n");
                return false;
            }
            RegPoolEntry *sticky3a = findRegisterByGlobalIndex(0x3a);
            if (!addXinFor(stream, sticky3a)) {
                reportError("failed to add the div sticky flags as xin\n");
                return false;
            }
        }
        if (xin || xout) {
            for (int i = 0; i != hardwarePoolCount; i++) {
                RegPool *hardwarePool = pools[i];
                RegPoolEntry *ext = hardwarePool->externalRegister;
                if (!ext) {
                    reportError("failed to get EXT for pool %s\n", hardwarePool->poolName);
                    return false;
                }
                if (xin && !addXinFor(stream, ext)) {
                    reportError("failed to add external input to %s\n", stream->streamName());
                    return false;
                }
                if (xout && !addXoutFor(stream, ext)) {
                    reportError("failed to add external output to %s\n", stream->streamName());
                    return false;
                }
                for (RegPoolEntry *virtualRegister = hardwarePool->virtualHead; virtualRegister; virtualRegister = virtualRegister->next) {
                    if (xin && !addXinFor(stream, virtualRegister)) {
                        reportError("failed to add external input to %s\n", stream->streamName());
                        return false;
                    }
                    // the original calls addXinFor here too
                    if (xout && !addXinFor(stream, virtualRegister)) {
                        reportError("failed to add external output to %s\n", stream->streamName());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

Instr *addXinFor(instStream *stream, RegPoolEntry *registerEntry)
{
    iInfo *info = getOpcodeInfoById(0xbe);
    if (!info) {
        reportError("addXinFor - cant get xin_hw\n");
        return 0;
    }
    Instr *newInputInstruction = new Instr(info);
    if (!newInputInstruction) {
        reportError("failed to make xin instruction\n");
        return 0;
    }
    if (newInputInstruction->operandCount != 1) {
        reportError("xin doesn't have just one operand\n");
        delete newInputInstruction;
        return 0;
    }
    int serial = nextInstructionSerial++;
    newInputInstruction->instructionSerial = serial;
    Operand *externalInputOperand = &newInputInstruction->operands[0];
    if (externalInputOperand->operandKind != 0x37) {
        reportError("xin wasn't external\n");
        delete newInputInstruction;
        return 0;
    }
    externalInputOperand->assignedRegister = registerEntry;
    externalInputOperand->componentMask = (1 << registerEntry->partCount) - 1;
    workTable *scheduleTable = stream->getTable();
    if (!scheduleTable) {
        reportError("can't get work table\n");
        delete newInputInstruction;
        return 0;
    }
    if (!scheduleTable->logAddLinear_Tail(newInputInstruction)) {
        reportError("failed to an external input to the tail of list\n");
        return 0;
    }
    return newInputInstruction;
}

Instr *addXoutFor(instStream *stream, RegPoolEntry *registerEntry)
{
    iInfo *info = getOpcodeInfoById(0xbf);
    if (!info) {
        reportError("addxoutFor - can't get xout_hw\n");
        return 0;
    }
    Instr *newOutputInstruction = new Instr(info);
    if (!newOutputInstruction) {
        reportError("failed to make xout instruction\n");
        return 0;
    }
    if (newOutputInstruction->operandCount != 1) {
        reportError("xout doesn't have just one operand\n");
        delete newOutputInstruction;
        return 0;
    }
    int serial = nextInstructionSerial++;
    newOutputInstruction->instructionSerial = serial;
    Operand *externalOutputOperand = &newOutputInstruction->operands[0];
    if (externalOutputOperand->operandKind != 0x37) {
        reportError("xout wasn't external\n");
        delete newOutputInstruction;
        return 0;
    }
    externalOutputOperand->assignedRegister = registerEntry;
    externalOutputOperand->componentMask = (1 << registerEntry->partCount) - 1;
    workTable *scheduleTable = stream->getTable();
    if (!scheduleTable) {
        reportError("can't get work table\n");
        delete newOutputInstruction;
        return 0;
    }
    if (!scheduleTable->logAddLinear_Head(newOutputInstruction)) {
        reportError("failed to an external output to the head of list\n");
        return 0;
    }
    return newOutputInstruction;
}

bool addNewOperand(Instr *in, RegPoolEntry *registerEntry, int bits, int kind, int dir, int flag)
{
    if (!registerEntry) {
        reportError("add new output - passed a null rg pointer\n");
        return false;
    }
    if (!bits) {
        reportError("passed null mod bits for new output register\n");
        return false;
    }
    if (bits & (-1 << registerEntry->partCount)) {
        reportError("add new output - out of bounds mod bits requested\n");
        return false;
    }
    iInfo *info = in->opcodeInfo;
    if (!info) {
        reportError("instruction has no m_Inst\n");
        return false;
    }
    unsigned char n = in->operandCount;
    int i;
    for (i = 0; i < n; i++) {
        Operand *existingOperand = &in->operands[i];
        if (existingOperand->isImplicit && existingOperand->direction == dir && registerEntry == existingOperand->assignedRegister) {
            existingOperand->componentMask |= bits;
            return true;
        }
    }
    if (i == 12) {
        reportError("ran out of operand space for add new output\n");
        return false;
    }
    Operand *newOperand = &in->operands[n];
    newOperand->operandIndex = n;
    newOperand->externalBlockModeFlag = 1;
    newOperand->direction = dir;
    newOperand->isImplicit = 1;
    newOperand->immediateDataType = 0;
    newOperand->pairedOperandIndexPlusOne = 0;
    newOperand->outputLatency = flag;
    newOperand->additionalLatency = 0;
    newOperand->assignedRegister = registerEntry;
    newOperand->componentMask = bits;
    newOperand->operandKind = kind;
    in->operandCount++;
    return in->validateOperands();
}

bool addNewInput(Instr *instruction, RegPoolEntry *registerEntry, int bits)
{
    if (!addNewOperand(instruction, registerEntry, bits, 0x37, 1, false)) {
        reportError("add new input failed\n");
        return false;
    }
    return true;
}

bool addNewOutput(Instr *instruction, RegPoolEntry *registerEntry, int bits, int flag)
{
    if (!addNewOperand(instruction, registerEntry, bits, 0x37, 2, flag)) {
        reportError("add new output failed\n");
        return false;
    }
    return true;
}

RegPoolEntry *getTagRegister(RegPool *pool, char *name)
{
    if (RegPool::isResvd(name, pool->registerClass)) {
        reportUserMessage("can't use %s as a name for one of the %s\n", name, pool->poolName);
        return 0;
    }
    RegPoolEntry *registerEntry = pool->findVirtualByName(name);
    if (!registerEntry) {
        registerEntry = pool->createVirtualRegister(name);
        if (!registerEntry) {
            reportError("failed to create a virtual reg\n");
            return 0;
        }
    }
    registerEntry->isVirtual = false;
    registerEntry->partCount = 4;
    return registerEntry;
}
