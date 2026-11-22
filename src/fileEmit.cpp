#include "Program.h"
#include "os.h"
#include "instStream.h"
#include "VUStr.h"
#include "fileEmit.h"

extern bool disableOutputAlignment;
int getNumOptimisers();
const char *getOptimiserName(int id);

char *emitFileStart(Program *program, char *buf)
{
    if ((!program->proc.newSyntax || program->proc.vuDirectiveSeen) && program->proc.mpgTargetAddress < 0)
        buf += formatIntoBuffer(buf, "\t\t.vu\n");
    if (!disableOutputAlignment)
        buf += formatIntoBuffer(buf, "\t\t.align 4\n");
    if (program->proc.globalName && *program->proc.globalName) {
        buf += formatIntoBuffer(buf, "\t\t.global\t%s_CodeStart\n", program->proc.globalName);
        buf += formatIntoBuffer(buf, "\t\t.global\t%s_CodeEnd\n", program->proc.globalName);
    }
    if (program->proc.mpgTargetAddress >= 0) {
        if (program->proc.globalName && *program->proc.globalName) {
            buf += formatIntoBuffer(buf, "\t\t.global\t%s_DmaTag\n", program->proc.globalName);
            buf += formatIntoBuffer(buf, "\t\t.global\t%s_DmaEnd\n", program->proc.globalName);
            buf += formatIntoBuffer(buf, "\n");
            buf += formatIntoBuffer(buf, "%s_DmaTag:\n", program->proc.globalName);
        }
        buf += formatIntoBuffer(buf, "\t\tDMAret\t*\n");
        buf += formatIntoBuffer(buf, "\t\tvifnop\t\n");
        buf += formatIntoBuffer(buf, "\t\tvifnop\t\n");
        buf += formatIntoBuffer(buf, "\t\tvifnop\t\n");
        buf += formatIntoBuffer(buf, "\t\tMPG\t%d,*\n", program->proc.mpgTargetAddress);
    }
    for (mNode *macroNode = program->proc.exportedSymbols.head; macroNode; macroNode = macroNode->next)
        buf += formatIntoBuffer(buf, "\t\t.global\t%s\n", macroNode->name);
    if (program->proc.globalName && *program->proc.globalName)
        buf += formatIntoBuffer(buf, "%s_CodeStart:\n", program->proc.globalName);
    return buf;
}

char *emitFileEnd(Program *program, char *buf)
{
    if (!disableOutputAlignment)
        buf += formatIntoBuffer(buf, "\t\t.align 4\n");
    if (program->proc.globalName && *program->proc.globalName)
        buf += formatIntoBuffer(buf, "%s_CodeEnd:\n", program->proc.globalName);
    for (mNode *macroNode = program->proc.trailingLabels.head; macroNode; macroNode = macroNode->next)
        if (macroNode->name)
            buf += formatIntoBuffer(buf, "%s:\n", macroNode->name);
    if (program->proc.mpgTargetAddress >= 0) {
        buf += formatIntoBuffer(buf, "\t.EndMPG\n");
        buf += formatIntoBuffer(buf, "\t.EndDmaData\n");
    }
    if (program->proc.globalName && *program->proc.globalName && program->proc.mpgTargetAddress >= 0)
        buf += formatIntoBuffer(buf, "%s_DmaEnd:\n", program->proc.globalName);
    return buf;
}

static inline unsigned char streamEndType(instStream *stream)
{
    return stream->endType;
}

static inline void dumpFailedRow(wtRow *workRow)
{
    workRow->refreshInstructionOperands(-1);
}

