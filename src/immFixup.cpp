#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"
#include "os.h"

bool fixAddressAliases(InstructionStream *s);
bool resolveInstructionImmediateFixup(Instr *in);
int maddImmediate(Operand *op, TypedValue *v, int n);

static inline void resetInstructionAliasMetadata(Instr *instruction)
{
    instruction->addressAliasMode = 0;
    instruction->addressAliasDirection = 0;
    instruction->addressAliasOperandIndex = -1;
    instruction->addressImmediateValue.clear();
    instruction->expectedAliasLoopId = -1;
}

static inline bool resetStreamAliasMetadata(InstructionStream *stream)
{
    ScheduleTable *table = stream->getTable();
    if (!table) {
        reportError("Init_immFixup - failed to get work table\n");
        return false;
    }
    stream->streamSerial = 0;
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        resetInstructionAliasMetadata(instruction);
    return true;
}

static inline void resolveInstructionFixupOrAbort(Instr *instruction)
{
    if (!resolveInstructionImmediateFixup(instruction)) {
        reportError("do_immFixup - failed for an inst\n");
        exitAfterCleanup(-1);
    }
}

static inline bool resolveStreamImmediateFixups(InstructionStream *stream)
{
    ScheduleTable *table = stream->getTable();
    if (!table)
        return true;
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        resolveInstructionFixupOrAbort(instruction);
    return true;
}

static inline bool dumpFailed(InstructionStream *stream)
{
    stream->dump(false);
    return false;
}

bool resolveProgramImmediateFixups(Program *program, bool init)
{
    InstructionStream *stream;
    if (init) {
        for (stream = program->streamHead; stream; stream = stream->nextStream)
            if (!resetStreamAliasMetadata(stream))
                return false;
        for (stream = program->streamHead; stream; stream = stream->nextStream)
            if (!fixAddressAliases(stream))
                return dumpFailed(stream);
    } else {
        for (stream = program->streamHead; stream; stream = stream->nextStream)
            if (!resolveStreamImmediateFixups(stream))
                return false;
        for (stream = program->streamHead; stream; stream = stream->nextStream)
            if (!resetStreamAliasMetadata(stream))
                return false;
    }
    return true;
}


static inline bool bit0(unsigned char mask)
{
    return mask & 1;
}

static inline bool bit0Clear(unsigned char mask)
{
    mask ^= 1;
    return mask & 1;
}

static inline bool hasAaData(Instr *instruction)
{
    if (instruction->addressAliasMode != 0 || instruction->addressAliasDirection != 0 ||
        instruction->addressAliasOperandIndex != -1 || instruction->addressImmediateValue.isValid())
        return true;
    return false;
}

static inline bool immValid(TypedValue *value)
{
    return value->isValid();
}

static inline RegisterUse *firstRegUse(ScheduleTable *table) { return table->registerUses.head; }
static inline RegisterEntry *regUseReg(RegisterUse *registerUse) { return registerUse->registerEntry; }
static inline Instr *regUseInstr(RegisterUse *registerUse) { return registerUse->definingInstruction; }
static inline olList *regUseVisList0(RegisterUse *registerUse) { return &registerUse->slots[0].inputLocations; }
static inline unsigned char regUseExtIn(RegisterUse *registerUse) { return registerUse->externalInputMask; }
static inline unsigned char regUseExtOut(RegisterUse *registerUse) { return registerUse->externalOutputMask; }
static inline Operand *instrRawOp(Instr *instruction, unsigned char idx) { return &instruction->operands[idx]; }
static inline Operand *instrRawOpS(Instr *instruction, signed char idx) { return &instruction->operands[idx]; }
static inline RegisterEntry *opRawReg(Operand *operand) { return operand->assignedRegister; }
static inline unsigned char opSlotMask(Operand *operand) { return operand->componentMask; }
static inline TypedValue *opImmVal(Operand *operand) { return &operand->immediateValue; }
static inline olList *opSlotUses0(Operand *operand) { return &operand->parts[0].operandLocations; }
static inline Instr *olInstr(OperandLocation *operandLocation) { return operandLocation->instruction; }
static inline char olOpIndex(OperandLocation *operandLocation) { return operandLocation->operandIndex; }
static inline TypedValue *instrAaImm(Instr *instruction) { return &instruction->addressImmediateValue; }
static inline unsigned char aaOutOp(AddressAliasDescriptor *aliasDescriptor) { return aliasDescriptor->outputOperandIndex; }
static inline unsigned char aaInOp(AddressAliasDescriptor *aliasDescriptor) { return aliasDescriptor->inputOperandIndex; }
static inline signed char aaImmOp(AddressAliasDescriptor *aliasDescriptor) { return aliasDescriptor->immediateOperandIndex; }
static inline int aaImmLit(AddressAliasDescriptor *aliasDescriptor) { return aliasDescriptor->literalImmediate; }
static inline unsigned char aaUseOp(AddressAliasUse *aliasUse) { return aliasUse->operandIndex; }

