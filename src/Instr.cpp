#include "VUStr.h"

// Instr (instruction) and Operand, CIE group 4.
#include "Instr.h"
#include <string.h>
#include "os.h"
#include "Parser.h"
#include "VUStr.h"
#include "infoList.h"

char *dupString(const char *s);

extern int errorOccurred;
extern bool quietDiagnostics;

opPair_s pairedOperandDescriptions[] = {
    {109, {1, 1, 0, 2}, 0},
    {110, {1, 1, 0, 2}, 0},
    {112, {2, 1, 0, 2}, 0},
    {158, {1, 2, 1, -1}, 1},
    {163, {1, 2, 1, -1}, 1},
    {159, {2, 2, 1, -1}, 1},
    {164, {2, 2, 1, -1}, 1},
    {0, {0}, 0},
};

opCount_s operandCountOverrides[] = {
    {155, 1}, {160, 1}, {156, 1}, {161, 1}, {0, 0},
};

Instr::Instr(iInfo *info)
{
    if (!info) {
        reportError("constructor called with null iInfo -... later will make this a NOP, but , for now..halt!\n");
        exitAfterCleanup(-1);
    }
    operandCount = 0;
    unk_13d1 = 0;
    instructionModifierBits = 0;
    destinationMask = 0;
    previousThroughputWriter = 0;
    sourceTag = 0;
    unk_18 = 0;
    unk_1a = 0;
    moveRowOffset = 0;
    moveChainDepth = 0;
    abstractMoveOpcode = 0;
    concreteMoveOpcode = 0;
    parsedLoopId = -1;
    branchLabelTags = 0;
    replacedInstructionWord = 0;
    ownerTable = 0;
    moveRecordWord = 0;
    externalInputPairedConstraint = 0;
    externalInputConstraint = 0;
    externalOutputPairedConstraint = 0;
    externalOutputConstraint = 0;
    linearOrderIndex = -1;
    instructionSerial = -1;
    candidateAttemptCount = 0;
    candidateAttemptLimit = 0;
    opcodeInfo = info;
    candidateBaseRow = 0;
    cloneImage = 0;
    unk_00 = 0;
    unk_04 = 0;
    reverseBranchSense = 0;
    tildePrefix = 0;
    scheduledCycle = -1;
    candidateCostOffset = 0;
    expectedAliasLoopId = -1;
    schedulerCategory = -1;
    resetImmFixup();
    opcodeInfo = info;
    unk_13d7 = info->pipelineMask;
    permittedPipelineMask = info->pipelineMask;
    opcodeResourceMask = info->resourceMask;
    branchFlags = info->branchFlags();
    baseIssueSpacing = info->minimumIssueSpacing;
    ramAccessType = info->ramType();
    opcodeFlags = info->kindAndFlags & ~7;
    rawOnly = info->isRawOnly();
    syntheticKind = info->syntheticKind();
    for (int i = 0; info->operandDescriptors[i].operandKind; i++) {
        operandCount++;
        Operand *op = &operands[i];
        iOpDesc *d = &info->operandDescriptors[i];
        op->operandKind = d->operandKind;
        op->operandIndex = i;
        op->unk_0b = d->unk_04 != 0;
        op->direction = d->direction;
        op->isImplicit = d->isImplicit != 0;
        op->unk_05 = d->unk_05;
        op->pairedOperandIndexPlusOne = d->patchedOutputIndexPlusOne;
        if (op->pairedOperandIndexPlusOne)
            operands[op->pairedOperandIndexPlusOne - 1].pairedOperandIndexPlusOne = i + 1;
        if (d->direction == 2)
            op->outputLatency = d->outputLatency;
        else
            op->outputLatency = 0;
    }
}

Instr::~Instr()
{
    if (branchLabelTags) {
        delete[] branchLabelTags;
        branchLabelTags = 0;
    }
    mValue *value = &addressImmediateValue;
    value->clear();
    predecessorDependencies.clear();
    successorDependencies.clear();
}

