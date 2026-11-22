#include "Program.h"
#include "os.h"
#include "instStream.h"
#include "VUStr.h"
#include "fileEmit.h"

extern bool disableOutputAlignment;
int getOptimizerCount();
const char *getOptimizerName(int id);

char *emitFileStart(Program *program, char *outputBuffer)
{
    if ((!program->proc.newSyntax || program->proc.vuDirectiveSeen) && program->proc.mpgTargetAddress < 0)
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.vu\n");
    if (!disableOutputAlignment)
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.align 4\n");
    if (program->proc.globalName && *program->proc.globalName) {
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.global\t%s_CodeStart\n", program->proc.globalName);
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.global\t%s_CodeEnd\n", program->proc.globalName);
    }
    if (program->proc.mpgTargetAddress >= 0) {
        if (program->proc.globalName && *program->proc.globalName) {
            outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.global\t%s_DmaTag\n", program->proc.globalName);
            outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.global\t%s_DmaEnd\n", program->proc.globalName);
            outputBuffer += formatIntoBuffer(outputBuffer, "\n");
            outputBuffer += formatIntoBuffer(outputBuffer, "%s_DmaTag:\n", program->proc.globalName);
        }
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\tDMAret\t*\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\tvifnop\t\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\tvifnop\t\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\tvifnop\t\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\tMPG\t%d,*\n", program->proc.mpgTargetAddress);
    }
    for (mNode *macroNode = program->proc.exportedSymbols.head; macroNode; macroNode = macroNode->next)
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.global\t%s\n", macroNode->name);
    if (program->proc.globalName && *program->proc.globalName)
        outputBuffer += formatIntoBuffer(outputBuffer, "%s_CodeStart:\n", program->proc.globalName);
    return outputBuffer;
}

char *emitFileEnd(Program *program, char *outputBuffer)
{
    if (!disableOutputAlignment)
        outputBuffer += formatIntoBuffer(outputBuffer, "\t\t.align 4\n");
    if (program->proc.globalName && *program->proc.globalName)
        outputBuffer += formatIntoBuffer(outputBuffer, "%s_CodeEnd:\n", program->proc.globalName);
    for (mNode *macroNode = program->proc.trailingLabels.head; macroNode; macroNode = macroNode->next)
        if (macroNode->name)
            outputBuffer += formatIntoBuffer(outputBuffer, "%s:\n", macroNode->name);
    if (program->proc.mpgTargetAddress >= 0) {
        outputBuffer += formatIntoBuffer(outputBuffer, "\t.EndMPG\n");
        outputBuffer += formatIntoBuffer(outputBuffer, "\t.EndDmaData\n");
    }
    if (program->proc.globalName && *program->proc.globalName && program->proc.mpgTargetAddress >= 0)
        outputBuffer += formatIntoBuffer(outputBuffer, "%s_DmaEnd:\n", program->proc.globalName);
    return outputBuffer;
}

static inline unsigned char streamEndType(InstructionStream *stream)
{
    return stream->endType;
}

static inline void dumpFailedRow(ScheduleRow *workRow)
{
    workRow->refreshInstructionOperands(-1);
}

