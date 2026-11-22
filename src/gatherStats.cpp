#include "os.h"
#include "lock_shared.h"

cls0805e2e4::cls0805e2e4()
{
}

cls0805e2e4::~cls0805e2e4()
{
}

void cls0805e2e4::noopStatisticsHook()
{
}

void cls0805e2e4::invalidateAllCounters()
{
    reverseLatencyDepth = reverseDependencyHeight = -1;
    forwardLatencyDepth = forwardDependencyHeight = -1;
    moveCount = moveDistance = -1;
    moveChainLength = -1;
    independentLatencyDepth = -1;
}

void cls0805e2e4::zeroAllCounters()
{
    reverseLatencyDepth = reverseDependencyHeight = 0;
    forwardLatencyDepth = forwardDependencyHeight = 0;
    moveCount = moveDistance = 0;
    moveChainLength = 0;
    independentLatencyDepth = 0;
}

void cls0805e2e4::copyCountersFrom(cls0805e2e4 *src)
{
    reverseLatencyDepth = src->reverseLatencyDepth;
    reverseDependencyHeight = src->reverseDependencyHeight;
    forwardLatencyDepth = src->forwardLatencyDepth;
    forwardDependencyHeight = src->forwardDependencyHeight;
    moveCount = src->moveCount;
    moveDistance = src->moveDistance;
    moveChainLength = src->moveChainLength;
    independentLatencyDepth = src->independentLatencyDepth;
}

void cls0805e2e4::mergeMaximumCounters(cls0805e2e4 *o)
{
    if (o->reverseLatencyDepth > reverseLatencyDepth) reverseLatencyDepth = o->reverseLatencyDepth;
    if (o->reverseDependencyHeight > reverseDependencyHeight) reverseDependencyHeight = o->reverseDependencyHeight;
    if (o->forwardLatencyDepth > forwardLatencyDepth) forwardLatencyDepth = o->forwardLatencyDepth;
    if (o->forwardDependencyHeight > forwardDependencyHeight) forwardDependencyHeight = o->forwardDependencyHeight;
    if (o->moveCount > moveCount) moveCount = o->moveCount;
    if (o->moveChainLength > moveChainLength) moveChainLength = o->moveChainLength;
    if (o->moveDistance > moveDistance) moveDistance = o->moveDistance;
    if (o->independentLatencyDepth > independentLatencyDepth) independentLatencyDepth = o->independentLatencyDepth;
}

char stallReasonTextBuffer[1024];  // stall reason text buffer

const char *stallReasonStr(short why)
{
    const char *s = "dunno why";
    switch (why & 0xff) {
    case 1:
        s = "instruction latency";
        break;
    case 2:
        s = "instruction thruput";
        break;
    case 3:
        s = "number of upper instructions";
        break;
    case 4:
        s = "number of lower instructions";
        break;
    case 5:
        s = "can't loop";
        break;
    case 6:
        s = "dependency chain depth";
        break;
    case 7:
        s = "dependency chain height";
        break;
    case 8:
        s = "branch instruction dependency chain height";
        break;
    case 9:
        formatIntoBuffer(stallReasonTextBuffer, "issue pressure on %s", deviceNameByIndex((char)((why & 0xff00) >> 8)));
        s = stallReasonTextBuffer;
        break;
    case 10:
        s = "circular dependency chain";
        break;
    }
    return s;
}