bool Instr::clearDynamicData()
{
    if (branchLabelTags) {
        delete[] branchLabelTags;
        branchLabelTags = 0;
    }
    addressImmediateValue.clear();
    predecessorDependencies.clear();
    successorDependencies.clear();
    return true;
}

bool Instr::validateOperands()
{
    if (!opcodeInfo)
        return false;
    for (int i = 0; i != operandCount; i++)
        if (!operands[i].validateKindAndMask())
            return false;
    return true;
}

void Instr::dumpOperandDiagnostics(int flags)
{
    if (isValid()) {
        int count = operandCount;
        for (int i = 0; i != count; i++) {
            operands[i].diagnoseOperand(0);
            count = operandCount;
        }
        if (flags & 1) {
            for (int j = 0; j != operandCount; j++) {
                Operand *op = &operands[j];
                RegPoolEntry *r = op->assignedRegister;
                if (r && r->isVirtual)
                    for (int k = 0; k != r->partCount; k++)
                        if ((op->componentMask >> k) & 1)
                            op->parts[k].operandLocations.validateOperandLocations();
            }
        }
    }
}

bool Instr::isNop()
{
    if (!opcodeInfo)
        return true;
    return opcodeInfo->isNop();
}

instStream *Instr::getOwningStream()
{
    return ownerTable ? ownerTable->ownerStream : 0;
}

bool Instr::resetFlowDependencies()
{
    predecessorDependencies.clear();
    successorDependencies.clear();
    scheduledCycle = -1;
    for (int i = 0; i != operandCount; i++)
        if (!operands[i].resetFlowState())
            return false;
    previousThroughputWriter = 0;
    return true;
}

bool Instr::isNonSynthetic()
{
    return !syntheticKind;
}

bool Instr::clearTransientFlags()
{
    candidateAttemptCount = 0;
    candidateCostOffset = 0;
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        for (int k = 0; k != 4; k++)
            op->parts[k].unk_50 = 0;
    }
    return true;
}

void Instr::leDump()
{
    leDump(-1, -1);
}

void Instr::leDump(int opnd)
{
    leDump(opnd, -1);
}

void Instr::leDump(int opnd, int mask)
{
    if (opnd >= operandCount) {
        reportError("Instruction::leDump - out of bounds operand!\n");
        return;
    }
    int saved = errorOccurred;
    errorOccurred = !quietDiagnostics;
    if (sourceTag) {
        int idx = unk_18;
        if (opnd >= 0) {
            if (operands[opnd].assignedRegister && operands[opnd].assignedRegister->partCount != 1 && mask >= 0)
                reportMessage("(field(s) : %s) ", formatComponentMaskLetters(mask));
            if (operands[opnd].isImplicit) {
                if (operands[opnd].assignedRegister)
                    reportMessage("implicit operand '%s'\n", operands[opnd].assignedRegister->registerName());
                else
                    reportError("implicit operand with no register??\n");
            } else
                idx = operands[opnd].unk_24;
        }
        sourceTag->dumpSourceLine(idx);
    } else {
        reportMessage("operand %d modbits %x..\n", opnd, mask);
        dumpOperandDiagnostics(false);
    }
    errorOccurred = saved;
}

bool hasDirectOperandDependency(Instr *a, Instr *b)
{
    if (!a || !b)
        return false;
    for (int i = 0; i != b->operandCount; i++) {
        Operand *op = &b->operands[i];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            int n = r->partCount;
            for (int k = 0; k != n; k++)
                if (((op->componentMask & ~op->externalPartMask) >> k) & 1)
                    for (ol_s *p = op->parts[k].operandLocations.head; p; p = p->next)
                        if (p->instruction == a)
                            return true;
        }
    }
    return false;
}

bool writesInputRegisterOf(Instr *a, Instr *b)
{
    if (!a || !b)
        return false;
    for (int i = 0; i != b->operandCount; i++) {
        Operand *op = &b->operands[i];
        RegPoolEntry *r = registerOperand(op, 1);
        if (r) {
            int want = op->componentMask;
            int mask = 0;
            Instr *in = a;
            for (int j = 0; j != in->operandCount; j++)
                if (registerOperand(&in->operands[j], 2) == r)
                    mask |= in->operands[j].componentMask;
            if (mask & want)
                return true;
        }
    }
    return false;
}