bool fixAddressAliases(InstructionStream *stream)
{
    ScheduleTable *table = stream->getTable();
    if (!table) {
        reportError("Init_immFixup - failed to get work table\n");
        return false;
    }
    for (RegisterUse *registerUse = firstRegUse(table); registerUse; registerUse = registerUse->next) {
        RegisterEntry *registerEntry = regUseReg(registerUse);
        if (!registerEntry) {
            reportError("aaFixup - regref had no register\n");
            exitAfterCleanup(-1);
        }
        Instr *instruction = regUseInstr(registerUse);
        olList *vis = regUseVisList0(registerUse);
        AddressAliasDescriptor *aliasDescriptor = findAddressAliasDescriptor(instruction);
        if (!aliasDescriptor || !bit0(regUseExtIn(registerUse)) || !bit0(regUseExtOut(registerUse)) || registerUse->recordedOutputWriteCount != 1)
            continue;
        RegisterEntry *outReg = instruction->operands[aliasDescriptor->outputOperandIndex].assignedRegister;
        if (outReg != registerEntry || opRawReg(instrRawOp(instruction, aaInOp(aliasDescriptor))) != outReg)
            continue;
        OperandLocation *operandLocation;
        for (operandLocation = vis->firstInternalLocation(); operandLocation; operandLocation = operandLocation->nextInternalLocation()) {
            Instr *aliasInstruction = olInstr(operandLocation);
            Operand *vop = operandAtLocation(operandLocation);
            if (!aliasInstruction)
                goto bad1;
            if (!vop)
                goto bad2;
        cont:
            {
            AddressAliasUse *aliasUse = findAddressAliasUse(aliasInstruction);
            if (aliasUse && olOpIndex(operandLocation) == aaUseOp(aliasUse) && opSlotMask(vop) == 1) {
                if (hasAaData(aliasInstruction)) {
                    reportError("instruction already has aa data loaded .. mode before\n");
                    aliasInstruction->dumpOperandDiagnostics(false);
                    immValid(instrAaImm(aliasInstruction));
                    return false;
                }
                aliasInstruction->addressAliasDirection = 1;
                aliasInstruction->addressAliasMode = aliasDescriptor->aliasMode;
                aliasInstruction->addressAliasOperandIndex = aliasUse->operandIndex;
                TypedValue *value = instrAaImm(aliasInstruction);
                if (aliasDescriptor->immediateOperandIndex >= 0) {
                    if (!value->set(opImmVal(instrRawOpS(instruction, aaImmOp(aliasDescriptor))))) {
                        reportError("failed to set xaaImm for before\n");
                        return false;
                    }
                } else
                    value->setInt(aaImmLit(aliasDescriptor));
            }
            continue;
            }
        bad1:
            reportError("failed to get instruction for visible input\n");
            exitAfterCleanup(-1);
            goto cont;
        bad2:
            reportError("failed to get instruction for visible input\n");
            exitAfterCleanup(-1);
            goto cont;
        }
        Operand *out = instruction->getOp(aaOutOp(aliasDescriptor));
        if (!out || !out->isOutput() || bit0Clear(opSlotMask(out))) {
            reportError("failed to get output operand for aa\n");
            exitAfterCleanup(-1);
        }
        for (operandLocation = opSlotUses0(out)->firstInternalLocation(); operandLocation; operandLocation = operandLocation->nextInternalLocation()) {
            Instr *aliasInstruction = olInstr(operandLocation);
            Operand *vop = operandAtLocation(operandLocation);
            if (!aliasInstruction || !vop) {
                reportError("failed to get instruction for visible input.. mode after\n");
                exitAfterCleanup(-1);
            }
            AddressAliasUse *aliasUse = findAddressAliasUse(aliasInstruction);
            if (aliasUse && olOpIndex(operandLocation) == aaUseOp(aliasUse) && opSlotMask(vop) == 1) {
                if (hasAaData(aliasInstruction)) {
                    reportError("instruction already has aa data loaded.. mode after\n");
                    aliasInstruction->dumpOperandDiagnostics(false);
                    immValid(instrAaImm(aliasInstruction));
                    return false;
                }
                TypedValue *value = instrAaImm(aliasInstruction);
                aliasInstruction->addressAliasDirection = 2;
                aliasInstruction->addressAliasMode = aliasDescriptor->aliasMode;
                aliasInstruction->addressAliasOperandIndex = aliasUse->operandIndex;
                if (aliasDescriptor->immediateOperandIndex >= 0) {
                    if (!value->set(opImmVal(instrRawOpS(instruction, aaImmOp(aliasDescriptor))))) {
                        reportError("failed to set xaaImm after\n");
                        return false;
                    }
                } else
                    value->setInt(aaImmLit(aliasDescriptor));
            }
        }
    }
    return true;
}

