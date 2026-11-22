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
int func_0804bf50(unsigned int x);
bool asciiCaseEqual(const char *a, const char *b);
char *dupString(const char *s);
int mstrlen(const char *s);
bool oDecodeToken(int kind, char *tok, ImplicitOp *out, bool newSyntax);
bool parseImmediate(const char *s, unsigned char type, mValue *v);
RegPool *GetRP(unsigned t);
int g_080b34dc = -1;

Instr *Processor::getNextInstruction(LocTag *t, int *pos)
{
    int copyCount;
    ImplicitOp op;
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
    Instr *in = 0;
    bool bad = false;
    int mask;
    iInfo *info;
    int mods;
    char tilde = 0;
    int i;
    char *s = 0;
    int mnemonicLen;
    char *p;
    char *q;
    bool hasMods;

    if (!t) {
        reportError("getNextInstruction - passed null pointer\n");
        goto fail;
    }
    if (t->isCommentToken(cur)) {
        reportError("getNextInstruction - tried to parse a comment\n");
        goto fail;
    }
    {
        char *s = t->tokenTextAt(cur);
        char *d = buf;
        p = d;
        for (; *s && copyCount < 0x800; copyCount++)
            *d++ = *s++;
        *d = 0;
    }
    mods = func_080521f0(p);
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
    in = new Instr(info);
    if (loopid < 0) {
        reportUserMessage("parsed loop id is <0\n");
        goto fail;
    }
    in->parsedLoopId = loopid;
    in->instructionModifierBits = mods;
    in->tildePrefix = tilde;
    mnemonicLen = mstrlen(info->mnemonic);
    {
        char *suffix = p + mnemonicLen;
        bad = false;
        hasMods = func_08051f08(suffix, &mask, &bad);
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
            in->destinationMask = mask;
            if (!in->destinationMask) {
                reportUserMessage("getNextInstruction: instruction had zero mod bits\n");
                goto fail;
            }
            forcedMods = true;
            explicitMods = true;
        } else
            in->destinationMask = 15;
        break;
    case 2:
        if (hasMods && mask != 7) {
            reportUserMessage("getNextInstruction: XYZ instruction had bad mod bits\n");
            goto fail;
        }
        forcedMods = true;
        in->destinationMask = 7;
        break;
    case 3:
        if (hasMods) {
            if (in->ramAccessType == 1 && func_0804bf50(mask) != 1) {
                reportUserMessage("getNextInstruction: MD_ONE instruction had bad mod bits. more than one?\n");
                goto fail;
            }
            explicitMods = true;
            in->destinationMask = mask;
        }
        break;
    default:
        reportError("processor: decode le - bad insn type\n");
        goto fail;
    }
    op.unk_1000[0] = 0;
    op.name[0] = 0;
    op.unk_800[0] = 0;
    op.unk_1800[0] = 0;
    op.unk_2000 = 0;
    op.unk_2008 = 0;
    op.unk_200c = 0;
    for (i = 0; i != in->operandCount; i++) {
        Operand *opd = &in->operands[i];
        int next = cur;
        if (opd->isImplicit) {
            if (!buildImplicitOperand(opd->operandKind, &op, newSyntax)) {
                reportError("getNextInstruction: failed to build implicit operand\n");
                goto fail;
            }
            if (opd->pairedOperandIndexPlusOne) {
                if (opd->direction != 1) {
                    reportError("implicit web patch set for non-output\n");
                    goto fail;
                }
                Operand *src = &in->operands[opd->pairedOperandIndexPlusOne - 1];
                if (src->direction != 2) {
                    reportError("implicit web patched operand not input\n");
                    goto fail;
                }
                if (!src->assignedRegister) {
                    reportError("no register found for implicit patch\n");
                    goto fail;
                }
                formatIntoBuffer(op.name, "%s", src->assignedRegister->registerName());
            }
        } else {
            bool last = false;
            if (i == in->operandCount - 1 || opd[1].isImplicit)
                last = true;
            opd->unk_24 = cur;
            next = gatherTokens(t, cur, last);
            if (next < 0)
                goto fail;
            if (info->syntheticKind() == 11 && !gatheredToken[0]) {
                gatheredToken[0] = ' ';
                gatheredToken[1] = 0;
            }
            opd->unk_25 = next;
            if (!oDecodeToken(opd->operandKind, gatheredToken, &op, newSyntax)) {
                reportUserMessage("failed to decode operand\n");
                goto fail;
            }
        }
        if (op.unk_1800[0]) {
            if (in->branchLabelTags) {
                reportUserMessage("more than one tagged operand associated with instruction at operand %d\n", i);
                goto fail;
            }
            in->branchLabelTags = dupString(op.unk_1800);
        }
        if (op.unk_1000[0] && !parseImmediate(op.unk_1000, opd->unk_05, &opd->immediateValue)) {
            reportError("failed to decode an immediate\n");
            goto fail;
        }
        opd->unk_0c = op.unk_2004;
        opd->componentMask = op.unk_2000;
        if (opd->operandKind == 35 || opd->operandKind == 42 || opd->operandKind == 1 || opd->operandKind == 9) {
            if (forcedMods) {
                if (opd->componentMask != in->destinationMask && opd->componentMask != 15)
                    reportWarning("getNextInstruction: forced mod bits!\n");
                opd->componentMask = in->destinationMask;
            } else if (opBits < 0) {
                opBits = opd->componentMask;
                in->destinationMask = opBits;
            } else if (opd->componentMask != opBits) {
                reportError("oDecode: bad mod bits in instruction.. not match opBitsStore (%x vs %x) op=%d?\n",
                              opd->componentMask, opBits, i);
                goto fail;
            }
        }
        if (opd->operandKind == 30 || opd->operandKind == 31) {
            if (explicitMods) {
                if (opd->unk_0c) {
                    if (opd->unk_0c != in->destinationMask) {
                        reportError("ea/reg or imm(ea)reg has differing mod bits\n");
                        goto fail;
                    }
                } else
                    opd->unk_0c = in->destinationMask;
            } else {
                if (!opd->unk_0c) {
                    reportError("have to set modbits for ea/reg or imm(ea)reg somewhere\n");
                    goto fail;
                }
                if (info->destinationMaskMode == 3 && func_0804bf50(opd->unk_0c) != 1) {
                    reportError("only one mod field allowed (one of x,y,z, or w)\n");
                    goto fail;
                }
                in->destinationMask = opd->unk_0c;
            }
        }
        if (op.name[0]) {
            int cls = -1;
            if (opd->operandKind == 38) {
                if (op.unk_2008 == 1) {
                    if (i != 2) {
                        reportUserMessage("a scalar can only be the third operand in generic instruction\n");
                        goto fail;
                    }
                    if (asciiCaseEqual(op.name, "i")) {
                        cls = 10;
                        opd->operandKind = 4;
                    }
                    if (asciiCaseEqual(op.name, "q")) {
                        cls = 10;
                        opd->operandKind = 7;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->operandKind = 3;
                    }
                } else if (op.unk_2008 == 2) {
                    if (i == 0) {
                        if (asciiCaseEqual(op.name, "acc")) {
                            cls = 10;
                            opd->operandKind = 9;
                        }
                    } else if (asciiCaseEqual(op.name, "acc")) {
                        reportUserMessage("ACC can only be first vector operand\n");
                        goto fail;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->operandKind = 1;
                    }
                    opd->componentMask = in->destinationMask;
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
                reportError("oDecode: not find expected register ! regtype= '%d' operand = %d\n", cls, i);
                goto fail;
            }
            s = nameBuf;
            if (pool->virtualPartCount && visibilityScopes.tail && !pool->findPhysicalByName(op.name) && op.name[0] != '_') {
                for (mNode *n = visibilityScopes.tail; n; n = n->previous) {
                    char *np = n->name;
                    if (!np) {
                        reportError("null name in parsep\n");
                        goto fail;
                    }
                    bool found = false;
                    while (*np && !found) {
                        char *r = op.name;
                        while (*r && *np && *r == *np) {
                            r++;
                            np++;
                        }
                        if (!*r && *np == ':') {
                            found = true;
                            break;
                        } else {
                            while (*np && *np != ':')
                                np++;
                            if (*np == ':')
                                np++;
                        }
                    }
                    if ((found && n->flags == 0) || (!found && n->flags == 1)) {
                        s += formatIntoBuffer(s, "%d", (int)(VclPtrWord)n->data);
                        break;
                    }
                }
            }
            formatIntoBuffer(s, "%s", op.name);
            char *full = dupString(nameBuf);
            RegPoolEntry *r = pool->findOrCreateRegister(full);
            if (!r) {
                reportUserMessage("oDecode: not find register '%s' from %s\n", full, pool->poolName);
                goto fail;
            }
            if (op.unk_200c) {
                RegPoolEntry *base = 0;
                if (op.unk_800[0]) {
                    base = pool->findPhysicalByName(op.unk_800);
                    if (!base) {
                        reportUserMessage("request to bing non-base register\n");
                        goto fail;
                    }
                    if (r->assignedPhysicalRegister && r->assignedPhysicalRegister != base)
                        reportWarning("changing binding for %s from %s to %s\n",
                                      r->registerName(), r->assignedPhysicalRegister->registerName(),
                                      base->registerName());
                } else if (r->assignedPhysicalRegister)
                    base = r->assignedPhysicalRegister;
                else if (r->isVirtual != 1)
                    base = r;
                if (!base) {
                    reportUserMessage(" can't figure out which base register to map %s to\n",
                                  r->registerName());
                    goto fail;
                }
                if (base->isVirtual == 1) {
                    reportError("what I thought was a base register isn't (%s maps to %s?)\n",
                                  base->registerName(), r->registerName());
                    goto fail;
                }
                r->assignedPhysicalRegister = base;
                opd->forcedBaseRegister = base;
            } else if (op.unk_800[0]) {
                RegPoolEntry *base = pool->findPhysicalByName(op.unk_800);
                if (!base) {
                    reportUserMessage("failed to find base map reg '%s'\n", op.unk_800);
                    goto fail;
                }
                if (r->isVirtual != 1 && base != r) {
                    reportUserMessage("base registers not the same\n");
                    goto fail;
                }
                opd->forcedBaseRegister = base;
            } else if (unk_65) {
                RegPoolEntry *e = pool->findPhysicalByName(op.name);
                if (e && e != e->ownerPool->zeroRegister)
                    opd->forcedBaseRegister = e;
            } else if (r->isVirtual != 1 && !r->unk_16)
                opd->forcedBaseRegister = r;
            opd->assignedRegister = r;
            opd->originalRegister = r;
            if (full)
                delete[] full;
        }
        cur = next;
    }
    if (!in->validateOperands()) {
        reportError("oDecode: sanity check failed\n");
        goto fail;
    }
    if (info->decodeHook && !info->decodeHook(info, in)) {
        reportError("oDecode:: iBuildHook failed to patch instruction\n");
        goto fail;
    }
    if (!in->expandVirtualStatOperand()) {
        reportError("failed to patch status register dependency\n");
        goto fail;
    }
    if (in->normalizeZeroSources() < 0) {
        reportError("failed to normalise instruction\n");
        goto fail;
    }
    ok = true;
    in->unk_14 = t;
    in->unk_18 = *pos;
    in->unk_1a = cur;
fail:
    if (!ok) {
        if (in) {
            reportUserMessage("couldn't decode instruction .. problem is at:\n");
            t->dumpSourceLine(cur);
            if (in->emitExpectedSyntax(newSyntax, fmtBuf, g_080b34dc))
                reportUserMessage("expected format is: %s\n", fmtBuf);
            delete in;
            in = 0;
        } else if (t) {
            reportUserMessage("found a syntax error at :\n");
            t->dumpSourceLine(cur);
        }
    } else
        *pos = cur;
    return in;
}

