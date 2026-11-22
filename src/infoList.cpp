// iHolder and infoList, CIE group 3.
#include "infoList.h"
#include "Instr.h"
#include "instStream.h"
#include "RegPool.h"
#include "workTable.h"
#include "ubsList.h"
#include "os.h"

char *stallTypeNames[3] = { "LATENCY", "THRUPUT", "RFWRITE" };

inline iHolder::iHolder()
{
    instruction = 0;
    operandIndex = 0;
    componentMask = 0;
}

static inline int iHolderMatches(iHolder *holder, ol_s *location)
{
    if (!location || !holder->instruction)
        return 0;
    int matches = 0;
    Instr *i = location->instruction;
    if (holder->instruction == i) {
        int idx = location->operandIndex;
        if (holder->operandIndex == idx)
            matches = 1;
    }
    return matches;
}

int iHolder::matchesOperandLocation(ol_s *location)
{
    return iHolderMatches(this, location);
}

void iHolder::dumpOperandLocation()
{
    if (instruction)
        instruction->leDump(operandIndex, componentMask);
    else
        reportMessage(" UNSET_INST\n");
}

inline void iHolder::dump()
{
    if (instruction)
        instruction->leDump(operandIndex, componentMask);
    else
        reportMessage(" UNSET_INST\n");
}

bool iHolder::catInit(ol_s *location)
{
    if (!location) {
        reportError("iHolder - passed a null ol\n");
        return false;
    }
    if (!instruction) {
        Instr *in = location->instruction;
        if (!in) {
            reportError("iHolder:: init called, but ol_s not inst\n");
            return false;
        }
        instruction = in;
        operandIndex = location->operandIndex;
        componentMask = 0;
    } else if (instruction != location->instruction || operandIndex != location->operandIndex) {
        reportError("iHolder::catInit - mismatch adding to an existing object\n");
        return false;
    }
    componentMask |= 1 << location->componentIndex;
    return true;
}

infoList::infoList()
{
    head = 0;
    count = 0;
}

infoList::~infoList()
{
    infoNode *p = head;
    while (p) {
        infoNode *n = p->next;
        delete p;
        p = n;
    }
    head = 0;
    count = 0;
}

bool infoList::clear()
{
    infoNode *p = head;
    while (p) {
        infoNode *n = p->next;
        delete p;
        p = n;
    }
    head = 0;
    count = 0;
    return true;
}

void infoList::dumpEntries()
{
    for (infoNode *p = head; p; p = p->next)
        p->dump();
}

inline infoNode::infoNode(InitTag)
{
    next = 0;
}

inline stallNode::stallNode(InitTag) : infoNode(infoNode::INIT)
{
    for (int stallTypeIndex = 0; stallTypeIndex != 3; stallTypeIndex++)
        stallByType[stallTypeIndex] = 0;
}

bool infoList::addStall(ol_s *location, ol_s *b, int x, int val)
{
    stallNode *p = (stallNode *)head;
    if (!location || !b) {
        reportError("infoList:: addStall - null input ptr\n");
        return false;
    }
    if (val <= 0) {
        reportError("infoList:: add stall - val <=0\n");
        return false;
    }
    for (; p; p = (stallNode *)p->next)
        if ((char)iHolderMatches(&p->setter, location) && (char)iHolderMatches(&p->user, b))
            break;
    if (!p) {
        p = new stallNode(stallNode::INIT);
        if (!p) {
            reportError("infoList::addStall - failed to get new stall\n");
            return false;
        }
        p->next = head;
        count++;
        head = p;
    }
    if (!p->catStall(location, b, x, val)) {
        reportError("infoList::addStall - failed to cat a stall\n");
        return false;
    }
    return true;
}

infoNode::infoNode()
{
    next = 0;
}

infoNode::~infoNode()
{
}

stallNode::stallNode() : infoNode(infoNode::INIT)
{
    for (int stallTypeIndex = 0; stallTypeIndex != 3; stallTypeIndex++)
        stallByType[stallTypeIndex] = 0;
}

stallNode::~stallNode()
{
}

void stallNode::dump()
{
    char buf[512];
    char *outputCursor = buf;
    outputCursor += formatIntoBuffer(outputCursor, "STALL");
    int max = 0;
    for (int stallTypeIndex = 0; stallTypeIndex != 3; stallTypeIndex++) {
        unsigned short stallDuration = stallByType[stallTypeIndex];
        if (stallDuration > max)
            max = stallDuration;
        if (stallDuration)
            outputCursor += formatIntoBuffer(outputCursor, "_%s(%d)", stallTypeNames[stallTypeIndex], stallDuration);
    }
    outputCursor += formatIntoBuffer(outputCursor, " maxVal %d ", max);
    if (isLocal)
        outputCursor += formatIntoBuffer(outputCursor, ": local");
    else
        outputCursor += formatIntoBuffer(outputCursor, " inter-stream");
    *outputCursor = 0;
    reportInfo("%s\n", buf);
    reportInfo("    setter is : ");
    setter.dump();
    reportInfo("    user is   : ");
    user.dump();
}