bool tidyStream(InstructionStream *stream)
{
    ScheduleTable *table = stream->getTable();
    if (!table)
        return true;
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        resolveInstructionFixupOrAbort(instruction);
    return true;
}

bool tidyTable(ScheduleTable *table)
{
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        resolveInstructionFixupOrAbort(instruction);
    return true;
}

bool tidyProgram(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream)
        if (!tidyStream(stream)) {
            reportError("failed to tidy a stream\n");
            exitAfterCleanup(-1);
        }
    return true;
}

struct epSrc {
    short lid;
    short type;
};

bool resolveSourceEpilogueLoopId(olList *oll, int luid, int epType, int *lid)
{
    if (luid < 0) {
        reportError("entered getSrcEpLid with -ve luid\n");
        return false;
    }
    if (!oll->count) {
        reportError("oll has no entries for getSrcEpLid\n");
        return false;
    }
    int nLocal = 0;
    OperandLocation *operandLocation;
    for (operandLocation = oll->head; operandLocation; operandLocation = operandLocation->next) {
        Instr *instruction = operandLocation->instruction;
        if (!instruction) {
            reportError("failed to get in from ol\n");
            exitAfterCleanup(-1);
        }
        InstructionStream *stream = instruction->getOwningStream();
        if (!stream) {
            reportError("getSrcEpLid- failed to get owner\n");
            return false;
        }
        if (stream->streamSerial != luid) {
            *lid = -1;
            return true;
        }
    }
    epSrc src[2];
    for (operandLocation = oll->head; operandLocation; operandLocation = operandLocation->next) {
        Instr *instruction = operandLocation->instruction;
        if (!instruction) {
            reportError("failed to get in from ol\n");
            exitAfterCleanup(-1);
        }
        InstructionStream *stream = instruction->getOwningStream();
        if (!stream) {
            reportError("getSrcEpLid- failed to get owner\n");
            return false;
        }
        int type = stream->loopRegionRole;
        if (stream->streamSerial != luid) {
            reportError("getSrcEpLid() - rather unlikely\n");
            return false;
        }
        if (nLocal > 1) {
            reportError("nLocal got >1 in getSrcEpLid()\n");
            oll->validateOperandLocations();
            return false;
        }
        src[nLocal].lid = instruction->expectedAliasLoopId;
        src[nLocal].type = type;
        nLocal++;
    }
    if (nLocal == 0) {
        reportError("no valid eplids found for getSrc\n");
        return false;
    }
    if (nLocal == 2) {
        if (src[0].type == src[1].type) {
            reportError("nLocal is 2 , but both setters from same type of construct\n");
            return false;
        }
        if (epType == 2) {
            reportError("nLocal is 2, but we are a prologue!\n");
            return false;
        }
        for (int pipeIndex = 0; pipeIndex != 2; pipeIndex++) {
            switch (epType) {
            case 3:
                switch (src[pipeIndex].type) {
                case 2:
                    break;
                case 5:
                    src[pipeIndex].lid--;
                    break;
                default:
                    reportError("epSrc: we are EPI, but not MAIN/PRO setter for n=2\n");
                    return false;
                }
                break;
            case 5:
                switch (src[pipeIndex].type) {
                case 2:
                    break;
                case 5:
                    src[pipeIndex].lid--;
                    break;
                default:
                    reportError("epSrc: we are MAIN, but not MAIN/PRO setter\n");
                    return false;
                }
                break;
            default:
                reportError("bad loop type found for n=2\n");
                return false;
            }
        }
        if (src[0].lid != src[1].lid) {
            reportError("lid mismatch for n=2\n");
            return false;
        }
    } else {
    switch (epType) {
    case 2:
        break;
    case 3:
        switch (src[0].type) {
        case 2:
        case 3:
            break;
        case 5:
            src[0].lid--;
            break;
        default:
            reportError("bad ep type for n=1\n");
            return false;
        }
        break;
    case 5:
        if (src[0].type != 2 && src[0].type != 5) {
            reportError("bad ep type for n=1  %d\n", src[0].type);
            return false;
        }
        break;
    default:
        reportError("mylm is bad for n=1\n");
        return false;
    }
    }
    *lid = src[0].lid;
    return true;
}