Instr *Instr::clone()
{
    if (!this)
        return 0;
    iInfo *info = opcodeInfo;
    if (!info)
        return 0;
    Instr *c = new Instr(info);
    if (!c) {
        reportError("failed to get a new instruction during clone!\n");
        exitAfterCleanup(-1);
    }
    if (!c->resetOptimizationState()) {
        reportError("failed to opt reset cloned instruction\n");
        exitAfterCleanup(-1);
    }
    c->destinationMask = destinationMask;
    c->rawOnly = rawOnly;
    c->moveRecordWord = 0;
    c->instructionModifierBits = instructionModifierBits;
    c->abstractMoveOpcode = abstractMoveOpcode;
    c->concreteMoveOpcode = concreteMoveOpcode;
    c->unk_13d7 = unk_13d7;
    c->schedulingStatistics.copyCountersFrom(&schedulingStatistics);
    c->linearOrderIndex = linearOrderIndex;
    c->instructionSerial = instructionSerial;
    c->candidateAttemptLimit = candidateAttemptLimit;
    c->parsedLoopId = parsedLoopId;
    c->tildePrefix = tildePrefix;
    c->reverseBranchSense = reverseBranchSense;
    c->sourceTag = sourceTag;
    c->unk_18 = unk_18;
    c->unk_1a = unk_1a;
    c->addressAliasMode = addressAliasMode;
    c->addressAliasDirection = addressAliasDirection;
    c->addressAliasOperandIndex = addressAliasOperandIndex;
    if (addressImmediateValue.isValid())
        c->addressImmediateValue.set(&addressImmediateValue);
    c->expectedAliasLoopId = expectedAliasLoopId;
    if (cloneImage) {
        reportError("during instruction clone - tmpClone was already set!\n");
        delete c;
        return 0;
    }
    cloneImage = c;
    c->cloneImage = this;
    if (branchLabelTags)
        c->branchLabelTags = dupString(branchLabelTags);
    c->operandCount = operandCount;
    Operand *s;
    for (int i = 0; (s = getOp(i)) != 0; i++) {
        Operand *d = c->getOp(i);
        if (d == 0) {
            reportError("fatal error, couldn't get operand during clone\n");
            exitAfterCleanup(-1);
        }
        if (s->operandIndex != i) {
            reportError("bad index in operand for clone\n");
            exitAfterCleanup(-1);
        }
        d->operandIndex = s->operandIndex;
        d->operandKind = s->operandKind;
        d->direction = s->direction;
        d->outputLatency = s->outputLatency;
        d->unk_05 = s->unk_05;
        d->unk_0b = s->unk_0b;
        d->assignedRegister = s->assignedRegister;
        d->componentMask = s->componentMask;
        d->unk_0c = s->unk_0c;
        d->pairedOperandIndexPlusOne = s->pairedOperandIndexPlusOne;
        d->isImplicit = s->isImplicit;
        d->originalRegister = s->originalRegister;
        d->forcedBaseRegister = s->forcedBaseRegister;
        if (s->immediateValue.isValid()) {
            if (!d->immediateValue.set(&s->immediateValue)) {
                reportError("failed to clone immediate\n");
                exitAfterCleanup(-1);
            }
        }
        d->unk_24 = s->unk_24;
        d->unk_25 = s->unk_25;
        d->additionalLatency = s->additionalLatency;
        d->externalPartMask = s->externalPartMask;
        d->operandStatus = 0;
        d->externalComponentMask = s->externalComponentMask;
        d->reductionHazardMask = s->reductionHazardMask;
    }
    return c;
}




