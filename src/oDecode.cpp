// Instruction line decoding ("oDecode", CIE group 38).
#include "os.h"
#include "Parser.h"
#include "mList.h"
#include "Processor.h"
#include "VUStr.h"
#include "Instr.h"
#include "workTable.h"
#include "decodedLine.h"
#include "Program.h"

char gatheredToken[1024];

int gatherTokens(LocTag *t, int i, bool stopAtOpcode);
int countSetBits(unsigned int x);
bool asciiCaseEqual(const char *a, const char *b);
char *dupString(const char *s);
int checkedStringLength(const char *s);
bool oDecodeToken(int kind, char *tok, ImplicitOperandDescription *out, bool newSyntax);
bool parseImmediate(const char *s, unsigned char type, TypedValue *v);
RegPool *getRegisterPoolByIndex(unsigned t);
int expectedSyntaxDiagnosticMode = -1;

Instr *Processor::getNextInstruction(LocTag *locationTag, int *pos)
{
    int copyCount;
    ImplicitOperandDescription decodedOperand;
    char buf[0x801];
    char nameBuf[0x400];
    char fmtBuf[0x400];
    int cur = *pos;
    short loopid;
    bool explicitMods = false;
    loopid = 0;
    copyCount = 0;
    bool forcedMods = false;
    bool parsedSuccessfully = false;
    int opBits = -1;
    Instr *decodedInstruction = 0;
    bool bad = false;
    int mask;
    OpcodeDescriptor *opcodeInfo;
    int mods;
    char tilde = 0;
    int operandIndex;
    char *expandedRegisterNameCursor = 0;
    int mnemonicLen;
    char *mnemonicCursor;
    char *loopTagDelimiterCursor;
    bool hasMods;

    if (!locationTag) {
        reportError("getNextInstruction - passed null pointer\n");
        goto fail;
    }
    if (locationTag->isCommentToken(cur)) {
        reportError("getNextInstruction - tried to parse a comment\n");
        goto fail;
    }
    {
        char *tokenSource = locationTag->tokenTextAt(cur);
        char *bufferDestination = buf;
        mnemonicCursor = bufferDestination;
        for (; *tokenSource && copyCount < VCL_TOKEN_COPY_LIMIT; copyCount++)
            *bufferDestination++ = *tokenSource++;
        *bufferDestination = 0;
    }
    mods = removeInstructionModifierGroups(mnemonicCursor);
    loopTagDelimiterCursor = mnemonicCursor;
    if (buf[0] == '~') {
        loopTagDelimiterCursor = buf + 1;
        tilde = 1;
        mnemonicCursor = loopTagDelimiterCursor;
    }
    while (*loopTagDelimiterCursor && !(loopTagDelimiterCursor[0] == '.' && loopTagDelimiterCursor[1] == '.'))
        loopTagDelimiterCursor++;
    if (*loopTagDelimiterCursor == '.') {
        if (*mnemonicCursor == '+')
            loopid = 0x1024;
        else {
            loopid = parseInteger(mnemonicCursor);
            if (loopid > 32) {
                reportUserMessage("unlinkely large loopid set with instruction\n");
                goto fail;
            }
        }
        mnemonicCursor = loopTagDelimiterCursor + 2;
    }
    opcodeInfo = findOpcodeInfoByMnemonic(mnemonicCursor, newSyntax);
    if (!opcodeInfo) {
        reportUserMessage("vcl can't find instruction '%s'\n", mnemonicCursor);
        goto fail;
    }
    cur++;
    decodedInstruction = new Instr(opcodeInfo);
    if (loopid < 0) {
        reportUserMessage("parsed loop id is <0\n");
        goto fail;
    }
    decodedInstruction->parsedLoopId = loopid;
    decodedInstruction->instructionModifierBits = mods;
    decodedInstruction->tildePrefix = tilde;
    mnemonicLen = checkedStringLength(opcodeInfo->mnemonic);
    {
        char *suffix = mnemonicCursor + mnemonicLen;
        bad = false;
        hasMods = parseTrailingComponentMask(suffix, &mask, &bad);
    }
    if (bad) {
        reportUserMessage("instruction has bad .modifier bits\n");
        goto fail;
    }
    switch ((char)opcodeInfo->destinationMaskMode) {
    case 0:
        break;
    case 1:
        if (hasMods) {
            decodedInstruction->destinationMask = mask;
            if (!decodedInstruction->destinationMask) {
                reportUserMessage("getNextInstruction: instruction had zero mod bits\n");
                goto fail;
            }
            forcedMods = true;
            explicitMods = true;
        } else
            decodedInstruction->destinationMask = 15;
        break;
    case 2:
        if (hasMods && mask != 7) {
            reportUserMessage("getNextInstruction: XYZ instruction had bad mod bits\n");
            goto fail;
        }
        forcedMods = true;
        decodedInstruction->destinationMask = 7;
        break;
    case 3:
        if (hasMods) {
            if (decodedInstruction->ramAccessType == 1 && countSetBits(mask) != 1) {
                reportUserMessage("getNextInstruction: MD_ONE instruction had bad mod bits. more than one?\n");
                goto fail;
            }
            explicitMods = true;
            decodedInstruction->destinationMask = mask;
        }
        break;
    default:
        reportError("processor: decode le - bad insn type\n");
        goto fail;
    }
    decodedOperand.immediateText[0] = 0;
    decodedOperand.name[0] = 0;
    decodedOperand.physicalRegisterName[0] = 0;
    decodedOperand.branchLabelText[0] = 0;
    decodedOperand.componentMask = 0;
    decodedOperand.genericOperandForm = 0;
    decodedOperand.forcePhysicalBinding = 0;
    for (operandIndex = 0; operandIndex != decodedInstruction->operandCount; operandIndex++) {
        Operand *opd = &decodedInstruction->operands[operandIndex];
        int next = cur;
        if (opd->isImplicit) {
            if (!buildImplicitOperand(opd->operandKind, &decodedOperand, newSyntax)) {
                reportError("getNextInstruction: failed to build implicit operand\n");
                goto fail;
            }
            if (opd->pairedOperandIndexPlusOne) {
                if (opd->direction != VCL_OPERAND_READ) {
                    reportError("implicit web patch set for non-output\n");
                    goto fail;
                }
                Operand *sourceOperand = &decodedInstruction->operands[opd->pairedOperandIndexPlusOne - 1];
                if (sourceOperand->direction != VCL_OPERAND_WRITE) {
                    reportError("implicit web patched operand not input\n");
                    goto fail;
                }
                if (!sourceOperand->assignedRegister) {
                    reportError("no register found for implicit patch\n");
                    goto fail;
                }
                formatIntoBuffer(decodedOperand.name, "%s", sourceOperand->assignedRegister->registerName());
            }
        } else {
            bool last = false;
            if (operandIndex == decodedInstruction->operandCount - 1 || opd[1].isImplicit)
                last = true;
            opd->sourceTokenStartIndex = cur;
            next = gatherTokens(locationTag, cur, last);
            if (next < 0)
                goto fail;
            if (opcodeInfo->syntheticKind() == 11 && !gatheredToken[0]) {
                gatheredToken[0] = ' ';
                gatheredToken[1] = 0;
            }
            opd->sourceTokenEndIndex = next;
            if (!oDecodeToken(opd->operandKind, gatheredToken, &decodedOperand, newSyntax)) {
                reportUserMessage("failed to decode operand\n");
                goto fail;
            }
        }
        if (decodedOperand.branchLabelText[0]) {
            if (decodedInstruction->branchLabelTags) {
                reportUserMessage("more than one tagged operand associated with instruction at operand %d\n", operandIndex);
                goto fail;
            }
            decodedInstruction->branchLabelTags = dupString(decodedOperand.branchLabelText);
        }
        if (decodedOperand.immediateText[0] && !parseImmediate(decodedOperand.immediateText, opd->immediateDataType, &opd->immediateValue)) {
            reportError("failed to decode an immediate\n");
            goto fail;
        }
        opd->addressComponentMask = decodedOperand.addressComponentMask;
        opd->componentMask = decodedOperand.componentMask;
        if (opd->operandKind == VCL_OP_V_FL4S || opd->operandKind == VCL_OP_C_GPRS || opd->operandKind == VCL_OP_V_FL4 || opd->operandKind == VCL_OP_V_ACC) {
            if (forcedMods) {
                if (opd->componentMask != decodedInstruction->destinationMask && opd->componentMask != VCL_COMPONENT_ALL)
                    reportWarning("getNextInstruction: forced mod bits!\n");
                opd->componentMask = decodedInstruction->destinationMask;
            } else if (opBits < 0) {
                opBits = opd->componentMask;
                decodedInstruction->destinationMask = opBits;
            } else if (opd->componentMask != opBits) {
                reportError("oDecode: bad mod bits in instruction.. not match opBitsStore (%x vs %x) op=%d?\n",
                              opd->componentMask, opBits, operandIndex);
                goto fail;
            }
        }
        if (opd->operandKind == VCL_OP_V_EA_REG || opd->operandKind == VCL_OP_V_EA_IMM_REG) {
            if (explicitMods) {
                if (opd->addressComponentMask) {
                    if (opd->addressComponentMask != decodedInstruction->destinationMask) {
                        reportError("ea/reg or imm(ea)reg has differing mod bits\n");
                        goto fail;
                    }
                } else
                    opd->addressComponentMask = decodedInstruction->destinationMask;
            } else {
                if (!opd->addressComponentMask) {
                    reportError("have to set modbits for ea/reg or imm(ea)reg somewhere\n");
                    goto fail;
                }
                if (opcodeInfo->destinationMaskMode == 3 && countSetBits(opd->addressComponentMask) != 1) {
                    reportError("only one mod field allowed (one of x,y,z, or w)\n");
                    goto fail;
                }
                decodedInstruction->destinationMask = opd->addressComponentMask;
            }
        }
        if (decodedOperand.name[0]) {
            int cls = -1;
            if (opd->operandKind == VCL_OP_V_TBD) {
                if (decodedOperand.genericOperandForm == 1) {
                    if (operandIndex != 2) {
                        reportUserMessage("a scalar can only be the third operand in generic instruction\n");
                        goto fail;
                    }
                    if (asciiCaseEqual(decodedOperand.name, "i")) {
                        cls = 10;
                        opd->operandKind = VCL_OP_V_I;
                    }
                    if (asciiCaseEqual(decodedOperand.name, "q")) {
                        cls = 10;
                        opd->operandKind = VCL_OP_V_Q;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->operandKind = VCL_OP_V_FL1;
                    }
                } else if (decodedOperand.genericOperandForm == 2) {
                    if (operandIndex == 0) {
                        if (asciiCaseEqual(decodedOperand.name, "acc")) {
                            cls = 10;
                            opd->operandKind = VCL_OP_V_ACC;
                        }
                    } else if (asciiCaseEqual(decodedOperand.name, "acc")) {
                        reportUserMessage("ACC can only be first vector operand\n");
                        goto fail;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->operandKind = VCL_OP_V_FL4;
                    }
                    opd->componentMask = decodedInstruction->destinationMask;
                    if (!newSyntax) {
                        reportError("new syntax off while parsing a tbd instruction\n");
                        goto fail;
                    }
                } else {
                    reportUserMessage("couldn't determine operand type for generic instruction\n");
                    goto fail;
                }
            } else
                cls = registerClassForOperandKind(opd->operandKind);
            RegPool *pool = getRegisterPoolByIndex(cls);
            if (!pool) {
                reportError("oDecode: not find expected register ! regtype= '%d' operand = %d\n", cls, operandIndex);
                goto fail;
            }
            expandedRegisterNameCursor = nameBuf;
            if (pool->virtualPartCount && visibilityScopes.tail && !pool->findPhysicalByName(decodedOperand.name) && decodedOperand.name[0] != '_') {
                for (mNode *macroNode = visibilityScopes.tail; macroNode; macroNode = macroNode->previous) {
                    char *macroNameCursor = macroNode->name;
                    if (!macroNameCursor) {
                        reportError("null name in parsep\n");
                        goto fail;
                    }
                    bool found = false;
                    while (*macroNameCursor && !found) {
                        char *operandNameCursor = decodedOperand.name;
                        while (*operandNameCursor && *macroNameCursor && *operandNameCursor == *macroNameCursor) {
                            operandNameCursor++;
                            macroNameCursor++;
                        }
                        if (!*operandNameCursor && *macroNameCursor == ':') {
                            found = true;
                            break;
                        } else {
                            while (*macroNameCursor && *macroNameCursor != ':')
                                macroNameCursor++;
                            if (*macroNameCursor == ':')
                                macroNameCursor++;
                        }
                    }
                    if ((found && macroNode->flags == 0) || (!found && macroNode->flags == 1)) {
                        expandedRegisterNameCursor += formatIntoBuffer(expandedRegisterNameCursor, "%d", (int)(VclPtrWord)macroNode->data);
                        break;
                    }
                }
            }
            formatIntoBuffer(expandedRegisterNameCursor, "%s", decodedOperand.name);
            char *full = dupString(nameBuf);
            RegisterEntry *registerEntry = pool->findOrCreateRegister(full);
            if (!registerEntry) {
                reportUserMessage("oDecode: not find register '%s' from %s\n", full, pool->poolName);
                goto fail;
            }
            if (decodedOperand.forcePhysicalBinding) {
                RegisterEntry *base = 0;
                if (decodedOperand.physicalRegisterName[0]) {
                    base = pool->findPhysicalByName(decodedOperand.physicalRegisterName);
                    if (!base) {
                        reportUserMessage("request to bing non-base register\n");
                        goto fail;
                    }
                    if (registerEntry->assignedPhysicalRegister && registerEntry->assignedPhysicalRegister != base)
                        reportWarning("changing binding for %s from %s to %s\n",
                                      registerEntry->registerName(), registerEntry->assignedPhysicalRegister->registerName(),
                                      base->registerName());
                } else if (registerEntry->assignedPhysicalRegister)
                    base = registerEntry->assignedPhysicalRegister;
                else if (registerEntry->isVirtual != 1)
                    base = registerEntry;
                if (!base) {
                    reportUserMessage(" can't figure out which base register to map %s to\n",
                                  registerEntry->registerName());
                    goto fail;
                }
                if (base->isVirtual == 1) {
                    reportError("what I thought was a base register isn't (%s maps to %s?)\n",
                                  base->registerName(), registerEntry->registerName());
                    goto fail;
                }
                registerEntry->assignedPhysicalRegister = base;
                opd->forcedBaseRegister = base;
            } else if (decodedOperand.physicalRegisterName[0]) {
                RegisterEntry *base = pool->findPhysicalByName(decodedOperand.physicalRegisterName);
                if (!base) {
                    reportUserMessage("failed to find base map reg '%s'\n", decodedOperand.physicalRegisterName);
                    goto fail;
                }
                if (registerEntry->isVirtual != 1 && base != registerEntry) {
                    reportUserMessage("base registers not the same\n");
                    goto fail;
                }
                opd->forcedBaseRegister = base;
            } else if (preferExistingPhysicalRegisters) {
                RegisterEntry *physicalRegister = pool->findPhysicalByName(decodedOperand.name);
                if (physicalRegister && physicalRegister != physicalRegister->ownerPool->zeroRegister)
                    opd->forcedBaseRegister = physicalRegister;
            } else if (registerEntry->isVirtual != 1 && !registerEntry->enabledPhysicalRegister)
                opd->forcedBaseRegister = registerEntry;
            opd->assignedRegister = registerEntry;
            opd->originalRegister = registerEntry;
            if (full)
                delete[] full;
        }
        cur = next;
    }
    if (!decodedInstruction->validateOperands()) {
        reportError("oDecode: sanity check failed\n");
        goto fail;
    }
    if (opcodeInfo->decodeHook && !opcodeInfo->decodeHook(opcodeInfo, decodedInstruction)) {
        reportError("oDecode:: iBuildHook failed to patch instruction\n");
        goto fail;
    }
    if (!decodedInstruction->expandVirtualStatOperand()) {
        reportError("failed to patch status register dependency\n");
        goto fail;
    }
    if (decodedInstruction->normalizeZeroSources() < 0) {
        reportError("failed to normalise instruction\n");
        goto fail;
    }
    parsedSuccessfully = true;
    decodedInstruction->sourceTag = locationTag;
    decodedInstruction->sourceTokenStartIndex = *pos;
    decodedInstruction->sourceTokenEndIndex = cur;
fail:
    if (!parsedSuccessfully) {
        if (decodedInstruction) {
            reportUserMessage("couldn't decode instruction .. problem is at:\n");
            locationTag->dumpSourceLine(cur);
            if (decodedInstruction->emitExpectedSyntax(newSyntax, fmtBuf, expectedSyntaxDiagnosticMode))
                reportUserMessage("expected format is: %s\n", fmtBuf);
            delete decodedInstruction;
            decodedInstruction = 0;
        } else if (locationTag) {
            reportUserMessage("found a syntax error at :\n");
            locationTag->dumpSourceLine(cur);
        }
    } else
        *pos = cur;
    return decodedInstruction;
}

