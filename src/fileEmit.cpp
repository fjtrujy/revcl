#include "Program.h"
#include "os.h"
#include "instStream.h"
#include "VUStr.h"
#include "fileEmit.h"

extern bool noAlign_080b3016;
int getNumOptimisers();
const char *getOptimiserName(int id);

char *emitFileStart(Program *p, char *buf)
{
    if ((!p->proc.newSyntax || p->proc.unk_63) && p->proc.mpgTargetAddress < 0)
        buf += formatIntoBuffer(buf, "\t\t.vu\n");
    if (!noAlign_080b3016)
        buf += formatIntoBuffer(buf, "\t\t.align 4\n");
    if (p->proc.globalName && *p->proc.globalName) {
        buf += formatIntoBuffer(buf, "\t\t.global\t%s_CodeStart\n", p->proc.globalName);
        buf += formatIntoBuffer(buf, "\t\t.global\t%s_CodeEnd\n", p->proc.globalName);
    }
    if (p->proc.mpgTargetAddress >= 0) {
        if (p->proc.globalName && *p->proc.globalName) {
            buf += formatIntoBuffer(buf, "\t\t.global\t%s_DmaTag\n", p->proc.globalName);
            buf += formatIntoBuffer(buf, "\t\t.global\t%s_DmaEnd\n", p->proc.globalName);
            buf += formatIntoBuffer(buf, "\n");
            buf += formatIntoBuffer(buf, "%s_DmaTag:\n", p->proc.globalName);
        }
        buf += formatIntoBuffer(buf, "\t\tDMAret\t*\n");
        buf += formatIntoBuffer(buf, "\t\tvifnop\t\n");
        buf += formatIntoBuffer(buf, "\t\tvifnop\t\n");
        buf += formatIntoBuffer(buf, "\t\tvifnop\t\n");
        buf += formatIntoBuffer(buf, "\t\tMPG\t%d,*\n", p->proc.mpgTargetAddress);
    }
    for (mNode *n = p->proc.exportedSymbols.head; n; n = n->next)
        buf += formatIntoBuffer(buf, "\t\t.global\t%s\n", n->name);
    if (p->proc.globalName && *p->proc.globalName)
        buf += formatIntoBuffer(buf, "%s_CodeStart:\n", p->proc.globalName);
    return buf;
}

char *emitFileEnd(Program *p, char *buf)
{
    if (!noAlign_080b3016)
        buf += formatIntoBuffer(buf, "\t\t.align 4\n");
    if (p->proc.globalName && *p->proc.globalName)
        buf += formatIntoBuffer(buf, "%s_CodeEnd:\n", p->proc.globalName);
    for (mNode *n = p->proc.trailingLabels.head; n; n = n->next)
        if (n->name)
            buf += formatIntoBuffer(buf, "%s:\n", n->name);
    if (p->proc.mpgTargetAddress >= 0) {
        buf += formatIntoBuffer(buf, "\t.EndMPG\n");
        buf += formatIntoBuffer(buf, "\t.EndDmaData\n");
    }
    if (p->proc.globalName && *p->proc.globalName && p->proc.mpgTargetAddress >= 0)
        buf += formatIntoBuffer(buf, "%s_DmaEnd:\n", p->proc.globalName);
    return buf;
}

static inline unsigned char streamEndType(instStream *s)
{
    return s->endType;
}

static inline void dumpFailedRow(wtRow *r)
{
    r->refreshInstructionOperands(-1);
}

