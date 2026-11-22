#include "os.h"
#include "lock_shared.h"

cls0805e2e4::cls0805e2e4()
{
}

cls0805e2e4::~cls0805e2e4()
{
}

void cls0805e2e4::func_0805e2fc()
{
}

void cls0805e2e4::invalidateAllCounters()
{
    unk_00 = unk_02 = -1;
    unk_04 = unk_06 = -1;
    unk_08 = unk_0c = -1;
    unk_10 = -1;
    unk_14 = -1;
}

void cls0805e2e4::zeroAllCounters()
{
    unk_00 = unk_02 = 0;
    unk_04 = unk_06 = 0;
    unk_08 = unk_0c = 0;
    unk_10 = 0;
    unk_14 = 0;
}

void cls0805e2e4::copyCountersFrom(cls0805e2e4 *src)
{
    unk_00 = src->unk_00;
    unk_02 = src->unk_02;
    unk_04 = src->unk_04;
    unk_06 = src->unk_06;
    unk_08 = src->unk_08;
    unk_0c = src->unk_0c;
    unk_10 = src->unk_10;
    unk_14 = src->unk_14;
}

void cls0805e2e4::mergeMaximumCounters(cls0805e2e4 *o)
{
    if (o->unk_00 > unk_00) unk_00 = o->unk_00;
    if (o->unk_02 > unk_02) unk_02 = o->unk_02;
    if (o->unk_04 > unk_04) unk_04 = o->unk_04;
    if (o->unk_06 > unk_06) unk_06 = o->unk_06;
    if (o->unk_08 > unk_08) unk_08 = o->unk_08;
    if (o->unk_10 > unk_10) unk_10 = o->unk_10;
    if (o->unk_0c > unk_0c) unk_0c = o->unk_0c;
    if (o->unk_14 > unk_14) unk_14 = o->unk_14;
}

char g_080de2a0[1024];  // stall reason text buffer

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
        formatIntoBuffer(g_080de2a0, "issue pressure on %s", func_0806c6a0((char)((why & 0xff00) >> 8)));
        s = g_080de2a0;
        break;
    case 10:
        s = "circular dependency chain";
        break;
    }
    return s;
}