int Instr::getRamBits()
{
    int r;
    if (!ramAccessType) {
        memset(&r, 0, sizeof(r));
        return r;
    }
    r = 0;
    if (opcodeInfo) {
        switch ((char)opcodeInfo->destinationMaskMode) {
        case 1:
            r = destinationMask;
            break;
        default:
            reportError("got here! - end of get rambits?\n");
            dumpOperandDiagnostics(false);
            exitAfterCleanup(-1);
            memset(&r, 0, sizeof(r));
            return r;
        case 3: {
            Operand *op = getOp(1);
            if (!op) {
                reportError("failed to get op1 for mod_1 ram test\n");
                exitAfterCleanup(-1);
            }
            if (!op->unk_0c)
                reportWarning("mb was 0 failed to get xmod bits for mod 1 inst\n");
            r = op->unk_0c;
            break;
        }
        case 0:
        case 4:
        case 5:
            r = 15;
            break;
        }
    }
    return r;
}

bool Instr::resetOptimizationState()
{
    position.clearInstructionPosition();
    nextMoveInstruction = 0;
    pendingLink.previousInstruction = 0;
    pendingLink.nextInstruction = 0;
    candidateAttemptCount = 0;
    replacedInstructionWord = 0;
    candidateCostOffset = 0;
    for (int i = 0; i != operandCount; i++) {
        if (!operands[i].resetOptimizationSlots()) {
            reportError("failed to optReset operand  %d\n", i);
            return false;
        }
    }
    return true;
}

int Instr::collectOutputRegisterMask(RegPoolEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *op;
    for (int i = 0; (op = getOp(i)) != 0; i++) {
        RegPoolEntry *r;
        if (op->isOutput())
            r = op->assignedRegister;
        else
            r = 0;
        if (reg == r) {
            *paired = *paired || op->pairedOperandIndexPlusOne;
            mask |= op->componentMask;
        }
    }
    return mask;
}

int Instr::collectInputRegisterMask(RegPoolEntry *reg, bool *paired)
{
    int mask = 0;
    *paired = false;
    Operand *op;
    for (int i = 0; (op = getOp(i)) != 0; i++) {
        RegPoolEntry *r;
        if (op->isInput())
            r = op->assignedRegister;
        else
            r = 0;
        if (reg == r) {
            *paired = *paired || op->pairedOperandIndexPlusOne;
            mask |= op->componentMask;
        }
    }
    return mask;
}


bool Instr::getImmediateOperand(int n, int *out)
{
    Operand *op = getOp(n);
    if (!op) {
        reportError("failed to get operand %d\n", n);
        return false;
    }
    return op->immediateValue.getInt(out);
}