bool instStream::gatherStats()
{
    cls0805e2e4 *st = &schedulingStatistics;
    st->reverseLatencyDepth = st->reverseDependencyHeight = -1;
    st->forwardLatencyDepth = st->forwardDependencyHeight = -1;
    st->moveCount = st->moveDistance = -1;
    st->moveChainLength = -1;
    st->independentLatencyDepth = -1;
    int physSum = 0;
    int maxLat = 0;
    branchForwardLatencyDepth = -1;
    unk_2b6 = -1;
    statCounts *sc = &stats;
    for (int i = 0; i != 9; i++)
        sc->resourceInstructionCounts[i] = 0;
    savedStallReason = 0;
    currentStallReason = 0;
    workTable *wt = getTable();
    if (!wt) {
        reportError("failed to gather stats - no get work table\n");
        return false;
    }

    Instr *in;
    for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        cls0805e2e4 *is = &in->schedulingStatistics;
        is->reverseLatencyDepth = is->reverseDependencyHeight = 0;
        is->forwardLatencyDepth = is->forwardDependencyHeight = 0;
        is->moveCount = is->moveDistance = 0;
        is->moveChainLength = 0;
        is->independentLatencyDepth = 0;
        int logged = 0;
        if (in) {
            for (int j = 0; j != 9; j++) {
                bool b = (in->opcodeResourceMask >> j) & 1;
                if (b)
                    sc->resourceInstructionCounts[j] += 2;
            }
            logged = 1;
        }
        if ((unsigned char)logged == 0) {
            reportError("failed to log\n");
            return false;
        }
    }

    for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = (isRegisterOperand(op, 1) && op->operandStatus != 2) ? op->assignedRegister : 0;
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    if ((op->componentMask >> p) & 1) {
                        for (ol_s *ol = op->parts[p].operandLocations.head; ol; ol = ol->next) {
                            if (!ol->instruction) {
                                reportError("failed to get setting instruction!\n");
                                return false;
                            }
                            if (iOwn(ol->instruction)) {
                                Operand *so = operandAtLocation(ol);
                                if (!so) {
                                    reportError("failed to get setting operand for instruction\n");
                                    return false;
                                }
                                int l = so->outputLatency + op->additionalLatency;
                                if (maxLat < l)
                                    maxLat = l;
                            }
                        }
                    }
                }
            }
        }
    }

    for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r;
            if (isRegisterOperand(op, 1) && op->operandStatus != 2)
                r = op->assignedRegister;
            else
                r = 0;
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    if (((op->componentMask & ~op->externalPartMask) >> p) & 1) {
                        if (op->parts[p].operandLocations.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->parts[p].operandLocations.head->instruction;
                        Operand *so = operandAtLocation(op->parts[p].operandLocations.head);
                        int d = src->schedulingStatistics.forwardLatencyDepth + so->outputLatency + op->additionalLatency;
                        int h = src->schedulingStatistics.forwardDependencyHeight + 1;
                        if (d > in->schedulingStatistics.forwardLatencyDepth)
                            in->schedulingStatistics.forwardLatencyDepth = d;
                        if (h > in->schedulingStatistics.forwardDependencyHeight)
                            in->schedulingStatistics.forwardDependencyHeight = h;
                    }
                }
            }
        }
    }

    for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = (isRegisterOperand(op, 1) && op->operandStatus != 2) ? op->assignedRegister : 0;
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    if (((op->componentMask & ~op->externalPartMask) >> p) & 1) {
                        if (op->parts[p].operandLocations.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->parts[p].operandLocations.head->instruction;
                        Operand *so = operandAtLocation(op->parts[p].operandLocations.head);
                        int d = src->schedulingStatistics.independentLatencyDepth + so->outputLatency + op->additionalLatency;
                        if (d > in->schedulingStatistics.independentLatencyDepth)
                            in->schedulingStatistics.independentLatencyDepth = d;
                    }
                }
            }
        }
    }

    wtInstrList *lst = &wt->linearInstructions;
    for (in = lst->tail; in; in = in->linearLink.previousInstruction) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r;
            if (isRegisterOperand(op, 1) && op->operandStatus != 2)
                r = op->assignedRegister;
            else
                r = 0;
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    if (((op->componentMask & ~op->externalPartMask) >> p) & 1) {
                        if (op->parts[p].operandLocations.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->parts[p].operandLocations.head->instruction;
                        Operand *so = operandAtLocation(op->parts[p].operandLocations.head);
                        int d = in->schedulingStatistics.reverseLatencyDepth + so->outputLatency + op->additionalLatency;
                        int h = in->schedulingStatistics.reverseDependencyHeight + 1;
                        if (d > src->schedulingStatistics.reverseLatencyDepth)
                            src->schedulingStatistics.reverseLatencyDepth = d;
                        if (h > src->schedulingStatistics.reverseDependencyHeight)
                            src->schedulingStatistics.reverseDependencyHeight = h;
                    }
                }
            }
        }
    }

    for (in = lst->tail; in; ) {
        iLink *link = &in->linearLink;
        Instr *m = link->previousInstruction;
        for (; m; m = m->linearLink.previousInstruction) {
            if (hasDirectOperandDependency(m, in)) {
                m->schedulingStatistics.moveCount++;
                m->schedulingStatistics.moveDistance += in->schedulingStatistics.moveDistance + 1;
                m->schedulingStatistics.moveChainLength = in->schedulingStatistics.moveChainLength + 1;
            }
        }
        // Keep the original pointer-word load while reading a complete
        // address on either 32- or 64-bit targets.
        m = (Instr *)*(VclPtrWord *)&link->previousInstruction;
        in = m;
    }

    int upper = 0;
    int lower = 0;
    int thru = 0;
    int best = 0;
    int both = 0;
    for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        if (in->branchFlags)
            branchForwardLatencyDepth = in->schedulingStatistics.forwardLatencyDepth;
        char c = in->permittedPipelineMask;
        if (c == 3)
            both++;
        else if (c & 2)
            lower++;
        else
            upper++;
        schedulingStatistics.mergeMaximumCounters(&in->schedulingStatistics);
    }

    int maxIssue = 0;
    for (in = wt->linearInstructions.head; in; in = in->linearLink.nextInstruction) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = registerOperand(op, 2);
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    int nBoth = 0;
                    int n1 = 0;
                    int nOther = 0;
                    if ((op->componentMask >> p) & 1) {
                        for (ol_s *ol = op->parts[p].operandLocations.firstInternalLocation(); ol; ol = ol->nextInternalLocation()) {
                            char c = ol->instruction->permittedPipelineMask;
                            if (c == 3)
                                nBoth++;
                            else if (c == 1)
                                n1++;
                            else
                                nOther++;
                        }
                    }
                    int hi = n1;
                    int lo = nOther;
                    if (n1 < nOther) {
                        hi = nOther;
                        lo = n1;
                    }
                    int rem = nBoth - (hi - lo);
                    if (rem > 0)
                        hi += (rem + 1) >> 1;
                    if (hi)
                        hi--;
                    hi += op->outputLatency;
                    if (hi > maxIssue)
                        maxIssue = hi;
                }
            }
        }
    }

    {
        int u = upper;
        int l = lower;
        for (; both; both--) {
            if (u < l)
                u++;
            else
                l++;
        }
        if (u > best) {
            currentStallReason = 3;
            best = u;
        }
        if (l > best) {
            currentStallReason = 4;
            best = l;
        }
    }
    if (maxIssue > best) {
        best = maxIssue;
        currentStallReason = 1;
    }

    int depth = schedulingStatistics.independentLatencyDepth;
    if (depth < 0)
        depth = 0;
    for (regUse *ru = wt->registerUses.head; ru; ru = ru->next) {
        RegPoolEntry *r = ru->registerEntry;
        if (!r) {
            reportError("regref had no reg\n");
            return false;
        }
        for (int p = 0; p != r->partCount; p++) {
            if ((ru->outputMask >> p) & 1) {
                regUseSlot *s = &ru->slots[p];
                int h = s->throughputSums[0];
                if (h > thru)
                    thru = h;
                int d = h - s->throughputSums[3];
                if (d > depth)
                    depth = d;
                physSum += s->throughputSums[1];
            }
        }
    }
    if (thru > best) {
        currentStallReason = 2;
        best = thru;
    }
    if (maxLat > best) {
        currentStallReason = 1;
        best = maxLat;
    }
    for (int i = 0; i != 9; i++) {
        if (i < 0 || i > 8) {
            reportError("bad stat\n");
            exitAfterCleanup(-1);
        }
        int n = (sc->resourceInstructionCounts[i] + 1) >> 1;
        if (n > best) {
            best = n;
            currentStallReason = (i << 8) | 9;
        }
    }
    if (physSum < best * 4)
        physSum = best * 4;
    if (physSum < 64)
        physSum = 64;
    if (physSum > 0x4000)
        physSum = 0x4000;
    if (wt->codeModeValue == 3 && physSum < wt->getPhysicalLen())
        physSum = wt->getPhysicalLen();
    physicalInstructionBudget = physSum;
    if (canLoop()) {
        savedStallReason = currentStallReason;
        logicalLength = best;
        if (best <= 1)
            logicalLength = 2;
    } else {
        savedStallReason = 5;
        logicalLength = 0;
    }
    if (schedulingStatistics.forwardLatencyDepth >= best) {
        currentStallReason = 6;
        best = schedulingStatistics.forwardLatencyDepth + 1;
    }
    if (schedulingStatistics.reverseLatencyDepth >= best) {
        currentStallReason = 7;
        best = schedulingStatistics.reverseLatencyDepth + 1;
    }
    Instr *br = wt->getBranch();
    if (br) {
        int h = br->schedulingStatistics.forwardLatencyDepth + 2;
        if (best <= h) {
            currentStallReason = 8;
            best = h;
        }
    }
    dependencyLength = best;
    wt->streamDependencyLength = dependencyLength;
    wt->streamLogicalLength = logicalLength;
    wt->logicalInstructionLength = depth;
    return true;
}