char *workTable::emit(char *buf, int mode, int *p1, int *p2)
{
    *p2 = 0;
    *p1 = 0;
    char done = 0;
    int len = rangeEnd - rangeStart;
    if (len < 0) {
        reportError("iTable::emit - not done!\n");
        return 0;
    }
    instStream *owner = ownerStream;
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
    int top = (unsigned char)owner->leadingLabelFlags;

    if (codeModeValue == 4) {
        *p1 = nulls;
        *p2 = nulls;
    } else {
        for (int rowIndex = rangeStart; rowIndex != rangeEnd; rowIndex++) {
            wtRow *workRow = &rows[rowIndex];
            if (!workRow->skip)
                (*p1)++;
            (*p2)++;
        }
        if (isEnd && !hasSafeTrailingInstructions()) {
            (*p2)++;
            (*p1)++;
        }
    }

    if (codeModeValue == 4) {
        wtRow blank;
        blank.clearEntries();
        for (int i = 0; i != nulls; i++) {
            int emissionFlags = 0;
            if (isEnd && i == 0) {
                emissionFlags = (unsigned char)(owner->trailingLabelFlags | 2);
                done = 1;
            }
            emissionFlags |= top;
            top = 0;
            buf = blank.emit(buf, emissionFlags, false, false);
            if (!buf) {
                reportError("failed to emit null ie for synth instruction!\n");
                return 0;
            }
        }
    } else {
        instStream *stream = ownerStream;
        if (stream) {
            buf += formatIntoBuffer(buf, "; ");
            if (stream->hasLoopOptimization)
                buf += formatIntoBuffer(buf, "_LPOPT_");
            else
                buf += formatIntoBuffer(buf, "_LNOPT_");
            buf += formatIntoBuffer(buf, "w=[ ");
            if (stream->appliedOptimizationMask) {
                for (int i = 0; i != getNumOptimisers(); i++) {
                    int bit = 1 << i;
                    if (stream->appliedOptimizationMask & bit)
                        buf += formatIntoBuffer(buf, "%s ", getOptimiserName(i));
                }
            }
            buf += formatIntoBuffer(buf, "] %d [%d %d] %d   [%s]\n",
                                 *p1, streamDependencyLength, streamLogicalLength, *p2, stream->streamName());
        }
        if (len < nulls) {
            reportError("iTable: not fall. and too small a table\n");
            stream->dump(0);
            goto fail;
        }
        *p1 = 0;
        *p2 = 0;
        int end = rangeEnd;
        if (isEnd && !hasSafeTrailingInstructions())
            end++;
        wtRow blank;
        blank.clearEntries();
        for (int i = rangeStart; i != end; i++) {
            wtRow *emissionRow = &blank;
            if (i < rangeEnd)
                emissionRow = &rows[i];
            int f = 0;
            if (!emissionRow->skip)
                (*p1)++;
            (*p2)++;
            if (isEnd) {
                if (i == end - 2) {
                    f = (unsigned char)(stream->trailingLabelFlags | 2);
                    done = 1;
                }
            } else if (i == rangeEnd - 1)
                f = (unsigned char)stream->trailingLabelFlags;
            f |= top;
            top = 0;
            switch (mode) {
            case 0:
                buf = emissionRow->emit(buf, f, false, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(emissionRow);
                    return 0;
                }
                break;
            case 1:
                *buf++ = ';';
                buf = emissionRow->emit(buf, f, true, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(emissionRow);
                    return 0;
                }
                *buf++ = ' ';
                buf = emissionRow->emit(buf, f, false, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(emissionRow);
                    return 0;
                }
                *buf++ = '\n';
                break;
            case 2:
                buf = emissionRow->emit(buf, f, true, true);
                if (!buf) {
                    reportError("failed to emit instruction!\n");
                    emissionRow->refreshInstructionOperands(-1);
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

extern bool disableEndBits;
extern char *stallTypeNames[3];
int countSetBits(unsigned int);
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
    int entryIndex;
    for (entryIndex = 0; entryIndex != 2; entryIndex++)
        if (entries[entryIndex].instruction)
            flags |= entries[entryIndex].instruction->instructionModifierBits;
    int len = 0;
    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        if (!entries[entryIndex].instruction)
            buf = emitNop(buf, flags, &len);
        else
            buf = emitInstr(entries[entryIndex].instruction, buf, false, flags, upper);
        if (!buf) {
            reportError("failed to emit upper for pipe %d ie\n", entryIndex);
            return 0;
        }
        flags = 0;
        if (entryIndex != 1)
            buf += formatIntoBuffer(buf, "         ");
    }
    if (maxStallLength) {
        buf += formatIntoBuffer(buf, "       ;\tSTALL");
        bool any = false;
        for (int k = 0; k != 3; k++) {
            int bit = (stallTypeMask >> k) & 1;
            if (bit) {
                buf += formatIntoBuffer(buf, "_%s", stallTypeNames[k]);
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
    char *lineCursor = line;
    if (disableEndBits)
        flags &= ~2;
    char *modifierSource = formatInstructionModifiers(flags);
    char *modifierDestination = mods;
    while (*modifierSource)
        *modifierDestination++ = *modifierSource++;
    *modifierDestination = 0;
    lineCursor += formatIntoBuffer(lineCursor, "NOP%s", mods);
    for (int paddingIndex = lineCursor - line; paddingIndex < 50; paddingIndex++)
        *lineCursor++ = ' ';
    *lineCursor = 0;
    buf += formatIntoBuffer(buf, "%s", line);
    return buf;
}

char *emitInstr(Instr *in, char *buf, bool upper, int flags, bool b)
{
    char line[320];
    char mods[16];
    char *modifierSource = formatInstructionModifiers(flags);
    char *lineCursor = line;
    char *modifierDestination = mods;
    while (*modifierSource)
        *modifierDestination++ = *modifierSource++;
    *modifierDestination = 0;
    if (in->tildePrefix || !in->opcodeInfo)
        lineCursor += formatIntoBuffer(lineCursor, "nop%s", mods);
    else {
        if (in->opcodeInfo->emitHook) {
            lineCursor = in->opcodeInfo->emitHook(line, in, upper, mods, b, 0);
            if (!lineCursor) {
                reportError("emit hook failed for an instruction\n");
                return 0;
            }
        } else {
            lineCursor = emitGenericInstructionText(line, in, upper, mods, b, 0);
            if (!lineCursor) {
                reportError("generic emit hook failed for an instruction\n");
                return 0;
            }
        }
    }
    for (int paddingIndex = lineCursor - line; paddingIndex < 50; paddingIndex++)
        *lineCursor++ = ' ';
    *lineCursor = 0;
    return buf + formatIntoBuffer(buf, "%s", line);
}

char *emitDestMask(char *outputCursor, int mdBits, unsigned char mods)
{
    if (outputCursor) {
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
            if (countSetBits(mods) != 1) {
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
            *outputCursor++ = '.';
            if (mods & 1) *outputCursor++ = 'x';
            if (mods & 2) *outputCursor++ = 'y';
            if (mods & 4) *outputCursor++ = 'z';
            if (mods & 8) *outputCursor++ = 'w';
        }
    }
done:
    return outputCursor;
}

extern const char bindExtForcedBaseError[] = "operand is forced to base, but not this one\n";
extern const char bindExtVirtualRegError[] = "found a virtual register on an x operand\n";
extern const char bindExtFindRegRefError[] = "failed to find regref in bind externals\n";