bool instStream::gatherStats()
{
    cls0805e2e4 *st = &unk_278;
    st->unk_00 = st->unk_02 = -1;
    st->unk_04 = st->unk_06 = -1;
    st->unk_08 = st->unk_0c = -1;
    st->unk_10 = -1;
    st->unk_14 = -1;
    int physSum = 0;
    int maxLat = 0;
    unk_2b4 = -1;
    unk_2b6 = -1;
    statCounts *sc = &stats;
    for (int i = 0; i != 9; i++)
        sc->unk_00[i] = 0;
    unk_2b8 = 0;
    unk_2ba = 0;
    workTable *wt = getTable();
    if (!wt) {
        reportError("failed to gather stats - no get work table\n");
        return false;
    }

    Instr *in;
    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        cls0805e2e4 *is = &in->unk_13f0;
        is->unk_00 = is->unk_02 = 0;
        is->unk_04 = is->unk_06 = 0;
        is->unk_08 = is->unk_0c = 0;
        is->unk_10 = 0;
        is->unk_14 = 0;
        int logged = 0;
        if (in) {
            for (int j = 0; j != 9; j++) {
                bool b = (in->opcodeResourceMask >> j) & 1;
                if (b)
                    sc->unk_00[j] += 2;
            }
            logged = 1;
        }
        if ((unsigned char)logged == 0) {
            reportError("failed to log\n");
            return false;
        }
    }

    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = (isRegisterOperand(op, 1) && op->operandStatus != 2) ? op->assignedRegister : 0;
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    if ((op->componentMask >> p) & 1) {
                        for (ol_s *ol = op->parts[p].unk_54.head; ol; ol = ol->next) {
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

    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
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
                        if (op->parts[p].unk_54.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->parts[p].unk_54.head->instruction;
                        Operand *so = operandAtLocation(op->parts[p].unk_54.head);
                        int d = src->unk_13f0.unk_04 + so->outputLatency + op->additionalLatency;
                        int h = src->unk_13f0.unk_06 + 1;
                        if (d > in->unk_13f0.unk_04)
                            in->unk_13f0.unk_04 = d;
                        if (h > in->unk_13f0.unk_06)
                            in->unk_13f0.unk_06 = h;
                    }
                }
            }
        }
    }

    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = (isRegisterOperand(op, 1) && op->operandStatus != 2) ? op->assignedRegister : 0;
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    if (((op->componentMask & ~op->externalPartMask) >> p) & 1) {
                        if (op->parts[p].unk_54.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->parts[p].unk_54.head->instruction;
                        Operand *so = operandAtLocation(op->parts[p].unk_54.head);
                        int d = src->unk_13f0.unk_14 + so->outputLatency + op->additionalLatency;
                        if (d > in->unk_13f0.unk_14)
                            in->unk_13f0.unk_14 = d;
                    }
                }
            }
        }
    }

    wtInstrList *lst = &wt->linearInstructions;
    for (in = lst->tail; in; in = in->unk_1c.unk_04) {
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
                        if (op->parts[p].unk_54.count != 1) {
                            reportError("found ol with!=1 entries\n");
                            return false;
                        }
                        Instr *src = op->parts[p].unk_54.head->instruction;
                        Operand *so = operandAtLocation(op->parts[p].unk_54.head);
                        int d = in->unk_13f0.unk_00 + so->outputLatency + op->additionalLatency;
                        int h = in->unk_13f0.unk_02 + 1;
                        if (d > src->unk_13f0.unk_00)
                            src->unk_13f0.unk_00 = d;
                        if (h > src->unk_13f0.unk_02)
                            src->unk_13f0.unk_02 = h;
                    }
                }
            }
        }
    }

    for (in = lst->tail; in; ) {
        iLink *link = &in->unk_1c;
        Instr *m = link->unk_04;
        for (; m; m = m->unk_1c.unk_04) {
            if (func_080543bc(m, in)) {
                m->unk_13f0.unk_08++;
                m->unk_13f0.unk_0c += in->unk_13f0.unk_0c + 1;
                m->unk_13f0.unk_10 = in->unk_13f0.unk_10 + 1;
            }
        }
        // Keep the original pointer-word load while reading a complete
        // address on either 32- or 64-bit targets.
        m = (Instr *)*(VclPtrWord *)&link->unk_04;
        in = m;
    }

    int upper = 0;
    int lower = 0;
    int thru = 0;
    int best = 0;
    int both = 0;
    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        if (in->branchFlags)
            unk_2b4 = in->unk_13f0.unk_04;
        char c = in->permittedPipelineMask;
        if (c == 3)
            both++;
        else if (c & 2)
            lower++;
        else
            upper++;
        unk_278.mergeMaximumCounters(&in->unk_13f0);
    }

    int maxIssue = 0;
    for (in = wt->linearInstructions.head; in; in = in->unk_1c.unk_00) {
        for (int i = 0; i != in->operandCount; i++) {
            Operand *op = &in->operands[i];
            RegPoolEntry *r = registerOperand(op, 2);
            if (r) {
                for (int p = 0; p != r->partCount; p++) {
                    int nBoth = 0;
                    int n1 = 0;
                    int nOther = 0;
                    if ((op->componentMask >> p) & 1) {
                        for (ol_s *ol = op->parts[p].unk_54.firstInternalLocation(); ol; ol = ol->nextInternalLocation()) {
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
            unk_2ba = 3;
            best = u;
        }
        if (l > best) {
            unk_2ba = 4;
            best = l;
        }
    }
    if (maxIssue > best) {
        best = maxIssue;
        unk_2ba = 1;
    }

    int depth = unk_278.unk_14;
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
        unk_2ba = 2;
        best = thru;
    }
    if (maxLat > best) {
        unk_2ba = 1;
        best = maxLat;
    }
    for (int i = 0; i != 9; i++) {
        if (i < 0 || i > 8) {
            reportError("bad stat\n");
            exitAfterCleanup(-1);
        }
        int n = (sc->unk_00[i] + 1) >> 1;
        if (n > best) {
            best = n;
            unk_2ba = (i << 8) | 9;
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
    unk_2bc = physSum;
    if (canLoop()) {
        unk_2b8 = unk_2ba;
        logicalLength = best;
        if (best <= 1)
            logicalLength = 2;
    } else {
        unk_2b8 = 5;
        logicalLength = 0;
    }
    if (unk_278.unk_04 >= best) {
        unk_2ba = 6;
        best = unk_278.unk_04 + 1;
    }
    if (unk_278.unk_00 >= best) {
        unk_2ba = 7;
        best = unk_278.unk_00 + 1;
    }
    Instr *br = wt->getBranch();
    if (br) {
        int h = br->unk_13f0.unk_04 + 2;
        if (best <= h) {
            unk_2ba = 8;
            best = h;
        }
    }
    dependencyLength = best;
    wt->unk_74 = dependencyLength;
    wt->unk_76 = logicalLength;
    wt->unk_78 = depth;
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
        len = wt->func_08072eec();
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
        unk_00[i] = 0;
}

bool statCounts::add(Instr *in)
{
    if (!in)
        return false;
    for (int i = 0; i != 9; i++) {
        bool b = (in->opcodeResourceMask >> i) & 1;
        if (b)
            unk_00[i] += 2;
    }
    return true;
}

int statCounts::get(int i)
{
    if (i < 0 || i > 8) {
        reportError("bad stat\n");
        exitAfterCleanup(-1);
    }
    return (unk_00[i] + 1) >> 1;
}

