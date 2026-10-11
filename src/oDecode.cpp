// Instruction line decoding ("oDecode", CIE group 38).
#include "os.h"
#include "Parser.h"
#include "mList.h"
#include "Processor.h"
#include "VUStr.h"
#include "Instr.h"
#include "workTable.h"
#include "flowBlock.h"
#include "Program.h"

char gatheredToken[1024];

int gatherTokens(LocTag *t, int i, bool stopAtOpcode);
int func_0804bf50(unsigned int x);
bool func_0804bfcc(const char *a, const char *b);
char *dupString(const char *s);
int mstrlen(const char *s);
bool oDecodeToken(int kind, char *tok, ImplicitOp *out, bool newSyntax);
bool parseImmediate(const char *s, unsigned char type, mValue *v);
RegPool *GetRP(unsigned t);
int g_080b34dc = -1;

Instr *Processor::getNextInstruction(LocTag *t, int *pos)
{
    int copyCount = 0;
    ImplicitOp op;
    char buf[0x801];
    char nameBuf[0x400];
    char fmtBuf[0x400];
    int cur = *pos;
    short loopid = 0;
    bool explicitMods = false;
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
        func_0804ad38("getNextInstruction - passed null pointer\n");
        goto fail;
    }
    if (t->func_08050f18(cur)) {
        func_0804ad38("getNextInstruction - tried to parse a comment\n");
        goto fail;
    }
    {
        char *s = t->func_08050be4(cur);
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
            loopid = func_0804add4(p);
            if (loopid > 32) {
                func_0804ad84("unlinkely large loopid set with instruction\n");
                goto fail;
            }
        }
        p = q + 2;
    }
    info = func_0806bf8c(p, unk_00);
    if (!info) {
        func_0804ad84("vcl can't find instruction '%s'\n", p);
        goto fail;
    }
    cur++;
    in = new Instr(info);
    if (loopid < 0) {
        func_0804ad84("parsed loop id is <0\n");
        goto fail;
    }
    in->unk_1420 = loopid;
    in->unk_1389 = mods;
    in->unk_1392 = tilde;
    mnemonicLen = mstrlen(info->unk_04);
    {
        char *suffix = p + mnemonicLen;
        bad = false;
        hasMods = func_08051f08(suffix, &mask, &bad);
    }
    if (bad) {
        func_0804ad84("instruction has bad .modifier bits\n");
        goto fail;
    }
    switch ((char)info->unk_0c) {
    case 0:
        break;
    case 1:
        if (hasMods) {
            in->unk_138f = mask;
            if (!in->unk_138f) {
                func_0804ad84("getNextInstruction: instruction had zero mod bits\n");
                goto fail;
            }
            forcedMods = true;
            explicitMods = true;
        } else
            in->unk_138f = 15;
        break;
    case 2:
        if (hasMods && mask != 7) {
            func_0804ad84("getNextInstruction: XYZ instruction had bad mod bits\n");
            goto fail;
        }
        forcedMods = true;
        in->unk_138f = 7;
        break;
    case 3:
        if (hasMods) {
            if (in->unk_13ce == 1 && func_0804bf50(mask) != 1) {
                func_0804ad84("getNextInstruction: MD_ONE instruction had bad mod bits. more than one?\n");
                goto fail;
            }
            explicitMods = true;
            in->unk_138f = mask;
        }
        break;
    default:
        func_0804ad38("processor: decode le - bad insn type\n");
        goto fail;
    }
    op.unk_1000[0] = 0;
    op.name[0] = 0;
    op.unk_800[0] = 0;
    op.unk_1800[0] = 0;
    op.unk_2000 = 0;
    op.unk_2008 = 0;
    op.unk_200c = 0;
    for (i = 0; i != in->unk_138b; i++) {
        Operand *opd = &in->unk_38[i];
        int next = cur;
        if (opd->unk_08) {
            if (!buildImplicitOperand(opd->unk_04, &op, unk_00)) {
                func_0804ad38("getNextInstruction: failed to build implicit operand\n");
                goto fail;
            }
            if (opd->unk_0d) {
                if (opd->unk_09 != 1) {
                    func_0804ad38("implicit web patch set for non-output\n");
                    goto fail;
                }
                Operand *src = &in->unk_38[opd->unk_0d - 1];
                if (src->unk_09 != 2) {
                    func_0804ad38("implicit web patched operand not input\n");
                    goto fail;
                }
                if (!src->unk_18) {
                    func_0804ad38("no register found for implicit patch\n");
                    goto fail;
                }
                func_0804adbc(op.name, "%s", src->unk_18->func_0805a8e0());
            }
        } else {
            bool last = false;
            if (i == in->unk_138b - 1 || opd[1].unk_08)
                last = true;
            opd->unk_24 = cur;
            next = gatherTokens(t, cur, last);
            if (next < 0)
                goto fail;
            if (info->func_08070020() == 11 && !gatheredToken[0]) {
                gatheredToken[0] = ' ';
                gatheredToken[1] = 0;
            }
            opd->unk_25 = next;
            if (!oDecodeToken(opd->unk_04, gatheredToken, &op, unk_00)) {
                func_0804ad84("failed to decode operand\n");
                goto fail;
            }
        }
        if (op.unk_1800[0]) {
            if (in->unk_139c) {
                func_0804ad84("more than one tagged operand associated with instruction at operand %d\n", i);
                goto fail;
            }
            in->unk_139c = dupString(op.unk_1800);
        }
        if (op.unk_1000[0] && !parseImmediate(op.unk_1000, opd->unk_05, &opd->unk_1c)) {
            func_0804ad38("failed to decode an immediate\n");
            goto fail;
        }
        opd->unk_0c = op.unk_2004;
        opd->unk_07 = op.unk_2000;
        if (opd->unk_04 == 35 || opd->unk_04 == 42 || opd->unk_04 == 1 || opd->unk_04 == 9) {
            if (forcedMods) {
                if (opd->unk_07 != in->unk_138f && opd->unk_07 != 15)
                    func_0804ac7c("getNextInstruction: forced mod bits!\n");
                opd->unk_07 = in->unk_138f;
            } else if (opBits < 0) {
                opBits = opd->unk_07;
                in->unk_138f = opBits;
            } else if (opd->unk_07 != opBits) {
                func_0804ad38("oDecode: bad mod bits in instruction.. not match opBitsStore (%x vs %x) op=%d?\n",
                              opd->unk_07, opBits, i);
                goto fail;
            }
        }
        if (opd->unk_04 == 30 || opd->unk_04 == 31) {
            if (explicitMods) {
                if (opd->unk_0c) {
                    if (opd->unk_0c != in->unk_138f) {
                        func_0804ad38("ea/reg or imm(ea)reg has differing mod bits\n");
                        goto fail;
                    }
                } else
                    opd->unk_0c = in->unk_138f;
            } else {
                if (!opd->unk_0c) {
                    func_0804ad38("have to set modbits for ea/reg or imm(ea)reg somewhere\n");
                    goto fail;
                }
                if (info->unk_0c == 3 && func_0804bf50(opd->unk_0c) != 1) {
                    func_0804ad38("only one mod field allowed (one of x,y,z, or w)\n");
                    goto fail;
                }
                in->unk_138f = opd->unk_0c;
            }
        }
        if (op.name[0]) {
            int cls = -1;
            if (opd->unk_04 == 38) {
                if (op.unk_2008 == 1) {
                    if (i != 2) {
                        func_0804ad84("a scalar can only be the third operand in generic instruction\n");
                        goto fail;
                    }
                    if (func_0804bfcc(op.name, "i")) {
                        cls = 10;
                        opd->unk_04 = 4;
                    }
                    if (func_0804bfcc(op.name, "q")) {
                        cls = 10;
                        opd->unk_04 = 7;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->unk_04 = 3;
                    }
                } else if (op.unk_2008 == 2) {
                    if (i == 0) {
                        if (func_0804bfcc(op.name, "acc")) {
                            cls = 10;
                            opd->unk_04 = 9;
                        }
                    } else if (func_0804bfcc(op.name, "acc")) {
                        func_0804ad84("ACC can only be first vector operand\n");
                        goto fail;
                    }
                    if (cls < 0) {
                        cls = 9;
                        opd->unk_04 = 1;
                    }
                    opd->unk_07 = in->unk_138f;
                    if (!unk_00) {
                        func_0804ad38("new syntax off while parsing a tbd instruction\n");
                        goto fail;
                    }
                } else {
                    func_0804ad84("couldn't determine operand type for generic instruction\n");
                    goto fail;
                }
            } else
                cls = func_08057a34(opd->unk_04);
            RegPool *pool = GetRP(cls);
            if (!pool) {
                func_0804ad38("oDecode: not find expected register ! regtype= '%d' operand = %d\n", cls, i);
                goto fail;
            }
            s = nameBuf;
            if (pool->unk_18 && unk_4c.unk_10 && !pool->func_08059418(op.name) && op.name[0] != '_') {
                for (mNode *n = unk_4c.unk_10; n; n = n->unk_08) {
                    char *np = n->unk_14;
                    if (!np) {
                        func_0804ad38("null name in parsep\n");
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
                    if ((found && n->unk_18 == 0) || (!found && n->unk_18 == 1)) {
                        s += func_0804adbc(s, "%d", (int)n->unk_10);
                        break;
                    }
                }
            }
            func_0804adbc(s, "%s", op.name);
            char *full = dupString(nameBuf);
            RegPoolEntry *r = pool->func_08059944(full);
            if (!r) {
                func_0804ad84("oDecode: not find register '%s' from %s\n", full, pool->unk_2c);
                goto fail;
            }
            if (op.unk_200c) {
                RegPoolEntry *base = 0;
                if (op.unk_800[0]) {
                    base = pool->func_08059418(op.unk_800);
                    if (!base) {
                        func_0804ad84("request to bing non-base register\n");
                        goto fail;
                    }
                    if (r->unk_18 && r->unk_18 != base)
                        func_0804ac7c("changing binding for %s from %s to %s\n",
                                      r->func_0805a8e0(), r->unk_18->func_0805a8e0(),
                                      base->func_0805a8e0());
                } else if (r->unk_18)
                    base = r->unk_18;
                else if (r->unk_14 != 1)
                    base = r;
                if (!base) {
                    func_0804ad84(" can't figure out which base register to map %s to\n",
                                  r->func_0805a8e0());
                    goto fail;
                }
                if (base->unk_14 == 1) {
                    func_0804ad38("what I thought was a base register isn't (%s maps to %s?)\n",
                                  base->func_0805a8e0(), r->func_0805a8e0());
                    goto fail;
                }
                r->unk_18 = base;
                opd->unk_10 = base;
            } else if (op.unk_800[0]) {
                RegPoolEntry *base = pool->func_08059418(op.unk_800);
                if (!base) {
                    func_0804ad84("failed to find base map reg '%s'\n", op.unk_800);
                    goto fail;
                }
                if (r->unk_14 != 1 && base != r) {
                    func_0804ad84("base registers not the same\n");
                    goto fail;
                }
                opd->unk_10 = base;
            } else if (unk_65) {
                RegPoolEntry *e = pool->func_08059418(op.name);
                if (e && e != e->unk_04->unk_10)
                    opd->unk_10 = e;
            } else if (r->unk_14 != 1 && !r->unk_16)
                opd->unk_10 = r;
            opd->unk_18 = r;
            opd->unk_14 = r;
            if (full)
                delete[] full;
        }
        cur = next;
    }
    if (!in->func_08053fac()) {
        func_0804ad38("oDecode: sanity check failed\n");
        goto fail;
    }
    if (info->unk_20 && !info->unk_20(info, in)) {
        func_0804ad38("oDecode:: iBuildHook failed to patch instruction\n");
        goto fail;
    }
    if (!in->func_08055ae4()) {
        func_0804ad38("failed to patch status register dependency\n");
        goto fail;
    }
    if (in->func_080532d8() < 0) {
        func_0804ad38("failed to normalise instruction\n");
        goto fail;
    }
    ok = true;
    in->unk_14 = t;
    in->unk_18 = *pos;
    in->unk_1a = cur;
fail:
    if (!ok) {
        if (in) {
            func_0804ad84("couldn't decode instruction .. problem is at:\n");
            t->func_08050c40(cur);
            if (in->func_080552a8(unk_00, fmtBuf, g_080b34dc))
                func_0804ad84("expected format is: %s\n", fmtBuf);
            delete in;
            in = 0;
        } else if (t) {
            func_0804ad84("found a syntax error at :\n");
            t->func_08050c40(cur);
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
        func_0804ad38("decodeLE: passed a null pointer\n");
        return false;
    }
    if (*pos == 0) {
        char *tok = t->func_08050be4(0, 0);
        mNode *n;
        if (func_080506e0(tok) && !t->func_08050f18(0)) {
            if (*nOut != 0) {
                func_0804ad84("found a label on a continued line\n");
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
            n = labels->func_0804b444(buf);
            if (!n) {
                func_0804ad38("Processor::decodeLE() - couldn't create tail for label list\n");
                return false;
            }
            n->unk_18 = mods;
            (*pos)++;
            if (func_0805085c(t) > 1) {
                char *tok2 = t->func_08050be4(1, 0);
                if (*tok2 == '[') {
                    {
                        s = tok2;
                        d = buf + 1;
                        i = 0;
                        for (; *s && i < 0x800; i++)
                            *d++ = *s++;
                        *d = 0;
                    }
                    n->unk_18 |= func_080521f0(buf);
                    (*pos)++;
                }
            }
            if (!func_080522c4(n->unk_18, unk_3d, true)) {
                int mode = unk_3d;
                func_0804ad84("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(mode));
                t->func_08050c40(-1);
                goto fail;
            }
        }
    }
    if (t->func_08050f48())
        (*pos)++;
next:
    while (*pos < func_0805085c(t)) {
        if (!t->func_08050f18(*pos)) {
            if (t->func_08050f7c(*pos)) {
                (*pos)++;
                goto done;
            }
            if (*nOut > 1) {
                func_0804ad84("too many instructions on a line!\n");
                t->func_08050c40(-1);
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

flowBlock *func_08083768(Program *p, Parser *parser, int type, bool newSyntax, int *count)
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
    p->proc.unk_00 = newSyntax;
    flowBlock *blocks = 0;

    switch (type) {
    case 1:
        p->proc.unk_3d = 1;
        p->proc.unk_65 = 0;
        break;
    case 2:
        p->proc.unk_3d = 3;
        p->proc.unk_65 = 1;
        break;
    default:
        func_0804ad38("Processor::Build:: bad build type request\n");
        goto done;
    }
    p->proc.unk_62 = 0;
    p->proc.unk_40 = 1;
    p->proc.unk_3f = 0;
    blocks = new flowBlock[0x1000];
    if (!blocks) {
        func_0804ad38("Processor::Build() new failed for decLine table\n");
        goto done;
    }
    pos = 0;
    while (lineIdx < parser->func_0805017c()) {
        LocTag *t = parser->func_0805018c(lineIdx);
        if (!t) {
            func_0804ad38("NULL LE return\n");
            goto done;
        }
        if (t->func_08050f48()) {
            func_0804ad84("unexpected line continue\n");
            goto done;
        }
        if (t->unk_08) {
            flowBlock *b = &blocks[n];
            b->unk_05 = 6;
            char *s = dupString(t->unk_00);
            if (!s) {
                func_0804ad38("failed to dupe the pass thru text\n");
                goto done;
            }
            b->unk_00 = s;
            n++;
            lineIdx++;
            continue;
        }
        if (t->func_08050eec()) {
            Directive *d = func_080519a0(t);
            if (!d) {
                func_0804ad84("failed to find a directive from line : \n");
                t->func_08050c40(-1);
                goto done;
            }
            if (!d->fn(p, t)) {
                func_0804ad84("a directive parser failed at line:\n");
                t->func_08050c40(-1);
                goto done;
            }
            lineIdx++;
            continue;
        }
        nOut = 0;
        do {
            t = parser->func_0805018c(lineIdx);
            if (!t) {
                func_0804ad38("got a null return.. while expanding line continues\n");
                goto done;
            }
            int old = pos;
            if (!p->proc.decodeLE(t, &pos, &labels, ins, &nOut)) {
                func_0804ad38("failed to build list of instructions for line\n");
                goto done;
            }
            if (pos >= func_0805085c(t)) {
                lineIdx++;
                pos = 0;
            } else if (old >= pos) {
                func_0804ad38("didn't advance start token during processing\n");
                goto done;
            }
        } while (parser->func_08050738(lineIdx));
        if (!nOut)
            continue;
        if (n < 0) {
            func_0804ad38("out of bounds rLine getting decoded instructions\n");
            goto done;
        }
        if (n >= maxLines) {
            func_0804ad38("out of bounds rLine getting decoded instructions\n");
            goto done;
        }
        flowBlock *b = &blocks[n];
        b->func_08084220();
        if (!b->unk_14.func_0804b4d0(&labels)) {
            func_0804ad38("failed to grab labels from current list\n");
            goto done;
        }
        bool synth = false;
        for (int i = 0; i != nOut; i++) {
            if (!ins[i]) {
                func_0804ad38("instruction decoder appears to have failed?\n");
                t->func_08050c40(-1);
                goto done;
            }
            if (ins[i]->unk_13cd) {
                if (nOut != 1) {
                    func_0804ad84("only one synthetic allowed per line\n");
                    t->func_08050c40(-1);
                    goto done;
                }
                synth = true;
            }
            if (!func_080522c4(ins[i]->unk_1389, p->proc.unk_3d, false)) {
                func_0804ad84("invalid combination of [E/M/D/T] for this context (%s) at instruction \n",
                              codeModeName(p->proc.unk_3d));
                ins[i]->leDump();
                goto done;
            }
        }
        b->unk_05 = p->proc.unk_3d;
        b->unk_06 = p->proc.unk_3e;
        if (synth) {
            Instr *in = ins[0];
            b->unk_0c[0] = in;
            if (!in) {
                func_0804ad38("during the initial build, found an iEntry, which thought it was synthetic, but had no instruction\n");
                goto done;
            }
            switch ((unsigned char)in->unk_13cd) {
            case 1:
                if (state) {
                    func_0804ad84("encountered enter directive in bad context\n");
                    goto done;
                }
                enters++;
                state = 1;
                break;
            case 2:
                if (state != 1) {
                    func_0804ad84("ValidateSynthetics: encountered an end enter directive, but not in enter context\n");
                    goto done;
                }
                state = 0;
                break;
            case 3:
            case 11:
                if (state) {
                    func_0804ad84("encountered exit directive in bad context\n");
                    goto done;
                }
                exits++;
                state = 2;
                break;
            case 4:
                if (state != 2) {
                    func_0804ad84("encountered a end enter directive, but not in enter context\n");
                    goto done;
                }
                state = 0;
                break;
            case 5:
                if (state) {
                    func_0804ad84("encountered cont directive in bad context..mode=%d\n", state);
                    goto done;
                }
                exits++;
                break;
            case 6:
                if (state != 1) {
                    func_0804ad84("encountered an input directive, but not in enter context\n");
                    goto done;
                }
                break;
            case 7:
                if (state != 2) {
                    func_0804ad84("encountered an output directive, but not in enter context\n");
                    goto done;
                }
                break;
            case 9:
            case 12:
                if (b->unk_05 == 3) {
                    func_0804ad84("a loop size directive is an invalid directive for raw mode vsm..\n");
                    goto done;
                }
                /* fall through */
            case 8:
                if (p->proc.unk_3f)
                    b->unk_0a = p->proc.unk_40;
                break;
            case 10:
                break;
            default:
                func_0804ad38("iStream::ValidateSynthetics() - unhandled/unexpected synthetic type (%d)\n",
                              (unsigned char)in->unk_13cd);
                in->func_08054008(false);
                goto done;
            }
        } else if (p->proc.unk_3d == 3) {
            if (nOut != p->proc.unk_38) {
                func_0804ad84("in RAW decode mode, there weren't %d instructions on line : \n", p->proc.unk_38);
                t->func_08050c40(-1);
                goto done;
            }
            if (p->proc.unk_3f)
                b->unk_0a = p->proc.unk_40;
            for (int j = 0; j != p->proc.unk_38; j++)
                b->unk_0c[j] = ins[j];
        } else {
            if (p->proc.unk_3f) {
                func_0804ad38("Build() - was in loop mode during uta\n");
                goto done;
            }
            if (nOut != 1) {
                func_0804ad84("Build() - more than one instruction on the line in vcl mode\n");
                t->func_08050c40(-1);
                goto done;
            }
            Instr *in = ins[0];
            if (!in) {
                func_0804ad38("decode vcl - instruction ptr was null.. when I thought it wasn't\n");
                t->func_08050c40(-1);
                goto done;
            }
            if (in->unk_138a) {
                func_0804ad84("vcl: instruction '%s' is unsupported, except in RAW mode.\n", in->unk_34->unk_04);
                in->leDump();
                delete in;
                goto done;
            }
            b->unk_0c[0] = in;
        }
        if (b->unk_05 == 1) {
            Instr *in = b->unk_0c[0];
            if (in && ((in->unk_1388 >> 2) & 1)) {
                int r = in->func_08053104();
                if (r < 0) {
                    func_0804ad38("failed to fixup a branch\n");
                    goto done;
                }
                if (r == 0) {
                    in->leDump();
                    b->unk_0c[0] = 0;
                    delete in;
                    in = 0;
                }
                if (r == 2)
                    in->leDump();
            }
        }
        n++;
    }
    if (labels.unk_08) {
        if (n < 0) {
            func_0804ad38("out of bounds rLine extending table\n");
            goto done;
        }
        if (n >= maxLines) {
            func_0804ad38("out of bounds rLine extending table\n");
            goto done;
        }
        func_0804ac7c("gathering labels at end of code into the final namelist\n");
        if (!p->proc.unk_20.func_0804b4d0(&labels)) {
            func_0804ad38("failed to grab ending labels\n");
            goto done;
        }
        for (mNode *nd = p->proc.unk_20.unk_0c; nd; nd = nd->unk_04) {
            if (nd->unk_18) {
                func_0804ad84("can't attach [modifiers] to labels at the end of code\n");
                func_0804ad84("Label '%s:' has '%s'\n", nd->unk_14, func_08052314(nd->unk_18));
                goto done;
            }
        }
    }
    if (state) {
        func_0804ad38("iStream::Validate - left stream - but not in null state\n");
        goto done;
    }
    if (!enters) {
        func_0804ad38("iStream: no enter directives!\n");
        goto done;
    }
    if (!exits) {
        func_0804ad38("iStream: no exit directives\n");
        goto done;
    }
    for (int i = 0; i != n - 1; i++) {
        mList *a = &blocks[i].unk_14;
        if (!a->unk_0c)
            continue;
        for (int j = i + 1; j != n; j++) {
            mList *c = &blocks[j].unk_14;
            if (c->unk_0c) {
                for (mNode *nd = a->unk_0c; nd; nd = nd->unk_04) {
                    if (c->func_0804b284(nd->unk_14)) {
                        func_0804ad84("found at least two uses of the label '%s' in the source\n", nd->unk_14);
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
    } else if (p->proc.unk_4c.unk_08)
        func_0804ac7c("scope stack open (depth %d) at end of source\n", p->proc.unk_4c.unk_08);
    p->proc.unk_4c.func_0804b1b0();
    return blocks;
}

int gatherTokens(LocTag *t, int i, bool stopAtOpcode)
{
    char *d = gatheredToken;
    int n = 0;
    char sep;

    for (; i < func_0805085c(t); i++) {
        char *tok = t->func_08050be4(i, &sep);
        if (!tok) {
            func_0804ad38("gatherTokens() - failed to get token\n");
            return -1;
        }
        if (!t->func_08050f18(i)) {
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
            if (n && stopAtOpcode && func_0806bf8c(tok, false)) {
                *d = 0;
                return i;
            }
            n++;
            while (*tok) {
                if ((unsigned)(d - gatheredToken) > 0x3fe) {
                    func_0804ad84("gatherTokens: the gathered token became too long!\n");
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
