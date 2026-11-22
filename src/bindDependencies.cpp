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

static inline bool addNewInputInl(Instr *in, RegPoolEntry *r, int bits)
{
    if (!addNewOperand(in, r, bits, 0x37, 1, false)) {
        reportError("add new input failed\n");
        return false;
    }
    return true;
}

static inline bool addNewOutputInl(Instr *in, RegPoolEntry *r, int bits, bool f)
{
    if (!addNewOperand(in, r, bits, 0x37, 2, f)) {
        reportError("add new output failed\n");
        return false;
    }
    return true;
}


bool bindDependencies(Program *p)
{
    RegPool *pools[8];
    instStream *s;
    int n = getPoolsByClassID(3, pools, 8);
    if (n < 0) {
        reportError("bindDependencies() - Processor failed to enumerate hardware register classes\n");
        return false;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        for (Instr *in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
            int bits = in->getRamBits();
            if (!in->branchLabelTags)
                continue;
            if (!in->ramAccessType) {
                reportUserMessage("tagString found for non memory instruction\n");
                return false;
            }
            if (bits) {
                char *tag = in->branchLabelTags;
                RegPool *hw = GetRP(10);
                if (!hw) {
                    reportError("failed to get a hw pool\n");
                    return false;
                }
                if (hw->poolKind != 3) {
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
                    RegPoolEntry *r = getTagRegister(hw, tag);
                    if (!r) {
                        reportError("failed to create a tag register\n");
                        return false;
                    }
                    switch ((unsigned char)in->ramAccessType) {
                    case 1:
                        if (!addNewInputInl(in, r, bits)) {
                            reportError("failed to add input operand for load\n");
                            return false;
                        }
                        break;
                    case 2:
                        if (!addNewOutputInl(in, r, bits, true)) {
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
            delete in->branchLabelTags;
            in->branchLabelTags = 0;
        }
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        bool xin = s->startType == 2;
        bool xout = s->endType == 3;
        if (xin) {
            RegPoolEntry *sticky39 = findRegisterByGlobalIndex(0x39);
            if (!addXinFor(s, sticky39)) {
                reportError("failed to add the fpu sticky flags as xin\n");
                return false;
            }
            RegPoolEntry *sticky3a = findRegisterByGlobalIndex(0x3a);
            if (!addXinFor(s, sticky3a)) {
                reportError("failed to add the div sticky flags as xin\n");
                return false;
            }
        }
        if (xin || xout) {
            for (int i = 0; i != n; i++) {
                RegPool *rp = pools[i];
                RegPoolEntry *ext = rp->externalRegister;
                if (!ext) {
                    reportError("failed to get EXT for pool %s\n", rp->poolName);
                    return false;
                }
                if (xin && !addXinFor(s, ext)) {
                    reportError("failed to add external input to %s\n", s->streamName());
                    return false;
                }
                if (xout && !addXoutFor(s, ext)) {
                    reportError("failed to add external output to %s\n", s->streamName());
                    return false;
                }
                for (RegPoolEntry *e = rp->virtualHead; e; e = e->next) {
                    if (xin && !addXinFor(s, e)) {
                        reportError("failed to add external input to %s\n", s->streamName());
                        return false;
                    }
                    // the original calls addXinFor here too
                    if (xout && !addXinFor(s, e)) {
                        reportError("failed to add external output to %s\n", s->streamName());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

Instr *addXinFor(instStream *s, RegPoolEntry *r)
{
    iInfo *info = getOpcodeInfoById(0xbe);
    if (!info) {
        reportError("addXinFor - cant get xin_hw\n");
        return 0;
    }
    Instr *in = new Instr(info);
    if (!in) {
        reportError("failed to make xin instruction\n");
        return 0;
    }
    if (in->operandCount != 1) {
        reportError("xin doesn't have just one operand\n");
        delete in;
        return 0;
    }
    int serial = nextInstructionSerial++;
    in->instructionSerial = serial;
    Operand *op = &in->operands[0];
    if (op->operandKind != 0x37) {
        reportError("xin wasn't external\n");
        delete in;
        return 0;
    }
    op->assignedRegister = r;
    op->componentMask = (1 << r->partCount) - 1;
    workTable *wt = s->getTable();
    if (!wt) {
        reportError("can't get work table\n");
        delete in;
        return 0;
    }
    if (!wt->logAddLinear_Tail(in)) {
        reportError("failed to an external input to the tail of list\n");
        return 0;
    }
    return in;
}

Instr *addXoutFor(instStream *s, RegPoolEntry *r)
{
    iInfo *info = getOpcodeInfoById(0xbf);
    if (!info) {
        reportError("addxoutFor - can't get xout_hw\n");
        return 0;
    }
    Instr *in = new Instr(info);
    if (!in) {
        reportError("failed to make xout instruction\n");
        return 0;
    }
    if (in->operandCount != 1) {
        reportError("xout doesn't have just one operand\n");
        delete in;
        return 0;
    }
    int serial = nextInstructionSerial++;
    in->instructionSerial = serial;
    Operand *op = &in->operands[0];
    if (op->operandKind != 0x37) {
        reportError("xout wasn't external\n");
        delete in;
        return 0;
    }
    op->assignedRegister = r;
    op->componentMask = (1 << r->partCount) - 1;
    workTable *wt = s->getTable();
    if (!wt) {
        reportError("can't get work table\n");
        delete in;
        return 0;
    }
    if (!wt->logAddLinear_Head(in)) {
        reportError("failed to an external output to the head of list\n");
        return 0;
    }
    return in;
}

bool addNewOperand(Instr *in, RegPoolEntry *r, int bits, int kind, int dir, int flag)
{
    if (!r) {
        reportError("add new output - passed a null rg pointer\n");
        return false;
    }
    if (!bits) {
        reportError("passed null mod bits for new output register\n");
        return false;
    }
    if (bits & (-1 << r->partCount)) {
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
        Operand *op = &in->operands[i];
        if (op->isImplicit && op->direction == dir && r == op->assignedRegister) {
            op->componentMask |= bits;
            return true;
        }
    }
    if (i == 12) {
        reportError("ran out of operand space for add new output\n");
        return false;
    }
    Operand *op = &in->operands[n];
    op->operandIndex = n;
    op->externalBlockModeFlag = 1;
    op->direction = dir;
    op->isImplicit = 1;
    op->immediateDataType = 0;
    op->pairedOperandIndexPlusOne = 0;
    op->outputLatency = flag;
    op->additionalLatency = 0;
    op->assignedRegister = r;
    op->componentMask = bits;
    op->operandKind = kind;
    in->operandCount++;
    return in->validateOperands();
}

bool addNewInput(Instr *in, RegPoolEntry *r, int bits)
{
    if (!addNewOperand(in, r, bits, 0x37, 1, false)) {
        reportError("add new input failed\n");
        return false;
    }
    return true;
}

bool addNewOutput(Instr *in, RegPoolEntry *r, int bits, int flag)
{
    if (!addNewOperand(in, r, bits, 0x37, 2, flag)) {
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
    RegPoolEntry *r = pool->findVirtualByName(name);
    if (!r) {
        r = pool->createVirtualRegister(name);
        if (!r) {
            reportError("failed to create a virtual reg\n");
            return 0;
        }
    }
    r->isVirtual = false;
    r->partCount = 4;
    return r;
}