bool Instr::changeBranchImmediate(const char *s)
{
    if (!branchFlags || !opcodeInfo) {
        reportError("changeBranchImmediate: not a branch\n");
        return false;
    }
    if (branchFlags & 2) {
        reportError("changeBranchImmediate: target is register!\n");
        return false;
    }
    int n = opcodeInfo->branchTargetOperandSelector;
    if (!n) {
        reportError("getBranchTargetImmediate - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *op = &operands[n - 1];
    if (op->operandKind != 0x1b) {
        reportError("changeBranchImmediate::target operand isn't immediate!\n");
        exitAfterCleanup(-1);
    }
    return op->immediateValue.setString(s);
}

bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot)
{
    if (!a || !b) {
        reportError("link_oParts, null arg pointer!\n");
        return false;
    }
    if (bSlot != slot) {
        reportError("swizzle detected for ps2 ?\n");
        return false;
    }
    OpSlot *sa = &a->operands[aOp].parts[slot];
    OpSlot *sb = &b->operands[bOp].parts[slot];
    ol_s *oa = sa->operandLocations.addOperandLocation(b, bOp, slot, false);
    if (oa) {
        olList *lb = &sb->operandLocations;
        ol_s *ob = lb->addOperandLocation(a, aOp, slot, false);
        if (!ob) {
            reportError("failed to add ol\n");
            return false;
        }
        return true;
    }
    reportError("failed to add ol\n");
    return false;
}

static inline void dumpLinkOperand(Operand *op)
{
    op->diagnoseOperand(0);
}

bool link_oParts(ol_s *o, ol_s *i, int x, int y)
{
    if (!o) {
        reportError("link_oParts, null arg pointer2!\n");
        return false;
    }
    if (!i) {
        reportError("link_oParts, null arg pointer2!\n");
        return false;
    }
    Operand *out = operandAtLocation(o);
    if (!out) {
        reportError("link_oParts(2) - output not set \n");
        return false;
    }
    if (!isRegisterOperand(out, 2)) {
        reportError("link_oParts - output was not output\n");
        dumpLinkOperand(out);
        return false;
    }
    Operand *in = operandAtLocation(i);
    if (!in) {
        reportError("link_oParts(2) - input not set \n");
        return false;
    }
    if (!isRegisterOperand(in, 1)) {
        reportError("link_oParts - input was not input\n");
        return false;
    }
    int to = o->componentIndex;
    int ti = i->componentIndex;
    if (!((out->componentMask >> to) & 1)) {
        reportError("link_oParts - mod bit not set for output\n");
        return false;
    }
    if (!((in->componentMask >> ti) & 1)) {
        reportError("link_oParts - mod bit not set for input\n");
        return false;
    }
    if (!((in->externalPartMask >> ti) & 1)) {
        reportError("link_oParts_ext - external bit not set for input\n");
        return false;
    }
    out->externalPartMask |= 1 << to;
    if (locationOwningStream(o) != locationOwningStream(i)) {
        out->externalComponentMask |= 1 << o->componentIndex;
        in->externalComponentMask |= 1 << i->componentIndex;
    }
    if (to != ti) {
        reportError("link_oParts - not a bug, but should not happen for this cpu, t_o!=t_i (%d!=%d)\n", to, ti);
        return false;
    }
    OpSlot *so = out->parts + ti;
    OpSlot *si = in->parts + ti;
    olList *lo = &so->operandLocations;
    ol_s *a = lo->copyOperandLocation(i, true);
    if (!a) {
        reportError("failed to add ol_input to output\n");
        return false;
    }
    olList *li = &si->operandLocations;
    ol_s *b = li->copyOperandLocation(o, true);
    if (!b) {
        reportError("failed to add ol_ouput to input\n");
        return false;
    }
    int s = o->cycleOffset;
    if (s > 0xff)
        s = 0xff;
    a->cycleOffset = s;
    a->pathKey = x;
    a->unk_0c = y;
    b->cycleOffset = 0;
    b->pathKey = x;
    b->unk_0c = y;
    return true;
}

bool Instr::setOwner(cls_unk30 *s)
{
    if (ownerTable && ownerTable != s) {
        reportError("in->setOwner - already owned..but by wrong stream!\n");
        exitAfterCleanup(-1);
    }
    ownerTable = s;
    return true;
}

const char *branchTypeNameByCode(int t)
{
    switch (t) {
    case 0:
        return "BTYPE_NULL";
    case 1:
        return "BTYPE_ABS";
    case 3:
        return "BTYPE_REG";
    case 5:
        return "BTYPE_COND_ABS";
    case 7:
        return "BTYPE_COND_REG";
    case 9:
        return "BTYPE_ABS_LINK";
    case 11:
        return "BTYPE_REG_LINK";
    default:
        return "BTYPE_TYPE_THIS_IN!";
    case 13:
        return "BTYPE_COND_ABS_LINK";
    }
}

char *Instr::emitExpectedSyntax(bool flag, char *buf, int)
{
    if (!buf)
        return 0;
    iInfo *info = opcodeInfo;
    if (info) {
        buf += formatIntoBuffer(buf, info->mnemonic);
        switch ((char)info->destinationMaskMode) {
        case 1:
            buf += formatIntoBuffer(buf, ".[xyzw]");
            break;
        case 2:
            buf += formatIntoBuffer(buf, ".xyz");
            break;
        case 3:
            buf += formatIntoBuffer(buf, ".[x/y/z/w]");
            break;
        }
    } else {
        buf += formatIntoBuffer(buf, "nop");
    }
    buf += formatIntoBuffer(buf, "\t");
    for (int n = 0; n != operandCount; n++) {
        if (operands[n].isImplicit)
            break;
        if (n)
            buf += formatIntoBuffer(buf, ",");
        buf = operands[n].emitExpectedOperandType(flag, buf);
        if (!buf)
            break;
    }
    return buf;
}

int Instr::outputMaskForRegister(RegPoolEntry *reg, int mask)
{
    int result = 0;
    if (reg) {
        for (int i = 0; i != operandCount; i++) {
            Operand *op = &operands[i];
            if (op->outRegZ() == reg)
                result |= op->componentMask;
        }
    }
    return result & mask;
}

int Instr::inputMaskForRegister(RegPoolEntry *reg, int mask)
{
    int result = 0;
    if (reg) {
        for (int i = 0; i != operandCount; i++) {
            Operand *op = &operands[i];
            if (op->inRegOp2() == reg)
                result |= op->componentMask;
        }
    }
    return result & mask;
}

const char *syntheticTypeName(int t)
{
    const char *s = "SYNTH_UNKNOWN";
    switch (t) {
    case 0: s = "SYNTH_NULL"; break;
    case 1: s = "SYNTH_ENTER"; break;
    case 2: s = "SYNTH_ENDENTER"; break;
    case 3: s = "SYNTH_EXIT"; break;
    case 4: s = "SYNTH_ENDEXIT"; break;
    case 5: s = "SYNTH_CONT"; break;
    case 6: s = "SYNTH_XINPUT"; break;
    case 7: s = "SYNTH_XOUTPUT"; break;
    case 8: s = "SYNTH_LOOP_MINSAFE"; break;
    case 9: s = "SYNTH_LOOP_EXTRA"; break;
    case 10: s = "SYNTH_BARRIER"; break;
    }
    return s;
}

bool Instr::hasRawTableOrImmediateToken()
{
    if (unk_00)
        return true;
    if (ownerTable)
        return ownerTable->codeModeValue == 3;
    return false;
}

char *Instr::getBranchTargetImmediate()
{
    iInfo *ii = opcodeInfo;
    if (!branchFlags)
        return 0;
    if (!ii) {
        reportError("branch type set - but no ii\n");
        exitAfterCleanup(-1);
    }
    int n = ii->branchTargetOperandSelector;
    if (!n) {
        reportError("getBranchTargetImmediate - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *op = &operands[n - 1];
    if (!op->isInput()) {
        reportError("alleged branch target immediate wasn't an input\n");
        exitAfterCleanup(-1);
    }
    if (op->operandKind == 0x1b)
        return op->immediateValue.getString();
    return 0;
}

RegPoolEntry *Instr::getBranchTargetRegister()
{
    iInfo *ii = opcodeInfo;
    if (!(branchFlags & 2))
        return 0;
    if (!ii) {
        reportError("branch type set - but no ii\n");
        exitAfterCleanup(-1);
    }
    int n = ii->branchTargetOperandSelector;
    if (!n) {
        reportError("getBranchTargetRegister - branch target operand not set\n");
        exitAfterCleanup(-1);
    }
    Operand *op = &operands[n - 1];
    if (!op->isInput()) {
        reportError("alleged branch target register wasn't an input\n");
        exitAfterCleanup(-1);
    }
    if (op->operandKind == 0x1b)
        return 0;
    RegPoolEntry *r = op->assignedRegister;
    if (!r) {
        reportError("branch target register wasn't set\n");
        exitAfterCleanup(-1);
    }
    return r;
}

int Instr::func_08055718()
{
    return 0;
}

Operand *Instr::findInputOperand(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->inRegOp2() == reg && (op->componentMask & mask))
            return op;
    }
    return 0;
}

Operand *Instr::findOutputOperand(RegPoolEntry *reg, int mask)
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->outRegZ() == reg && (op->componentMask & mask))
            return op;
    }
    return 0;
}