bool Processor::decodeLE(LocTag *t, int *pos, mList *labels, Instr **out, int *nOut)
{
    char buf[0x810];
    char *s, *d;
    int i;

    if (!t) {
        reportError("decodeLE: passed a null pointer\n");
        return false;
    }
    if (*pos == 0) {
        char *tok = t->tokenTextAt(0, 0);
        mNode *n;
        if (func_080506e0(tok) && !t->isCommentToken(0)) {
            if (*nOut != 0) {
                reportUserMessage("found a label on a continued line\n");
                return false;
            }
            {
                i = 0;
                s = tok;
                d = buf;
                for (; *s && i < 0x800; i++)
                    *d++ = *s++;
                *d = 0;
            }
            int mods = func_080521f0(buf);
            char *p = buf;
            while (*p && *p != ':')
                p++;
            *p = 0;
            n = labels->appendNamedNode(buf);
            if (!n) {
                reportError("Processor::decodeLE() - couldn't create tail for label list\n");
                return false;
            }
            n->flags = mods;
            (*pos)++;
            if (getLineTokenCount(t) > 1) {
                char *tok2 = t->tokenTextAt(1, 0);
                if (*tok2 == '[') {
                    {
                        s = tok2;
                        d = buf + 1;
                        i = 0;
                        for (; *s && i < 0x800; i++)
                            *d++ = *s++;
                        *d = 0;
                    }
                    i = func_080521f0(buf);
                    n->flags |= i;
                    (*pos)++;
                }
            }
            if (!func_080522c4(n->flags, decodeMode, true)) {
                int mode = decodeMode;
                reportUserMessage("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(mode));
                t->dumpSourceLine(-1);
                goto fail;
            }
        }
    }
    if (t->hasAssignmentMarker())
        (*pos)++;
next:
    while (*pos < getLineTokenCount(t)) {
        if (!t->isCommentToken(*pos)) {
            if (t->func_08050f7c(*pos)) {
                (*pos)++;
                goto done;
            }
            if (*nOut > 1) {
                reportUserMessage("too many instructions on a line!\n");
                t->dumpSourceLine(-1);
                goto fail;
            }
            Instr *ins = getNextInstruction(t, pos);
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

DecodedLine *decodeSourceLines(Program *p, Parser *parser, int type, bool newSyntax, int *count)
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
    p->proc.unk_64 = type;
    p->proc.newSyntax = newSyntax;
    DecodedLine *blocks = 0;

    switch (type) {
    case 1:
        p->proc.decodeMode = 1;
        p->proc.unk_65 = 0;
        break;
    case 2:
        p->proc.decodeMode = 3;
        p->proc.unk_65 = 1;
        break;
    default:
        reportError("Processor::Build:: bad build type request\n");
        goto done;
    }
    p->proc.savedDecodeMode = 0;
    p->proc.rawLoopId = 1;
    p->proc.insideRawLoop = 0;
    blocks = new DecodedLine[0x1000];
    if (!blocks) {
        reportError("Processor::Build() new failed for decLine table\n");
        goto done;
    }
    pos = 0;
    while (lineIdx < parser->getParsedLineCount()) {
        LocTag *t = parser->getParsedLine(lineIdx);
        if (!t) {
            reportError("NULL LE return\n");
            goto done;
        }
        if (t->hasAssignmentMarker()) {
            reportUserMessage("unexpected line continue\n");
            goto done;
        }
        if (t->isRawMacroBlock) {
            DecodedLine *b = &blocks[n];
            b->decodeMode = 6;
            char *s = dupString(t->lineText);
            if (!s) {
                reportError("failed to dupe the pass thru text\n");
                goto done;
            }
            b->passThroughText = s;
            n++;
            lineIdx++;
            continue;
        }
        if (t->startsWithDirective()) {
            Directive *d = func_080519a0(t);
            if (!d) {
                reportUserMessage("failed to find a directive from line : \n");
                t->dumpSourceLine(-1);
                goto done;
            }
            if (!d->fn(p, t)) {
                reportUserMessage("a directive parser failed at line:\n");
                t->dumpSourceLine(-1);
                goto done;
            }
            lineIdx++;
            continue;
        }
        nOut = 0;
        do {
            t = parser->getParsedLine(lineIdx);
            if (!t) {
                reportError("got a null return.. while expanding line continues\n");
                goto done;
            }
            int old = pos;
            if (!p->proc.decodeLE(t, &pos, &labels, ins, &nOut)) {
                reportError("failed to build list of instructions for line\n");
                goto done;
            }
            if (pos >= getLineTokenCount(t)) {
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
        DecodedLine *b = &blocks[n];
        b->initialize();
        if (!b->labels.transferNodesFrom(&labels)) {
            reportError("failed to grab labels from current list\n");
            goto done;
        }
        bool synth = false;
        for (int i = 0; i != nOut; i++) {
            if (!ins[i]) {
                reportError("instruction decoder appears to have failed?\n");
                t->dumpSourceLine(-1);
                goto done;
            }
            if (ins[i]->syntheticKind) {
                if (nOut != 1) {
                    reportUserMessage("only one synthetic allowed per line\n");
                    t->dumpSourceLine(-1);
                    goto done;
                }
                synth = true;
            }
            if (!func_080522c4(ins[i]->instructionModifierBits, p->proc.decodeMode, false)) {
                reportUserMessage("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(p->proc.decodeMode));
                ins[i]->leDump();
                goto done;
            }
        }
        b->decodeMode = p->proc.decodeMode;
        b->decodeSubMode = p->proc.decodeSubMode;
        if (synth) {
            Instr *in = ins[0];
            b->instructions[0] = in;
            if (!in) {
                reportError("during the initial build, found an iEntry, which thought it was synthetic, but had no instruction\n");
                goto done;
            }
            switch ((unsigned char)in->syntheticKind) {
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
                if (b->decodeMode == 3) {
                    reportUserMessage("a loop size directive is an invalid directive for raw mode vsm..\n");
                    goto done;
                }
                /* fall through */
            case 8:
                if (p->proc.insideRawLoop)
                    b->rawLoopId = p->proc.rawLoopId;
                break;
            case 10:
                break;
            default:
                reportError("iStream::ValidateSynthetics() - unhandled/unexpected synthetic type (%d)\n",
                              (unsigned char)in->syntheticKind);
                in->dumpOperandDiagnostics(false);
                goto done;
            }
        } else if (p->proc.decodeMode == 3) {
            if (nOut != p->proc.instructionsPerRawLine) {
                reportUserMessage("in RAW decode mode, there weren't %d instructions on line : \n", p->proc.instructionsPerRawLine);
                t->dumpSourceLine(-1);
                goto done;
            }
            if (p->proc.insideRawLoop)
                b->rawLoopId = p->proc.rawLoopId;
            for (int j = 0; j != p->proc.instructionsPerRawLine; j++)
                b->instructions[j] = ins[j];
        } else {
            if (p->proc.insideRawLoop) {
                reportError("Build() - was in loop mode during uta\n");
                goto done;
            }
            if (nOut != 1) {
                reportUserMessage("Build() - more than one instruction on the line in vcl mode\n");
                t->dumpSourceLine(-1);
                goto done;
            }
            Instr *in = ins[0];
            if (!in) {
                reportError("decode vcl - instruction ptr was null.. when I thought it wasn't\n");
                t->dumpSourceLine(-1);
                goto done;
            }
            if (in->rawOnly) {
                reportUserMessage("vcl: instruction '%s' is unsupported, except in RAW mode.\n", in->opcodeInfo->mnemonic);
                in->leDump();
                delete in;
                goto done;
            }
            b->instructions[0] = in;
        }
        if (b->decodeMode == 1) {
            Instr *in = b->instructions[0];
            if (in && ((in->branchFlags >> 2) & 1)) {
                int r = in->simplifyBranch();
                if (r < 0) {
                    reportError("failed to fixup a branch\n");
                    goto done;
                }
                if (r == 0) {
                    in->leDump();
                    b->instructions[0] = 0;
                    delete in;
                    in = 0;
                }
                if (r == 2)
                    in->leDump();
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
        if (!p->proc.trailingLabels.transferNodesFrom(&labels)) {
            reportError("failed to grab ending labels\n");
            goto done;
        }
        for (mNode *nd = p->proc.trailingLabels.head; nd; nd = nd->next) {
            if (nd->flags) {
                reportUserMessage("can't attach [modifiers] to labels at the end of code\n");
                reportUserMessage("Label '%s:' has '%s'\n", nd->name, func_08052314(nd->flags));
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
        mList *a = &blocks[i].labels;
        if (!a->head)
            continue;
        for (int j = i + 1; j != n; j++) {
            mList *c = &blocks[j].labels;
            if (c->head) {
                for (mNode *nd = a->head; nd; nd = nd->next) {
                    if (c->findByNameIgnoreCase(nd->name)) {
                        reportUserMessage("found at least two uses of the label '%s' in the source\n", nd->name);
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
    } else if (p->proc.visibilityScopes.nodeCount)
        reportWarning("scope stack open (depth %d) at end of source\n", p->proc.visibilityScopes.nodeCount);
    p->proc.visibilityScopes.clearNodes();
    return blocks;
}

int gatherTokens(LocTag *t, int i, bool stopAtOpcode)
{
    char *d = gatheredToken;
    int n = 0;
    char sep;

    for (; i < getLineTokenCount(t); i++) {
        char *tok = t->tokenTextAt(i, &sep);
        if (!tok) {
            reportError("gatherTokens() - failed to get token\n");
            return -1;
        }
        if (!t->isCommentToken(i)) {
            if (t->func_08050f7c(i)) {
                *d = 0;
                return i;
            }
            if (n && stopAtOpcode) {
                for (char *p = tok; *p; p++)
                    if (p[0] == '.' && p[1] == '.') {
                        *d = 0;
                        return i;
                    }
            }
            if (n && stopAtOpcode && findOpcodeInfoByMnemonic(tok, false)) {
                *d = 0;
                return i;
            }
            n++;
            while (*tok) {
                if ((unsigned)(d - gatheredToken) > 0x3fe) {
                    reportUserMessage("gatherTokens: the gathered token became too long!\n");
                    return -1;
                }
                *d++ = *tok++;
                *d = 0;
            }
        }
        if (sep)
            break;
    }
    *d = 0;
    return i + 1;
}