static inline bool isCinput(Operand *operand)
{
    return operand->direction == 1;
}

bool resolveInstructionImmediateFixup(Instr *instruction)
{
    InstructionStream *stream = instruction->getOwningStream();
    if (!stream) {
        reportError("failed to get owner istream from inst during immediate fixup\n");
        exitAfterCleanup(-1);
    }
    if (!instruction->addressAliasMode)
        return true;
    Operand *operand = instruction->getOp(instruction->addressAliasOperandIndex);
    if (!operand) {
        reportError("failed to get opernad for imm fix\n");
        exitAfterCleanup(-1);
    }
    if (!isCinput(operand)) {
        reportError("operand isn't cinput for in fix\n");
        exitAfterCleanup(-1);
    }
    int epilogueIndex;
    if (stream->streamSerial) {
        if (instruction->expectedAliasLoopId < 0) {
            reportError("loop, epID<0 for operand in im fixup aaDstMode=%d..epMode=%d\n",
                          instruction->addressAliasDirection, instruction->expectedAliasLoopId);
            instruction->dumpOperandDiagnostics(false);
            exitAfterCleanup(-1);
        }
        epilogueIndex = instruction->expectedAliasLoopId;
    } else {
        if (instruction->expectedAliasLoopId >= 0) {
            reportError("nonloop, epID>=0 for operand in im fixup aaDstmode=%d..epMode=%d\n",
                          instruction->addressAliasDirection, instruction->expectedAliasLoopId);
            instruction->dumpOperandDiagnostics(false);
            exitAfterCleanup(-1);
        }
        epilogueIndex = 0;
    }
    if (instruction->addressAliasDirection == 1)
        epilogueIndex--;
    int lid;
    if (stream->streamSerial) {
        olList *oll = &operand->parts[0].operandLocations;
        if (!resolveSourceEpilogueLoopId(oll, stream->streamSerial, stream->loopRegionRole, &lid)) {
            reportError("failed to determine actual loopid of setter.. we are:\n");
            instruction->dumpInstructionContext();
            if (instruction->getOwningStream())
                reportError("parent stream is : %s\n", instruction->getOwningStream()->streamName());
            reportError("expected was %d\n", instruction->expectedAliasLoopId);
            exitAfterCleanup(-1);
        }
    } else
        lid = operand->externalPartMask ? -1 : 0;
    if (epilogueIndex != lid) {
    int diff = epilogueIndex - lid;
    int dir;
    if (diff < 0) {
        dir = -1;
        diff = -diff;
    } else
        dir = 1;
    int mul;
    switch (instruction->addressAliasMode) {
    case 1:
        mul = 1;
        break;
    case 2:
        mul = -1;
        break;
    case 3:
        reportError("well - I'll code this one later\n");
        exitAfterCleanup(-1);
        return false;
    default:
        reportError("bad opchar mode %d\n", instruction->addressAliasMode);
        exitAfterCleanup(-1);
        return false;
    }
    if (dir * mul < 0)
        diff = -diff;
    int fixupResult = maddImmediate(&instruction->operands[instruction->addressAliasOperandIndex], &instruction->addressImmediateValue, diff);
    if (fixupResult < 0) {
        reportError("failed to madd immediates\n");
        instruction->dumpInstructionContext(instruction->addressAliasOperandIndex);
        exitAfterCleanup(-1);
    }
    if (fixupResult == 1) {
        reportError("ran out of bits for immfixup\n");
        return false;
    }
    }
    return true;
}

bool initializeImmediateFixups(InstructionStream *stream)
{
    ScheduleTable *table = stream->getTable();
    if (!table) {
        reportError("Init_immFixup - failed to get work table\n");
        return false;
    }
    stream->streamSerial = 0;
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction)
        resetInstructionAliasMetadata(instruction);
    return true;
}

void Instr::hasImmValue()
{
    immValid(&addressImmediateValue);
}

bool Instr::hasImmFixup()
{
    if (addressAliasMode != 0 || addressAliasDirection != 0 || addressAliasOperandIndex != -1 || addressImmediateValue.isValid())
        return true;
    return false;
}

void Instr::resetImmFixup()
{
    addressAliasMode = 0;
    addressAliasDirection = 0;
    addressAliasOperandIndex = -1;
    addressImmediateValue.clear();
}

extern const char parseImmediateError[] = "failed to parse immediate '%s'\n";
extern const char evalImmediateError[] = "failed to evaluate tree for '%s' to type %s\n";
extern const char parseImmediateWarn[] = "encountered an error parsing '%s'\n";
extern const char maddImmediateInputError[] = "madd immediate- input not valid\n";