bool Instr::hasLinkedOutputFrom(unsigned int n)
{
    if (!opcodeInfo)
        return false;
    for (; n < operandCount; n++) {
        Operand *op = &operands[n];
        RegPoolEntry *r = registerOperand(op, 2);
        if (r) {
            if (op->externalPartMask)
                return true;
            for (int j = 0; j != r->partCount; j++)
                if (((op->componentMask >> j) & 1) && op->parts[j].operandLocations.count)
                    return true;
        }
    }
    return false;
}

bool Instr::classifySchedulingCategory()
{
    if (ramAccessType == 2) {
        schedulerCategory = 1;
        return true;
    }
    if (opcodeFlags & 0x40) {
        schedulerCategory = 2;
        return true;
    }
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (isRegisterOperand(op, 2) && op->externalComponentMask) {
            schedulerCategory = 2;
            return true;
        }
    }
    schedulerCategory = 0;
    return true;
}

Instr **Instr::getIE()
{
    cls_unk30 *table = ownerTable;
    if (!table) {
        reportError("failed to get itable from instruction\n");
        return 0;
    }
    InstructionPosition *pos = &position;
    Instr **ie = table->getIE_ck(pos->linearRow);
    if (!ie) {
        reportError("failed to get ie from table for in::getIE()\n");
        return 0;
    }
    if (ie[pos->pipelineIndex] != this) {
        reportError("we aren't where we thought we were for in::getIE\n");
        return 0;
    }
    return ie;
}

