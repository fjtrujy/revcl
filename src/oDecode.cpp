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
int mstrlen(const char *s);
bool oDecodeToken(int kind, char *tok, ImplicitOp *out, bool newSyntax);
bool parseImmediate(const char *s, unsigned char type, mValue *v);
RegPool *GetRP(unsigned t);
int expectedSyntaxDiagnosticMode = -1;

Instr *Processor::getNextInstruction(LocTag *locationTag, int *pos)
{
    int copyCount;
    ImplicitOp decodedOperand;
    char buf[0x801];
    char nameBuf[0x400];
    char fmtBuf[0x400];
    int cur = *pos;
    short loopid;
    bool explicitMods = false;
    loopid = 0;
    copyCount = 0;
    bool forcedMods = false;
    bool ok = false;
    int opBits = -1;
    Instr *decodedInstruction = 0;
    bool bad = false;
    int mask;
    iInfo *info;
    int mods;
    char tilde = 0;
    int operandIndex;
    char *s = 0;
    int mnemonicLen;
    char *p;
    char *q;
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
        char *s = locationTag->tokenTextAt(cur);
        char *d = buf;
        p = d;
        for (; *s && copyCount < 0x800; copyCount++)
            *d++ = *s++;
        *d = 0;
    }
    mods = removeInstructionModifierGroups(p);
    q = p;
    if (buf[0] == '~') {
        q = buf + 1;
        tilde = 1;
        p = q;
    }
    while (*q && !(q[0] == '.' && q[1] == '.'))
        q++;
    if (*q == '.') {
        if (*p == '+')
            loopid = 0x1024;
        else {
            loopid = parseInteger(p);
            if (loopid > 32) {
                reportUserMessage("unlinkely large loopid set with instruction\n");
                goto fail;
            }
        }
        p = q + 2;
    }
    info = findOpcodeInfoByMnemonic(p, newSyntax);
    if (!info) {
        reportUserMessage("vcl can't find instruction '%s'\n", p);
        goto fail;
    }
    cur++;
    decodedInstruction = new Instr(info);
    if (loopid < 0) {
        reportUserMessage("parsed loop id is <0\n");
        goto fail;
    }
    decodedInstruction->parsedLoopId = loopid;
    decodedInstruction->instructionModifierBits = mods;
    decodedInstruction->tildePrefix = tilde;
    mnemonicLen = mstrlen(info->mnemonic);
    {
        char *suffix = p + mnemonicLen;
        bad = false;
        hasMods = parseTrailingComponentMask(suffix, &mask, &bad);
    }
    if (bad) {
        reportUserMessage("instruction has bad .modifier bits\n");
        goto fail;
    }
    switch ((char)info->destinationMaskMode) {
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
                if (opd->direction != 1) {
                    reportError("implicit web patch set for non-output\n");
                    goto fail;
                }
                Operand *src = &decodedInstruction->operands[opd->pairedOperandIndexPlusOne - 1];
                if (src->direction != 2) {
                    reportError("implicit web patched operand not input\n");
                    goto fail;
                }
                if (!src->assignedRegister) {
                    reportError("no register found for implicit patch\n");
                    goto fail;
                }
                formatIntoBuffer(decodedOperand.name, "%s", src->assignedRegister->registerName());
            }
        } else {
            bool last = false;
            if (operandIndex == decodedInstruction->operandCount - 1 || opd[1].isImplicit)
                last = true;
            opd->sourceTokenStartIndex = cur;
            next = gatherTokens(locationTag, cur, last);
            if (next < 0)
                goto fail;
            if (info->syntheticKind() == 11 && !gatheredToken[0]) {
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
        if (opd->operandKind == 35 || opd->operandKind == 42 || opd->operandKind == 1 || opd->operandKind == 9) {
            if (forcedMods) {
                if (opd->componentMask != decodedInstruction->destinationMask && opd->componentMask != 15)
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
        if (opd->operandKind == 30 || opd->operandKind == 31) {
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
                if (info->destinationMaskMode == 3 && countSetBits(opd->addressComponentMask) != 1) {
                    reportError("only one mod field allowed (one of x,y,z, or w)\n");
                    goto fail;
                }
                decodedInstruction->destinationMask = opd->addressComponentMask;
            }
        }
        if (decodedOperand.name[0]) {
            int cls = -1;
            if (opd->operandKind == 38) {
                if (decodedOperand.genericOperandForm == 1) {
                    if (operandIndex != 2) {
                        reportUserMessage("a scalar can only be the third operand in generic instruction\n");
                        goto fail;
                    }
                    if (asciiCaseEqual(decodedOperand.name, "i")) {
                        cls = 10;
                        opd->operandKind = 4;
                    }
                    if (asciiCaseEqual(decodedOperand.name, "q")) {
                        cls = 10;
                        opd->operandKind = 7;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->operandKind = 3;
                    }
                } else if (decodedOperand.genericOperandForm == 2) {
                    if (operandIndex == 0) {
                        if (asciiCaseEqual(decodedOperand.name, "acc")) {
                            cls = 10;
                            opd->operandKind = 9;
                        }
                    } else if (asciiCaseEqual(decodedOperand.name, "acc")) {
                        reportUserMessage("ACC can only be first vector operand\n");
                        goto fail;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->operandKind = 1;
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
            RegPool *pool = GetRP(cls);
            if (!pool) {
                reportError("oDecode: not find expected register ! regtype= '%d' operand = %d\n", cls, operandIndex);
                goto fail;
            }
            s = nameBuf;
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
                        s += formatIntoBuffer(s, "%d", (int)(VclPtrWord)macroNode->data);
                        break;
                    }
                }
            }
            formatIntoBuffer(s, "%s", decodedOperand.name);
            char *full = dupString(nameBuf);
            RegPoolEntry *registerEntry = pool->findOrCreateRegister(full);
            if (!registerEntry) {
                reportUserMessage("oDecode: not find register '%s' from %s\n", full, pool->poolName);
                goto fail;
            }
            if (decodedOperand.forcePhysicalBinding) {
                RegPoolEntry *base = 0;
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
                RegPoolEntry *base = pool->findPhysicalByName(decodedOperand.physicalRegisterName);
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
                RegPoolEntry *physicalRegister = pool->findPhysicalByName(decodedOperand.name);
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
    if (info->decodeHook && !info->decodeHook(info, decodedInstruction)) {
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
    ok = true;
    decodedInstruction->sourceTag = locationTag;
    decodedInstruction->sourceTokenStartIndex = *pos;
    decodedInstruction->sourceTokenEndIndex = cur;
fail:
    if (!ok) {
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
    int i;

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
                i = 0;
                sourceCursor = tok;
                destinationCursor = buf;
                for (; *sourceCursor && i < 0x800; i++)
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
                        i = 0;
                        for (; *sourceCursor && i < 0x800; i++)
                            *destinationCursor++ = *sourceCursor++;
                        *destinationCursor = 0;
                    }
                    i = removeInstructionModifierGroups(buf);
                    macroNode->flags |= i;
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
    bool ok = false;
    int n = 0;
    int lineIdx = 0;
    int state = 0;
    int exits = 0;
    int enters = 0;
    int maxLines = 0x1000;
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
    blocks = new DecodedLine[0x1000];
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
            DecodedLine *decodedLine = &blocks[n];
            decodedLine->decodeMode = 6;
            char *copiedLineText = dupString(locationTag->lineText);
            if (!copiedLineText) {
                reportError("failed to dupe the pass thru text\n");
                goto done;
            }
            decodedLine->passThroughText = copiedLineText;
            n++;
            lineIdx++;
            continue;
        }
        if (locationTag->startsWithDirective()) {
            Directive *d = findRegisteredDirective(locationTag);
            if (!d) {
                reportUserMessage("failed to find a directive from line : \n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            if (!d->fn(program, locationTag)) {
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
        if (n < 0) {
            reportError("out of bounds rLine getting decoded instructions\n");
            goto done;
        }
        if (n >= maxLines) {
            reportError("out of bounds rLine getting decoded instructions\n");
            goto done;
        }
        DecodedLine *decodedBlock = &blocks[n];
        decodedBlock->initialize();
        if (!decodedBlock->labels.transferNodesFrom(&labels)) {
            reportError("failed to grab labels from current list\n");
            goto done;
        }
        bool synth = false;
        for (int i = 0; i != nOut; i++) {
            if (!ins[i]) {
                reportError("instruction decoder appears to have failed?\n");
                locationTag->dumpSourceLine(-1);
                goto done;
            }
            if (ins[i]->syntheticKind) {
                if (nOut != 1) {
                    reportUserMessage("only one synthetic allowed per line\n");
                    locationTag->dumpSourceLine(-1);
                    goto done;
                }
                synth = true;
            }
            if (!areInstructionModifiersAllowed(ins[i]->instructionModifierBits, program->proc.decodeMode, false)) {
                reportUserMessage("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(program->proc.decodeMode));
                ins[i]->leDump();
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
                decodedInstruction->leDump();
                delete decodedInstruction;
                goto done;
            }
            decodedBlock->instructions[0] = decodedInstruction;
        }
        if (decodedBlock->decodeMode == 1) {
            Instr *branchInstruction = decodedBlock->instructions[0];
            if (branchInstruction && ((branchInstruction->branchFlags >> 2) & 1)) {
                int r = branchInstruction->simplifyBranch();
                if (r < 0) {
                    reportError("failed to fixup a branch\n");
                    goto done;
                }
                if (r == 0) {
                    branchInstruction->leDump();
                    decodedBlock->instructions[0] = 0;
                    delete branchInstruction;
                    branchInstruction = 0;
                }
                if (r == 2)
                    branchInstruction->leDump();
            }
        }
        n++;
    }
    if (labels.nodeCount) {
        if (n < 0) {
            reportError("out of bounds rLine extending table\n");
            goto done;
        }
        if (n >= maxLines) {
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
    for (int i = 0; i != n - 1; i++) {
        mList *macroList = &blocks[i].labels;
        if (!macroList->head)
            continue;
        for (int j = i + 1; j != n; j++) {
            mList *labelList = &blocks[j].labels;
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
    *count = n;
    ok = true;
done:
    if (!ok) {
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
    int n = 0;
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
            if (n && stopAtOpcode) {
                for (char *tokenCursor = tok; *tokenCursor; tokenCursor++)
                    if (tokenCursor[0] == '.' && tokenCursor[1] == '.') {
                        *outputCursor = 0;
                        return firstTokenIndex;
                    }
            }
            if (n && stopAtOpcode && findOpcodeInfoByMnemonic(tok, false)) {
                *outputCursor = 0;
                return firstTokenIndex;
            }
            n++;
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