char *ScheduleTable::emit(char *outputBuffer, int mode, int *p1, int *p2)
{
    *p2 = 0;
    *p1 = 0;
    char done = 0;
    int len = rangeEnd - rangeStart;
    if (len < 0) {
        reportError("iTable::emit - not done!\n");
        return 0;
    }
    InstructionStream *owner = ownerStream;
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
            ScheduleRow *workRow = &rows[rowIndex];
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
        ScheduleRow blank;
        blank.clearEntries();
        for (int nopIndex = 0; nopIndex != nulls; nopIndex++) {
            int emissionFlags = 0;
            if (isEnd && nopIndex == 0) {
                emissionFlags = (unsigned char)(owner->trailingLabelFlags | 2);
                done = 1;
            }
            emissionFlags |= top;
            top = 0;
            outputBuffer = blank.emit(outputBuffer, emissionFlags, false, false);
            if (!outputBuffer) {
                reportError("failed to emit null ie for synth instruction!\n");
                return 0;
            }
        }
    } else {
        InstructionStream *stream = ownerStream;
        if (stream) {
            outputBuffer += formatIntoBuffer(outputBuffer, "; ");
            if (stream->hasLoopOptimization)
                outputBuffer += formatIntoBuffer(outputBuffer, "_LPOPT_");
            else
                outputBuffer += formatIntoBuffer(outputBuffer, "_LNOPT_");
            outputBuffer += formatIntoBuffer(outputBuffer, "w=[ ");
            if (stream->appliedOptimizationMask) {
                for (int optimizerIndex = 0; optimizerIndex != getOptimizerCount(); optimizerIndex++) {
                    int bit = 1 << optimizerIndex;
                    if (stream->appliedOptimizationMask & bit)
                        outputBuffer += formatIntoBuffer(outputBuffer, "%s ", getOptimizerName(optimizerIndex));
                }
            }
            outputBuffer += formatIntoBuffer(outputBuffer, "] %d [%d %d] %d   [%s]\n",
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
        ScheduleRow blank;
        blank.clearEntries();
        for (int emissionRowIndex = rangeStart; emissionRowIndex != end; emissionRowIndex++) {
            ScheduleRow *emissionRow = &blank;
            if (emissionRowIndex < rangeEnd)
                emissionRow = &rows[emissionRowIndex];
            int rowEmissionFlags = 0;
            if (!emissionRow->skip)
                (*p1)++;
            (*p2)++;
            if (isEnd) {
                if (emissionRowIndex == end - 2) {
                    rowEmissionFlags = (unsigned char)(stream->trailingLabelFlags | 2);
                    done = 1;
                }
            } else if (emissionRowIndex == rangeEnd - 1)
                rowEmissionFlags = (unsigned char)stream->trailingLabelFlags;
            rowEmissionFlags |= top;
            top = 0;
            switch (mode) {
            case 0:
                outputBuffer = emissionRow->emit(outputBuffer, rowEmissionFlags, false, true);
                if (!outputBuffer) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(emissionRow);
                    return 0;
                }
                break;
            case 1:
                *outputBuffer++ = ';';
                outputBuffer = emissionRow->emit(outputBuffer, rowEmissionFlags, true, true);
                if (!outputBuffer) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(emissionRow);
                    return 0;
                }
                *outputBuffer++ = ' ';
                outputBuffer = emissionRow->emit(outputBuffer, rowEmissionFlags, false, true);
                if (!outputBuffer) {
                    reportError("failed to emit instruction!\n");
                    dumpFailedRow(emissionRow);
                    return 0;
                }
                *outputBuffer++ = '\n';
                break;
            case 2:
                outputBuffer = emissionRow->emit(outputBuffer, rowEmissionFlags, true, true);
                if (!outputBuffer) {
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
    return outputBuffer;
fail:
    return 0;
}

extern bool disableEndBits;
extern char *stallTypeNames[3];
int countSetBits(unsigned int);
char *emitNop(char *buf, int flags, int *len);
char *emitInstr(Instr *in, char *buf, bool upper, int flags, bool b);

char *ScheduleRow::emit(char *outputBuffer, int flags, bool upper, bool unused)
{
    if (!outputBuffer) {
        reportError("iEntry::emit - null pointer!\n");
        return 0;
    }
    if (skip) {
        if (countEntries()) {
            reportError("can't skip a non-empty ie\n");
            return 0;
        }
        return outputBuffer;
    }
    outputBuffer += formatIntoBuffer(outputBuffer, "         ");
    int entryIndex;
    for (entryIndex = 0; entryIndex != 2; entryIndex++)
        if (entries[entryIndex].instruction)
            flags |= entries[entryIndex].instruction->instructionModifierBits;
    int len = 0;
    for (entryIndex = 0; entryIndex != 2; entryIndex++) {
        if (!entries[entryIndex].instruction)
            outputBuffer = emitNop(outputBuffer, flags, &len);
        else
            outputBuffer = emitInstr(entries[entryIndex].instruction, outputBuffer, false, flags, upper);
        if (!outputBuffer) {
            reportError("failed to emit upper for pipe %d ie\n", entryIndex);
            return 0;
        }
        flags = 0;
        if (entryIndex != 1)
            outputBuffer += formatIntoBuffer(outputBuffer, "         ");
    }
    if (maxStallLength) {
        outputBuffer += formatIntoBuffer(outputBuffer, "       ;\tSTALL");
        bool any = false;
        for (int stallTypeIndex = 0; stallTypeIndex != 3; stallTypeIndex++) {
            int bit = (stallTypeMask >> stallTypeIndex) & 1;
            if (bit) {
                outputBuffer += formatIntoBuffer(outputBuffer, "_%s", stallTypeNames[stallTypeIndex]);
                any = true;
            }
        }
        if (!any)
            reportWarning("reported a stall.. but dunno what type\n");
        int stall = maxStallLength;
        outputBuffer += formatIntoBuffer(outputBuffer, " ?%d", stall);
    }
    return outputBuffer + formatIntoBuffer(outputBuffer, "\n");
}

char *emitNop(char *outputBuffer, int flags, int *len)
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
    outputBuffer += formatIntoBuffer(outputBuffer, "%s", line);
    return outputBuffer;
}

char *emitInstr(Instr *emittedInstruction, char *outputBuffer, bool upper, int flags, bool symbolic)
{
    char line[320];
    char mods[16];
    char *modifierSource = formatInstructionModifiers(flags);
    char *lineCursor = line;
    char *modifierDestination = mods;
    while (*modifierSource)
        *modifierDestination++ = *modifierSource++;
    *modifierDestination = 0;
    if (emittedInstruction->tildePrefix || !emittedInstruction->opcodeInfo)
        lineCursor += formatIntoBuffer(lineCursor, "nop%s", mods);
    else {
        if (emittedInstruction->opcodeInfo->emitHook) {
            lineCursor = emittedInstruction->opcodeInfo->emitHook(line, emittedInstruction, upper, mods, symbolic, 0);
            if (!lineCursor) {
                reportError("emit hook failed for an instruction\n");
                return 0;
            }
        } else {
            lineCursor = emitGenericInstructionText(line, emittedInstruction, upper, mods, symbolic, 0);
            if (!lineCursor) {
                reportError("generic emit hook failed for an instruction\n");
                return 0;
            }
        }
    }
    for (int paddingIndex = lineCursor - line; paddingIndex < 50; paddingIndex++)
        *lineCursor++ = ' ';
    *lineCursor = 0;
    return outputBuffer + formatIntoBuffer(outputBuffer, "%s", line);
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