char *workTable::emit(char *buf, int mode, int *p1, int *p2)
{
    *p2 = 0;
    *p1 = 0;
    char done = 0;
    int len = unk_34 - unk_30;
    if (len < 0) {
        reportError("iTable::emit - not done!\n");
        return 0;
    }
    instStream *owner = unk_10;
    if (!owner) {
        reportError("during emit - owner not set\n");
        return 0;
    }
    int nulls = 0;
    char isEnd = 0;
    if (streamEndType(owner) == 2) {
        isEnd = 1;
        nulls = 2;
    } else if (streamEndType(owner) == 3) {
        isEnd = 1;
        nulls = 2;
    }
    if (owner->branchType)
        nulls = 2;
    int top = (unsigned char)owner->unk_2d4;

    if (unk_09 == 4) {
        *p1 = nulls;
        *p2 = nulls;
    } else {
        for (int i = unk_30; i != unk_34; i++) {
            wtRow *r = &unk_2c[i];
            if (!r->skip)
                (*p1)++;
            (*p2)++;
        }
        if (isEnd && !func_08073854()) {
            (*p2)++;
            (*p1)++;
        }
    }

    if (unk_09 == 4) {
        wtRow blank;
        blank.clearEntries();
        for (int i = 0; i != nulls; i++) {
            int f = 0;
            if (isEnd && i == 0) {
                f = (unsigned char)(owner->unk_2d5 | 2);
                done = 1;
            }
            f |= top;
            top = 0;
            buf = blank.emit(buf, f, false, false);
            if (!buf) {
                reportError("failed to emit null ie for synth instruction!\n");
                return 0;
            }
        }
    } else {
        instStream *s = unk_10;
        if (s) {
            buf += formatIntoBuffer(buf, "; ");
            if (s->unk_140)
                buf += formatIntoBuffer(buf, "_LPOPT_");
            else
                buf += formatIntoBuffer(buf, "_LNOPT_");
            buf += formatIntoBuffer(buf, "w=[ ");
            if (s->unk_144) {
                for (int i = 0; i != getNumOptimisers(); i++) {
                    int bit = 1 << i;
                    if (s->unk_144 & bit)
                        buf += formatIntoBuffer(buf, "%s ", getOptimiserName(i));
                }
            }
            buf += formatIntoBuffer(buf, "] %d [%d %d] %d   [%s]\n",
                                 *p1, unk_74, unk_76, *p2, s->streamName());
        }
        if (len < nulls) {
            reportError("iTable: not fall. and too small a table\n");
            s->dump(0);
            goto fail;
        }
        *p1 = 0;
        *p2 = 0;
        int end = unk_34;
        if (isEnd && !func_08073854())
            end++;
        wtRow blank;
        blank.clearEntries();
        for (int i = unk_30; i != end; i++) {
            wtRow *r = &blank;
            if (i < unk_34)
                r = &unk_2c[i];
            int f = 0;
            if (!r->skip)
                (*p1)++;
            (*p2)++;
            if (isEnd) {
                if (i == end - 2) {
                    f = (unsigned char)(s->unk_2d5 | 2);
                    done = 1;
                }
            } else if (i == unk_34 - 1)
                f = (unsigned char)s->unk_2d5;
            f |= top;
            top = 0;
            switch (mode) {
            case 0:
                buf = r->emit(buf, f, false, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(r);
                    return 0;
                }
                break;
            case 1:
                *buf++ = ';';
                buf = r->emit(buf, f, true, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(r);
                    return 0;
                }
                *buf++ = ' ';
                buf = r->emit(buf, f, false, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(r);
                    return 0;
                }
                *buf++ = '\n';
                break;
            case 2:
                buf = r->emit(buf, f, true, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    r->refreshInstructionOperands(-1);
                    goto fail;
                }
                break;
            }
        }
    }
    if (isEnd && !done) {
        reportWarning("did not emit end insn\n");
        return 0;
    }
    return buf;
fail:
    return 0;
}

extern bool noEbit_080b3017;
extern char *stallNames_080b3244[3];
int func_0804bf50(unsigned int);
char *emitNop(char *buf, int flags, int *len);
char *emitInstr(Instr *in, char *buf, bool upper, int flags, bool b);

