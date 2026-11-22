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
    Instr *locationInstruction = location->instruction;
    if (holder->instruction == locationInstruction) {
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
        instruction->dumpInstructionContext(operandIndex, componentMask);
    else
        reportMessage(" UNSET_INST\n");
}

inline void iHolder::dump()
{
    if (instruction)
        instruction->dumpInstructionContext(operandIndex, componentMask);
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
        Instr *locationInstruction = location->instruction;
        if (!locationInstruction) {
            reportError("iHolder:: init called, but ol_s not inst\n");
            return false;
        }
        instruction = locationInstruction;
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
    infoNode *currentNode = head;
    while (currentNode) {
        infoNode *nextNode = currentNode->next;
        delete currentNode;
        currentNode = nextNode;
    }
    head = 0;
    count = 0;
}

bool infoList::clear()
{
    infoNode *currentNode = head;
    while (currentNode) {
        infoNode *nextNode = currentNode->next;
        delete currentNode;
        currentNode = nextNode;
    }
    head = 0;
    count = 0;
    return true;
}

void infoList::dumpEntries()
{
    for (infoNode *currentNode = head; currentNode; currentNode = currentNode->next)
        currentNode->dump();
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

bool infoList::addStall(ol_s *location, ol_s *userLocation, int stallType, int stallDuration)
{
    stallNode *currentStall = (stallNode *)head;
    if (!location || !userLocation) {
        reportError("infoList:: addStall - null input ptr\n");
        return false;
    }
    if (stallDuration <= 0) {
        reportError("infoList:: add stall - val <=0\n");
        return false;
    }
    for (; currentStall; currentStall = (stallNode *)currentStall->next)
        if ((char)iHolderMatches(&currentStall->setter, location) && (char)iHolderMatches(&currentStall->user, userLocation))
            break;
    if (!currentStall) {
        currentStall = new stallNode(stallNode::INIT);
        if (!currentStall) {
            reportError("infoList::addStall - failed to get new stall\n");
            return false;
        }
        currentStall->next = head;
        count++;
        head = currentStall;
    }
    if (!currentStall->catStall(location, userLocation, stallType, stallDuration)) {
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

bool stallNode::catStall(ol_s *location, ol_s *userLocation, int type, int stallDuration)
{
    if (!location || !userLocation) {
        reportWarning("stallNode - init called with a null input\n");
        return false;
    }
    if (!setter.catInit(location)) {
        reportError("stallInit - failed to add in a source loc\n");
        return false;
    }
    Instr *setterInstruction = location->instruction;
    Instr *userInstruction = userLocation->instruction;
    if (!setterInstruction || !userInstruction) {
        reportError("can't determine - at least one of instructions in stall pair\n");
        return false;
    }
    instStream *stream = setterInstruction->getOwningStream();
    instStream *userStream = userInstruction->getOwningStream();
    stream->participatesInStall = 1;
    userStream->participatesInStall = 1;
    if (!stream || !userStream) {
        reportError("can't determine owner ist for one of inputs to stall\n");
        return false;
    }
    isLocal = stream->streamId == userStream->streamId;
    if (!user.catInit(userLocation)) {
        reportError("stallInit - failed to add in a user loc\n");
        return false;
    }
    if (type < 0 || type > 2) {
        reportError("bad stall type\n");
        return false;
    }
    if (stallDuration > stallByType[type])
        stallByType[type] = stallDuration;
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
    int usedBeforeSetCount;
    reportInfo("Used before set : ");
    usedBeforeSetCount = 0;
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
                    int useIndex;
                    for (useIndex = 0; useIndex < usedBeforeSetCount &&
                         (list[useIndex].inst != inst || list[useIndex].opnd != opnd); useIndex++)
                        ;
                    ubsEntry *usedBeforeSetEntry = 0;
                    if (useIndex == usedBeforeSetCount) {
                        if (usedBeforeSetCount < 64) {
                            usedBeforeSetEntry = &list[usedBeforeSetCount];
                            usedBeforeSetEntry->mask = 0;
                            usedBeforeSetCount++;
                        }
                    } else
                        usedBeforeSetEntry = &list[useIndex];
                    usedBeforeSetEntry->inst = inst;
                    usedBeforeSetEntry->mask |= bit;
                    usedBeforeSetEntry->opnd = opnd;
                }
            }
        }
        for (int remainingUseIndex = 0; remainingUseIndex != usedBeforeSetCount; remainingUseIndex++) {
            reportInfo("   ");
            list[remainingUseIndex].inst->dumpInstructionContext(list[remainingUseIndex].opnd, list[remainingUseIndex].mask);
        }
    } else {
        reportInfo("register %s is used before set at \n",
                      reg->registerName(), useStream->streamName());
    }
}

bool ubsNode::catInit(instStream *entry, instStream *use, RegPoolEntry *registerEntry, unsigned char bits)
{
    if (!entry) {
        reportError("catInit - entry point not set\n");
        return false;
    }
    if (!use) {
        reportError("catInit - use point not set\n");
        return false;
    }
    if (!registerEntry) {
        reportError("ubs::catInit - register not set\n");
        return false;
    }
    if (!bits) {
        reportError("ubs::catInit - bits was null in cat init\n");
        return false;
    }
    entryStream = entry;
    useStream = use;
    this->reg = registerEntry;
    slotMask |= bits;
    return true;
}

bool ubsList::addUBS(instStream *entry, instStream *use, RegPoolEntry *registerEntry, unsigned char bits)
{
    ubsNode *usedBeforeSetNode = (ubsNode *)head;
    if (!entry || !use || !registerEntry || !bits) {
        reportError("ubsList: addUBS - passed bad args\n");
        return false;
    }
    for (; usedBeforeSetNode; usedBeforeSetNode = (ubsNode *)usedBeforeSetNode->next)
        if (registerEntry == usedBeforeSetNode->reg && entry == usedBeforeSetNode->entryStream && use == usedBeforeSetNode->useStream)
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
    if (!usedBeforeSetNode->catInit(entry, use, registerEntry, bits)) {
        reportError("failed to cat init a used-before-set operand\n");
        return false;
    }
    return true;
}