bool Processor::decodeLE(LocTag *locationTag, int *pos, mList *labels, Instr **out, int *nOut)
{
    char buf[0x810];
    char *sourceCursor, *destinationCursor;
    int copyIndex;

    if (!locationTag) {
        reportError("decodeLE: passed a null pointer\n");
        return false;
    }
    if (*pos == 0) {
        char *tok = locationTag->tokenTextAt(0, 0);
        mNode *macroNode;
        if (hasTrailingLabelColon(tok) && !locationTag->isCommentToken(0)) {
            if (*nOut != 0) {
                reportUserMessage("found a label on a continued line\n");
                return false;
            }
            {
                copyIndex = 0;
                sourceCursor = tok;
                destinationCursor = buf;
                for (; *sourceCursor && copyIndex < VCL_TOKEN_COPY_LIMIT; copyIndex++)
                    *destinationCursor++ = *sourceCursor++;
                *destinationCursor = 0;
            }
            int mods = removeInstructionModifierGroups(buf);
            char *labelCursor = buf;
            while (*labelCursor && *labelCursor != ':')
                labelCursor++;
            *labelCursor = 0;
            macroNode = labels->appendNamedNode(buf);
            if (!macroNode) {
                reportError("Processor::decodeLE() - couldn't create tail for label list\n");
                return false;
            }
            macroNode->flags = mods;
            (*pos)++;
            if (getLineTokenCount(locationTag) > 1) {
                char *tok2 = locationTag->tokenTextAt(1, 0);
                if (*tok2 == '[') {
                    {
                        sourceCursor = tok2;
                        destinationCursor = buf + 1;
                        copyIndex = 0;
                        for (; *sourceCursor && copyIndex < VCL_TOKEN_COPY_LIMIT; copyIndex++)
                            *destinationCursor++ = *sourceCursor++;
                        *destinationCursor = 0;
                    }
                    copyIndex = removeInstructionModifierGroups(buf);
                    macroNode->flags |= copyIndex;
                    (*pos)++;
                }
            }
            if (!areInstructionModifiersAllowed(macroNode->flags, decodeMode, true)) {
                int mode = decodeMode;
                reportUserMessage("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(mode));
                locationTag->dumpSourceLine(-1);
                goto fail;
            }
        }
    }
    if (locationTag->hasAssignmentMarker())
        (*pos)++;
next:
    while (*pos < getLineTokenCount(locationTag)) {
        if (!locationTag->isCommentToken(*pos)) {
            if (locationTag->probeNonCommentTokenAndReturnFalse(*pos)) {
                (*pos)++;
                goto done;
            }
            if (*nOut > 1) {
                reportUserMessage("too many instructions on a line!\n");
                locationTag->dumpSourceLine(-1);
                goto fail;
            }
            Instr *ins = getNextInstruction(locationTag, pos);
            if (!ins)
                goto fail;
            out[*nOut] = ins;
            (*nOut)++;
            goto next;
        }
        (*pos)++;
    }
    goto done;
fail:
    return false;
done:
    return true;
}

DecodedLine *decodeSourceLines(Program *program, Parser *parser, int type, bool newSyntax, int *count)
{
    mList labels;
    int nOut;
    Instr *ins[2];
    int pos = 0;
    *count = 0;
    bool decodedSuccessfully = false;
    int decodedLineCount = 0;
    int lineIdx = 0;
    int state = 0;
    int exits = 0;
    int enters = 0;
    int maxLines = VCL_MAX_DECODED_SOURCE_LINES;
    program->proc.sourceBuildType = type;
    program->proc.newSyntax = newSyntax;
    DecodedLine *blocks = 0;

    switch (type) {
    case 1:
        program->proc.decodeMode = 1;
        program->proc.preferExistingPhysicalRegisters = 0;
        break;
    case 2:
        program->proc.decodeMode = 3;
        program->proc.preferExistingPhysicalRegisters = 1;
        break;
    default:
        reportError("Processor::Build:: bad build type request\n");
        goto done;
    }
    program->proc.savedDecodeMode = 0;
    program->proc.rawLoopId = 1;
    program->proc.insideRawLoop = 0;
    blocks = new DecodedLine[VCL_MAX_DECODED_SOURCE_LINES];
    if (!blocks) {
        reportError("Processor::Build() new failed for decLine table\n");
        goto done;
    }
    pos = 0;
    while (lineIdx < parser->getParsedLineCount()) {
        LocTag *locationTag = parser->getParsedLine(lineIdx);
        if (!locationTag) {
            reportError("NULL LE return\n");
            goto done;
        }
        if (locationTag->hasAssignmentMarker()) {
            reportUserMessage("unexpected line continue\n");
            goto done;
        }
        if (locationTag->isRawMacroBlock) {
            DecodedLine *decodedLine = &blocks[decodedLineCount];
            decodedLine->decodeMode = 6;
            char *copiedLineText = dupString(locationTag->lineText);
            if (!copiedLineText) {
                reportError("failed to dupe the pass thru text\n");
                goto done;
            }
            decodedLine->passThroughText = copiedLineText;
            decodedLineCount++;
            lineIdx++;
            continue;
        }
        if (locationTag->startsWithDirective()) {
            Directive *currentDirective = findRegisteredDirective(locationTag);
            if (!currentDirective) {
                reportUserMessage("failed to find a directive from line : \n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            if (!currentDirective->fn(program, locationTag)) {
                reportUserMessage("a directive parser failed at line:\n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            lineIdx++;
            continue;
        }
        nOut = 0;
        do {
            locationTag = parser->getParsedLine(lineIdx);
            if (!locationTag) {
                reportError("got a null return.. while expanding line continues\n");
                goto done;
            }
            int old = pos;
            if (!program->proc.decodeLE(locationTag, &pos, &labels, ins, &nOut)) {
                reportError("failed to build list of instructions for line\n");
                goto done;
            }
            if (pos >= getLineTokenCount(locationTag)) {
                lineIdx++;
                pos = 0;
            } else if (old >= pos) {
                reportError("didn't advance start token during processing\n");
                goto done;
            }
        } while (parser->isAssignmentLine(lineIdx));
        if (!nOut)
            continue;
        if (decodedLineCount < 0) {
            reportError("out of bounds rLine getting decoded instructions\n");
            goto done;
        }
        if (decodedLineCount >= maxLines) {
            reportError("out of bounds rLine getting decoded instructions\n");
            goto done;
        }
        DecodedLine *decodedBlock = &blocks[decodedLineCount];
        decodedBlock->initialize();
        if (!decodedBlock->labels.transferNodesFrom(&labels)) {
            reportError("failed to grab labels from current list\n");
            goto done;
        }
        bool synth = false;
        for (int decodedInstructionIndex = 0; decodedInstructionIndex != nOut; decodedInstructionIndex++) {
            if (!ins[decodedInstructionIndex]) {
                reportError("instruction decoder appears to have failed?\n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            if (ins[decodedInstructionIndex]->syntheticKind) {
                if (nOut != 1) {
                    reportUserMessage("only one synthetic allowed per line\n");
                    locationTag->dumpSourceLine(-1);
                    goto done;
                }
                synth = true;
            }
            if (!areInstructionModifiersAllowed(ins[decodedInstructionIndex]->instructionModifierBits, program->proc.decodeMode, false)) {
                reportUserMessage("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(program->proc.decodeMode));
                ins[decodedInstructionIndex]->dumpInstructionContext();
                goto done;
            }
        }
        decodedBlock->decodeMode = program->proc.decodeMode;
        decodedBlock->decodeSubMode = program->proc.decodeSubMode;
        if (synth) {
            Instr *firstInstruction = ins[0];
            decodedBlock->instructions[0] = firstInstruction;
            if (!firstInstruction) {
                reportError("during the initial build, found an iEntry, which thought it was synthetic, but had no instruction\n");
                goto done;
            }
            switch ((unsigned char)firstInstruction->syntheticKind) {
            case 1:
                if (state) {
                    reportUserMessage("encountered enter directive in bad context\n");
                    goto done;
                }
                enters++;
                state = 1;
                break;
            case 2:
                if (state != 1) {
                    reportUserMessage("ValidateSynthetics: encountered an end enter directive, but not in enter context\n");
                    goto done;
                }
                state = 0;
                break;
            case 3:
            case 11:
                if (state) {
                    reportUserMessage("encountered exit directive in bad context\n");
                    goto done;
                }
                exits++;
                state = 2;
                break;
            case 4:
                if (state != 2) {
                    reportUserMessage("encountered a end enter directive, but not in enter context\n");
                    goto done;
                }
                state = 0;
                break;
            case 5:
                if (state) {
                    reportUserMessage("encountered cont directive in bad context..mode=%d\n", state);
                    goto done;
                }
                exits++;
                break;
            case 6:
                if (state != 1) {
                    reportUserMessage("encountered an input directive, but not in enter context\n");
                    goto done;
                }
                break;
            case 7:
                if (state != 2) {
                    reportUserMessage("encountered an output directive, but not in enter context\n");
                    goto done;
                }
                break;
            case 9:
            case 12:
                if (decodedBlock->decodeMode == 3) {
                    reportUserMessage("a loop size directive is an invalid directive for raw mode vsm..\n");
                    goto done;
                }
                /* fall through */
            case 8:
                if (program->proc.insideRawLoop)
                    decodedBlock->rawLoopId = program->proc.rawLoopId;
                break;
            case 10:
                break;
            default:
                reportError("iStream::ValidateSynthetics() - unhandled/unexpected synthetic type (%d)\n",
                              (unsigned char)firstInstruction->syntheticKind);
                firstInstruction->dumpOperandDiagnostics(false);
                goto done;
            }
        } else if (program->proc.decodeMode == 3) {
            if (nOut != program->proc.instructionsPerRawLine) {
                reportUserMessage("in RAW decode mode, there weren't %d instructions on line : \n", program->proc.instructionsPerRawLine);
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            if (program->proc.insideRawLoop)
                decodedBlock->rawLoopId = program->proc.rawLoopId;
            for (int instructionIndex = 0; instructionIndex != program->proc.instructionsPerRawLine; instructionIndex++)
                decodedBlock->instructions[instructionIndex] = ins[instructionIndex];
        } else {
            if (program->proc.insideRawLoop) {
                reportError("Build() - was in loop mode during uta\n");
                goto done;
            }
            if (nOut != 1) {
                reportUserMessage("Build() - more than one instruction on the line in vcl mode\n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            Instr *decodedInstruction = ins[0];
            if (!decodedInstruction) {
                reportError("decode vcl - instruction ptr was null.. when I thought it wasn't\n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            if (decodedInstruction->rawOnly) {
                reportUserMessage("vcl: instruction '%s' is unsupported, except in RAW mode.\n", decodedInstruction->opcodeInfo->mnemonic);
                decodedInstruction->dumpInstructionContext();
                delete decodedInstruction;
                goto done;
            }
            decodedBlock->instructions[0] = decodedInstruction;
        }
        if (decodedBlock->decodeMode == 1) {
            Instr *branchInstruction = decodedBlock->instructions[0];
            if (branchInstruction && ((branchInstruction->branchFlags >> 2) & 1)) {
                int branchSimplificationResult = branchInstruction->simplifyBranch();
                if (branchSimplificationResult < 0) {
                    reportError("failed to fixup a branch\n");
                    goto done;
                }
                if (branchSimplificationResult == 0) {
                    branchInstruction->dumpInstructionContext();
                    decodedBlock->instructions[0] = 0;
                    delete branchInstruction;
                    branchInstruction = 0;
                }
                if (branchSimplificationResult == 2)
                    branchInstruction->dumpInstructionContext();
            }
        }
        decodedLineCount++;
    }
    if (labels.nodeCount) {
        if (decodedLineCount < 0) {
            reportError("out of bounds rLine extending table\n");
            goto done;
        }
        if (decodedLineCount >= maxLines) {
            reportError("out of bounds rLine extending table\n");
            goto done;
        }
        reportWarning("gathering labels at end of code into the final namelist\n");
        if (!program->proc.trailingLabels.transferNodesFrom(&labels)) {
            reportError("failed to grab ending labels\n");
            goto done;
        }
        for (mNode *macroNode = program->proc.trailingLabels.head; macroNode; macroNode = macroNode->next) {
            if (macroNode->flags) {
                reportUserMessage("can't attach [modifiers] to labels at the end of code\n");
                reportUserMessage("Label '%s:' has '%s'\n", macroNode->name, formatInstructionModifiers(macroNode->flags));
                goto done;
            }
        }
    }
    if (state) {
        reportError("iStream::Validate - left stream - but not in null state\n");
        goto done;
    }
    if (!enters) {
        reportError("iStream: no enter directives!\n");
        goto done;
    }
    if (!exits) {
        reportError("iStream: no exit directives\n");
        goto done;
    }
    for (int lineIndex = 0; lineIndex != decodedLineCount - 1; lineIndex++) {
        mList *macroList = &blocks[lineIndex].labels;
        if (!macroList->head)
            continue;
        for (int compareLineIndex = lineIndex + 1; compareLineIndex != decodedLineCount; compareLineIndex++) {
            mList *labelList = &blocks[compareLineIndex].labels;
            if (labelList->head) {
                for (mNode *labelNode = macroList->head; labelNode; labelNode = labelNode->next) {
                    if (labelList->findByNameIgnoreCase(labelNode->name)) {
                        reportUserMessage("found at least two uses of the label '%s' in the source\n", labelNode->name);
                        goto done;
                    }
                }
            }
        }
    }
    *count = decodedLineCount;
    decodedSuccessfully = true;
done:
    if (!decodedSuccessfully) {
        delete[] blocks;
        blocks = 0;
        *count = 0;
    } else if (program->proc.visibilityScopes.nodeCount)
        reportWarning("scope stack open (depth %d) at end of source\n", program->proc.visibilityScopes.nodeCount);
    program->proc.visibilityScopes.clearNodes();
    return blocks;
}

int gatherTokens(LocTag *locationTag, int firstTokenIndex, bool stopAtOpcode)
{
    char *outputCursor = gatheredToken;
    int tokenCount = 0;
    char sep;

    for (; firstTokenIndex < getLineTokenCount(locationTag); firstTokenIndex++) {
        char *tok = locationTag->tokenTextAt(firstTokenIndex, &sep);
        if (!tok) {
            reportError("gatherTokens() - failed to get token\n");
            return -1;
        }
        if (!locationTag->isCommentToken(firstTokenIndex)) {
            if (locationTag->probeNonCommentTokenAndReturnFalse(firstTokenIndex)) {
                *outputCursor = 0;
                return firstTokenIndex;
            }
            if (tokenCount && stopAtOpcode) {
                for (char *tokenCursor = tok; *tokenCursor; tokenCursor++)
                    if (tokenCursor[0] == '.' && tokenCursor[1] == '.') {
                        *outputCursor = 0;
                        return firstTokenIndex;
                    }
            }
            if (tokenCount && stopAtOpcode && findOpcodeInfoByMnemonic(tok, false)) {
                *outputCursor = 0;
                return firstTokenIndex;
            }
            tokenCount++;
            while (*tok) {
                if ((unsigned)(outputCursor - gatheredToken) > 0x3fe) {
                    reportUserMessage("gatherTokens: the gathered token became too long!\n");
                    return -1;
                }
                *outputCursor++ = *tok++;
                *outputCursor = 0;
            }
        }
        if (sep)
            break;
    }
    *outputCursor = 0;
    return firstTokenIndex + 1;
}