bool stallNode::catStall(ol_s *location, ol_s *b, int type, int val)
{
    if (!location || !b) {
        reportWarning("stallNode - init called with a null input\n");
        return false;
    }
    if (!setter.catInit(location)) {
        reportError("stallInit - failed to add in a source loc\n");
        return false;
    }
    Instr *ia = location->instruction;
    Instr *ib = b->instruction;
    if (!ia || !ib) {
        reportError("can't determine - at least one of instructions in stall pair\n");
        return false;
    }
    instStream *stream = ia->getOwningStream();
    instStream *sb = ib->getOwningStream();
    stream->participatesInStall = 1;
    sb->participatesInStall = 1;
    if (!stream || !sb) {
        reportError("can't determine owner ist for one of inputs to stall\n");
        return false;
    }
    isLocal = stream->streamId == sb->streamId;
    if (!user.catInit(b)) {
        reportError("stallInit - failed to add in a user loc\n");
        return false;
    }
    if (type < 0 || type > 2) {
        reportError("bad stall type\n");
        return false;
    }
    if (val > stallByType[type])
        stallByType[type] = val;
    return true;
}

ubsNode::ubsNode() : infoNode(infoNode::INIT)
{
    reg = 0;
    slotMask = 0;
    entryStream = 0;
    useStream = 0;
}

ubsNode::~ubsNode()
{
}

void ubsNode::dump()
{
    struct ubsEntry {
        Instr *inst;
        int opnd;
        int mask;
    } list[64];
    int n;
    reportInfo("Used before set : ");
    n = 0;
    instStream *use = useStream;
    workTable *scheduleTable = use->workTablePtr;
    bool haveTable = scheduleTable != 0;
    if (!haveTable) {
        reportError("work table not set in %s\n", use->streamName());
        exitAfterCleanup(-1);
    }
    if (!haveTable)
        scheduleTable = use->workTablePtr;
    regUse *registerUse;
    if (scheduleTable && (registerUse = scheduleTable->registerUses.findByRegister(reg))) {
        reportInfo("register %s is used before set at instruction(s) :\n",
                      reg->registerName());
        for (int partIndex = 0; partIndex != reg->partCount; partIndex++) {
            if ((slotMask >> partIndex) & 1) {
                for (ol_s *location = registerUse->slots[partIndex].inputLocations.head; location; location = location->next) {
                    Instr *inst = location->instruction;
                    int bit = location->componentIndex;
                    int opnd = location->operandIndex;
                    bit = 1 << bit;
                    int j;
                    for (j = 0; j < n &&
                         (list[j].inst != inst || list[j].opnd != opnd); j++)
                        ;
                    ubsEntry *usedBeforeSetEntry = 0;
                    if (j == n) {
                        if (n < 64) {
                            usedBeforeSetEntry = &list[n];
                            usedBeforeSetEntry->mask = 0;
                            n++;
                        }
                    } else
                        usedBeforeSetEntry = &list[j];
                    usedBeforeSetEntry->inst = inst;
                    usedBeforeSetEntry->mask |= bit;
                    usedBeforeSetEntry->opnd = opnd;
                }
            }
        }
        for (int k = 0; k != n; k++) {
            reportInfo("   ");
            list[k].inst->leDump(list[k].opnd, list[k].mask);
        }
    } else {
        reportInfo("register %s is used before set at \n",
                      reg->registerName(), useStream->streamName());
    }
}

bool ubsNode::catInit(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits)
{
    if (!entry) {
        reportError("catInit - entry point not set\n");
        return false;
    }
    if (!use) {
        reportError("catInit - use point not set\n");
        return false;
    }
    if (!reg) {
        reportError("ubs::catInit - register not set\n");
        return false;
    }
    if (!bits) {
        reportError("ubs::catInit - bits was null in cat init\n");
        return false;
    }
    entryStream = entry;
    useStream = use;
    this->reg = reg;
    slotMask |= bits;
    return true;
}

bool ubsList::addUBS(instStream *entry, instStream *use, RegPoolEntry *reg, unsigned char bits)
{
    ubsNode *usedBeforeSetNode = (ubsNode *)head;
    if (!entry || !use || !reg || !bits) {
        reportError("ubsList: addUBS - passed bad args\n");
        return false;
    }
    for (; usedBeforeSetNode; usedBeforeSetNode = (ubsNode *)usedBeforeSetNode->next)
        if (reg == usedBeforeSetNode->reg && entry == usedBeforeSetNode->entryStream && use == usedBeforeSetNode->useStream)
            break;
    if (!usedBeforeSetNode) {
        usedBeforeSetNode = new ubsNode;
        if (!usedBeforeSetNode) {
            reportError("infoList::addStall - failed to get new ubs\n");
            return false;
        }
        count++;
        usedBeforeSetNode->next = head;
        head = usedBeforeSetNode;
    }
    if (!usedBeforeSetNode->catInit(entry, use, reg, bits)) {
        reportError("failed to cat init a used-before-set operand\n");
        return false;
    }
    return true;
}