int Instr::getFirstOutputLatency()
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->outRegZ() && !op->unk_0b)
            return op->outputLatency;
    }
    return 0;
}

bool Instr::markDependencyInputReachable()
{
    bool r = !unk_0e_1;
    unk_0e_1 = 1;
    return r;
}

bool Instr::markDependencyOutputReachable()
{
    bool r = !unk_0e_0;
    unk_0e_0 = 1;
    return r;
}

int Instr::opcodeId()
{
    if (!opcodeInfo)
        return -1;
    return getOpcodeId(opcodeInfo);
}

static inline bool isLiteral(mValue *v)
{
    return v->type == mValue::STRING;
}

bool Instr::expandVirtualStatOperand()
{
    for (int i = 0; i != operandCount; i++) {
        Operand *op = &operands[i];
        if (op->operandKind < 22 || op->operandKind > 25)
            continue;
        if (i != operandCount - 1) {
            reportError("virtual stat operand not final\n");
            return false;
        }
        operandCount--;
        switch (op->operandKind) {
        case 23:
            op->isImplicit = 1;
            op->operandKind = 21;
            op->direction = 2;
            op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(55);
            op->componentMask = 1;
            op->outputLatency = 4;
            op->unk_0b = 0;
            op->operandIndex = operandCount++;
            op = &operands[i + 1];
            op->isImplicit = 1;
            op->operandKind = 18;
            op->direction = 1;
            op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(57);
            op->componentMask = 1;
            op->outputLatency = -1;
            op->unk_0b = 1;
            op->operandIndex = operandCount++;
            break;
        case 22:
            op->isImplicit = 1;
            op->operandKind = 20;
            op->direction = 2;
            op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(56);
            op->componentMask = 1;
            op->outputLatency = operands[0].outputLatency;
            op->unk_0b = 0;
            op->operandIndex = operandCount;
            op = &operands[i + 1];
            operandCount++;
            op->isImplicit = 1;
            op->operandKind = 19;
            op->direction = 1;
            op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(58);
            op->componentMask = 1;
            op->outputLatency = -1;
            op->unk_0b = 1;
            op->operandIndex = operandCount++;
            break;
        case 24: {
            if (operands[i - 1].operandKind != 27) {
                reportError("not immediate predecessor in fs test\n");
                return false;
            }
            bool a = true;
            bool b = true;
            bool c = true;
            bool d = true;
            mValue *v = &operands[i - 1].immediateValue;
            unsigned int imm;
            if (isLiteral(v)) {
                reportWarning("fsop has literals - so assuming the worst - if it were numerical vcl could figure out dependencies better\n");
            } else if (v->getUInt(&imm)) {
                unsigned int bits;
                int id = -1;
                if (opcodeInfo)
                    id = getOpcodeId(opcodeInfo);
                switch (id) {
                case 147:
                    break;
                case 148:
                    bits = ~imm;
                    goto test;
                case 146:
                    bits = imm;
                test:
                    a = (bits & 0xf) != 0;
                    b = (bits & 0x30) != 0;
                    c = (bits & 0xc00) != 0;
                    d = (bits & 0x3c0) != 0;
                    break;
                default:
                    reportError("bad fsop \n");
                    return false;
                }
            } else {
                reportError("failed to get immediate value\n");
                return false;
            }
            a = a | c;
            b = b | d;
            if (a) {
                op->isImplicit = 1;
                op->operandKind = 21;
                op->direction = 1;
                op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(55);
                op->componentMask = 1;
                op->outputLatency = -1;
                op->unk_0b = 0;
                op->operandIndex = operandCount;
                op++;
                operandCount++;
            }
            if (b) {
                op->isImplicit = 1;
                op->operandKind = 20;
                op->direction = 1;
                op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(56);
                op->componentMask = 1;
                op->outputLatency = -1;
                op->unk_0b = 0;
                op->operandIndex = operandCount;
                op++;
                operandCount++;
            }
            if (c) {
                op->isImplicit = 1;
                op->operandKind = 18;
                op->direction = 1;
                op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(57);
                op->componentMask = 1;
                op->outputLatency = -1;
                op->unk_0b = 1;
                op->operandIndex = operandCount;
                op++;
                operandCount++;
                op->isImplicit = 1;
                op->operandKind = 18;
                op->direction = 2;
                op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(57);
                op->componentMask = 1;
                op->outputLatency = 1;
                op->unk_0b = 1;
                op->operandIndex = operandCount;
                op++;
                operandCount++;
            }
            if (d) {
                op->isImplicit = 1;
                op->operandKind = 19;
                op->direction = 1;
                op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(58);
                op->componentMask = 1;
                op->outputLatency = -1;
                op->unk_0b = 1;
                op->operandIndex = operandCount;
                op++;
                operandCount++;
                op->isImplicit = 1;
                op->operandKind = 19;
                op->direction = 2;
                op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(58);
                op->componentMask = 1;
                op->outputLatency = 1;
                op->unk_0b = 1;
                op->operandIndex = operandCount++;
            }
            break;
        }
        case 25:
            op->isImplicit = 1;
            op->operandKind = 18;
            op->direction = 2;
            op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(57);
            op->componentMask = 1;
            op->outputLatency = 1;
            op->unk_0b = 1;
            op->operandIndex = operandCount;
            op = &operands[i + 1];
            operandCount++;
            op->isImplicit = 1;
            op->operandKind = 19;
            op->direction = 2;
            op->originalRegister = op->assignedRegister = findRegisterByGlobalIndex(58);
            op->componentMask = 1;
            op->outputLatency = 1;
            op->unk_0b = 1;
            op->operandIndex = operandCount++;
            break;
        }
    }
    return true;
}

