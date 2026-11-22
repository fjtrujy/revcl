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

static inline int iHolderMatches(iHolder *h, ol_s *ol)
{
    if (!ol || !h->instruction)
        return 0;
    int r = 0;
    Instr *i = ol->instruction;
    if (h->instruction == i) {
        int idx = ol->operandIndex;
        if (h->operandIndex == idx)
            r = 1;
    }
    return r;
}

int iHolder::matchesOperandLocation(ol_s *ol)
{
    return iHolderMatches(this, ol);
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

bool iHolder::catInit(ol_s *ol)
{
    if (!ol) {
        reportError("iHolder - passed a null ol\n");
        return false;
    }
    if (!instruction) {
        Instr *in = ol->instruction;
        if (!in) {
            reportError("iHolder:: init called, but ol_s not inst\n");
            return false;
        }
        instruction = in;
        operandIndex = ol->operandIndex;
        componentMask = 0;
    } else if (instruction != ol->instruction || operandIndex != ol->operandIndex) {
        reportError("iHolder::catInit - mismatch adding to an existing object\n");
        return false;
    }
    componentMask |= 1 << ol->componentIndex;
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
    for (int i = 0; i != 3; i++)
        stallByType[i] = 0;
}

bool infoList::addStall(ol_s *a, ol_s *b, int x, int val)
{
    stallNode *p = (stallNode *)head;
    if (!a || !b) {
        reportError("infoList:: addStall - null input ptr\n");
        return false;
    }
    if (val <= 0) {
        reportError("infoList:: add stall - val <=0\n");
        return false;
    }
    for (; p; p = (stallNode *)p->next)
        if ((char)iHolderMatches(&p->setter, a) && (char)iHolderMatches(&p->user, b))
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
    if (!p->catStall(a, b, x, val)) {
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
    for (int i = 0; i != 3; i++)
        stallByType[i] = 0;
}

stallNode::~stallNode()
{
}

void stallNode::dump()
{
    char buf[512];
    char *p = buf;
    p += formatIntoBuffer(p, "STALL");
    int max = 0;
    for (int i = 0; i != 3; i++) {
        unsigned short v = stallByType[i];
        if (v > max)
            max = v;
        if (v)
            p += formatIntoBuffer(p, "_%s(%d)", stallTypeNames[i], v);
    }
    p += formatIntoBuffer(p, " maxVal %d ", max);
    if (isLocal)
        p += formatIntoBuffer(p, ": local");
    else
        p += formatIntoBuffer(p, " inter-stream");
    *p = 0;
    reportInfo("%s\n", buf);
    reportInfo("    setter is : ");
    setter.dump();
    reportInfo("    user is   : ");
    user.dump();
}

bool stallNode::catStall(ol_s *a, ol_s *b, int type, int val)
{
    if (!a || !b) {
        reportWarning("stallNode - init called with a null input\n");
        return false;
    }
    if (!setter.catInit(a)) {
        reportError("stallInit - failed to add in a source loc\n");
        return false;
    }
    Instr *ia = a->instruction;
    Instr *ib = b->instruction;
    if (!ia || !ib) {
        reportError("can't determine - at least one of instructions in stall pair\n");
        return false;
    }
    instStream *sa = ia->getOwningStream();
    instStream *sb = ib->getOwningStream();
    sa->participatesInStall = 1;
    sb->participatesInStall = 1;
    if (!sa || !sb) {
        reportError("can't determine owner ist for one of inputs to stall\n");
        return false;
    }
    isLocal = sa->streamId == sb->streamId;
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
    workTable *wt = use->workTablePtr;
    bool haveTable = wt != 0;
    if (!haveTable) {
        reportError("work table not set in %s\n", use->streamName());
        exitAfterCleanup(-1);
    }
    if (!haveTable)
        wt = use->workTablePtr;
    regUse *ru;
    if (wt && (ru = wt->registerUses.findByRegister(reg))) {
        reportInfo("register %s is used before set at instruction(s) :\n",
                      reg->registerName());
        for (int i = 0; i != reg->partCount; i++) {
            if ((slotMask >> i) & 1) {
                for (ol_s *p = ru->slots[i].inputLocations.head; p; p = p->next) {
                    Instr *inst = p->instruction;
                    int bit = p->componentIndex;
                    int opnd = p->operandIndex;
                    bit = 1 << bit;
                    int j;
                    for (j = 0; j < n &&
                         (list[j].inst != inst || list[j].opnd != opnd); j++)
                        ;
                    ubsEntry *e = 0;
                    if (j == n) {
                        if (n < 64) {
                            e = &list[n];
                            e->mask = 0;
                            n++;
                        }
                    } else
                        e = &list[j];
                    e->inst = inst;
                    e->mask |= bit;
                    e->opnd = opnd;
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
    ubsNode *p = (ubsNode *)head;
    if (!entry || !use || !reg || !bits) {
        reportError("ubsList: addUBS - passed bad args\n");
        return false;
    }
    for (; p; p = (ubsNode *)p->next)
        if (reg == p->reg && entry == p->entryStream && use == p->useStream)
            break;
    if (!p) {
        p = new ubsNode;
        if (!p) {
            reportError("infoList::addStall - failed to get new ubs\n");
            return false;
        }
        count++;
        p->next = head;
        head = p;
    }
    if (!p->catInit(entry, use, reg, bits)) {
        reportError("failed to cat init a used-before-set operand\n");
        return false;
    }
    return true;
}