static inline int clampDepLen(int len)
{
    if (len <= 0)
        len = 0;
    return len;
}

int instStream::getRecordedDependencyLength()
{
    int len = dependencyLength;
    if (len <= 0)
        len = 0;
    return len;
}

int instStream::getRecordedLogicalLength()
{
    int len = logicalLength;
    if (len <= 0)
        len = 0;
    return len;
}

int instStream::getOrGuessDepLength()
{
    workTable *wt = getTable();
    if (!wt) {
        reportError("getOrGuess - no work table\n");
        exitAfterCleanup(-1);
    }
    int len = 0;
    switch (wt->codeModeValue) {
    case 1:
        len = clampDepLen(dependencyLength);
        break;
    case 3:
        len = wt->getRawOrSyntheticLogicalLength();
        break;
    }
    return len;
}

int instStream::getClampedDependencyLength()
{
    workTable *wt = getTable();
    if (!wt) {
        reportError("getOrGuess - no work table\n");
        exitAfterCleanup(-1);
    }
    int len = 0;
    switch (wt->codeModeValue) {
    case 1:
    case 3:
        len = clampDepLen(dependencyLength);
        break;
    }
    return len;
}

int instStream::getOrGuessLogicalLength()
{
    workTable *wt = workTablePtr;
    if (!wt)
        wt = getWorkTable();
    if (!wt) {
        reportError("getOrGuess - no work table\n");
        exitAfterCleanup(-1);
    }
    return wt->getLogicalLen();
}

void statCounts::clear()
{
    for (int i = 0; i != 9; i++)
        resourceInstructionCounts[i] = 0;
}

bool statCounts::add(Instr *in)
{
    if (!in)
        return false;
    for (int i = 0; i != 9; i++) {
        bool b = (in->opcodeResourceMask >> i) & 1;
        if (b)
            resourceInstructionCounts[i] += 2;
    }
    return true;
}

int statCounts::get(int i)
{
    if (i < 0 || i > 8) {
        reportError("bad stat\n");
        exitAfterCleanup(-1);
    }
    return (resourceInstructionCounts[i] + 1) >> 1;
}