char *wtRow::emit(char *buf, int flags, bool upper, bool unused)
{
    if (!buf) {
        reportError("iEntry::emit - null pointer!\n");
        return 0;
    }
    if (skip) {
        if (countEntries()) {
            reportError("can't skip a non-empty ie\n");
            return 0;
        }
        return buf;
    }
    buf += formatIntoBuffer(buf, "         ");
    int i;
    for (i = 0; i != 2; i++)
        if (entries[i].unk_00)
            flags |= entries[i].unk_00->unk_1389;
    int len = 0;
    for (i = 0; i != 2; i++) {
        if (!entries[i].unk_00)
            buf = emitNop(buf, flags, &len);
        else
            buf = emitInstr(entries[i].unk_00, buf, false, flags, upper);
        if (!buf) {
            reportError("failed to emit upper for pipe %d ie\n", i);
            return 0;
        }
        flags = 0;
        if (i != 1)
            buf += formatIntoBuffer(buf, "         ");
    }
    if (maxStallLength) {
        buf += formatIntoBuffer(buf, "       ;\tSTALL");
        bool any = false;
        for (int k = 0; k != 3; k++) {
            int bit = (stallTypeMask >> k) & 1;
            if (bit) {
                buf += formatIntoBuffer(buf, "_%s", stallNames_080b3244[k]);
                any = true;
            }
        }
        if (!any)
            reportWarning("reported a stall.. but dunno what type\n");
        int stall = maxStallLength;
        buf += formatIntoBuffer(buf, " ?%d", stall);
    }
    return buf + formatIntoBuffer(buf, "\n");
}

char *emitNop(char *buf, int flags, int *len)
{
    char line[320];
    char mods[16];
    char *p = line;
    if (noEbit_080b3017)
        flags &= ~2;
    char *s = func_08052314(flags);
    char *d = mods;
    while (*s)
        *d++ = *s++;
    *d = 0;
    p += formatIntoBuffer(p, "NOP%s", mods);
    for (int n = p - line; n < 50; n++)
        *p++ = ' ';
    *p = 0;
    buf += formatIntoBuffer(buf, "%s", line);
    return buf;
}

char *emitInstr(Instr *in, char *buf, bool upper, int flags, bool b)
{
    char line[320];
    char mods[16];
    char *s = func_08052314(flags);
    char *p = line;
    char *d = mods;
    while (*s)
        *d++ = *s++;
    *d = 0;
    if (in->unk_1392 || !in->opcodeInfo)
        p += formatIntoBuffer(p, "nop%s", mods);
    else {
        if (in->opcodeInfo->emitHook) {
            p = in->opcodeInfo->emitHook(line, in, upper, mods, b, 0);
            if (!p) {
                reportError("emit hook failed for an instruction\n");
                return 0;
            }
        } else {
            p = func_0808750c(line, in, upper, mods, b, 0);
            if (!p) {
                reportError("generic emit hook failed for an instruction\n");
                return 0;
            }
        }
    }
    for (int n = p - line; n < 50; n++)
        *p++ = ' ';
    *p = 0;
    return buf + formatIntoBuffer(buf, "%s", line);
}

char *emitDestMask(char *p, int mdBits, unsigned char mods)
{
    if (p) {
        switch (mdBits) {
        case 0:
            goto done;
        case 1:
            break;
        case 2:
            if (mods != 7) {
                reportError("not just one mod bit for MD_XYZ..bits are %x\n", mods);
                return 0;
            }
            break;
        case 3:
            if (func_0804bf50(mods) != 1) {
                reportError("not just one mod bit for MD_ONE\n");
                return 0;
            }
            break;
        default:
            reportError("unknown md_bits for vpu target \n");
            return 0;
        }
        if (!mods) {
            reportError("modBits is null for case MD_ANY.. which should be ok, but ee-dvp-as doesn't assemble it\n");
            return 0;
        }
        if (mods != 15) {
            *p++ = '.';
            if (mods & 1) *p++ = 'x';
            if (mods & 2) *p++ = 'y';
            if (mods & 4) *p++ = 'z';
            if (mods & 8) *p++ = 'w';
        }
    }
done:
    return p;
}

extern const char bindExtForcedBaseError[] = "operand is forced to base, but not this one\n";
extern const char bindExtVirtualRegError[] = "found a virtual register on an x operand\n";
extern const char bindExtFindRegRefError[] = "failed to find regref in bind externals\n";