void Instr::clearExternalTimingConstraints()
{
    externalInputPairedConstraint = 0;
    externalInputConstraint = 0;
    externalOutputPairedConstraint = 0;
    externalOutputConstraint = 0;
}

void Instr::copyExternalTimingConstraints(Instr *src)
{
    externalInputPairedConstraint = src->externalInputPairedConstraint;
    externalInputConstraint = src->externalInputConstraint;
    externalOutputPairedConstraint = src->externalOutputPairedConstraint;
    externalOutputConstraint = src->externalOutputConstraint;
}

void Instr::raiseExternalInputPairedConstraint(short v)
{
    externalInputPairedConstraint = externalInputPairedConstraint < v ? v : externalInputPairedConstraint;
}

void Instr::raiseExternalInputConstraint(short v)
{
    externalInputConstraint = externalInputConstraint < v ? v : externalInputConstraint;
}

void Instr::raiseExternalOutputPairedConstraint(short v)
{
    externalOutputPairedConstraint = externalOutputPairedConstraint < v ? v : externalOutputPairedConstraint;
}

void Instr::raiseExternalOutputConstraint(short v)
{
    externalOutputConstraint = externalOutputConstraint < v ? v : externalOutputConstraint;
}

OpSlot::OpSlot()
{
}

OpSlot::~OpSlot()
{
}

void OpSlot::noopOperandSlotHook()
{
}

bool OpSlot::resetSlotLock()
{
    resetLock();
    unk_50 = 0;
    unk_52[0] = 0;
    return true;
}


int OpSlot::clearSlotLocations()
{
    return operandLocations.clear();
}
