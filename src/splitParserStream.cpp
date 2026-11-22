#include "bindExternals_shared.h"

#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "decodedLine.h"


bool splitParserStream(Program *program, DecodedLine *blocks, int count)
{
    int lineIndex = 0;
    if (!blocks) {
        reportError("flowMon::Build - passed a null table\n");
        return false;
    }
    if (count <= 0) {
        reportError("flowMon::Build - strange number of entries in the input table\n");
        return false;
    }

    for (; lineIndex != count; lineIndex++) {
        DecodedLine *decodedLine = &blocks[lineIndex];
        for (int instructionIndex = 0; instructionIndex != 2; instructionIndex++) {
            VclPtrWord instructionWord = (VclPtrWord)decodedLine->instructions[instructionIndex];
            if (!instructionWord || !((Instr *)instructionWord)->branchFlags)
                continue;
            char *target = ((Instr *)instructionWord)->getBranchTargetImmediate();
            if (target) {
                DecodedLine *targetLine = findDecodedLineByLabel(blocks, count, target);
                if (!targetLine) {
                    reportUserMessage("vcl couldn't find the label '%s' refered to in instruction :\n", target);
                    ((Instr *)instructionWord)->leDump(-1);
                    return false;
                }
                targetLine->isBranchTarget = 1;
            } else {
                char *tags = ((Instr *)instructionWord)->branchLabelTags;
                if (!tags)
                    continue;
                char *tag = tags;
                if (!*tag)
                    continue;
                do {
                    char buf[2064];
                    char *scan = tag;
                    char *outputCursor = buf;
                    while (*scan != ':')
                        *outputCursor++ = *scan++;
                    *outputCursor = 0;
                    DecodedLine *matchedTagLine = findDecodedLineByLabel(blocks, count, buf);
                    if (!matchedTagLine) {
                        reportError("failed to find a tagged label '%s' attached to branch:\n", buf);
                        ((Instr *)instructionWord)->leDump(-1);
                        return false;
                    }
                    if (!program->uniqueNamePrefixes.findByExactName(buf) && !program->uniqueNamePrefixes.findOrAppendByExactName(buf)) {
                        reportError("failed to add tag name '%s' to the global tag list\n", buf);
                        return false;
                    }
                    matchedTagLine->isBranchTarget = 1;
                    tag = scan + 1;
                } while (*tag);
            }
        }
    }

    int delay = 0;
    int lastUid = 0;
    unsigned char mode = 0;
    unsigned char sub = 0;
    short uid = 1;
    for (lineIndex = 0; lineIndex != count; lineIndex++) {
        DecodedLine *currentDecodedLine = &blocks[lineIndex];
        char kind = currentDecodedLine->decodeMode;
        switch ((unsigned char)kind) {
        case 1:
        case 3: {
            unsigned int nextMode = (unsigned char)kind;
            if (mode != nextMode || sub != (unsigned char)currentDecodedLine->decodeSubMode) {
                uid++;
                mode = nextMode;
                sub = currentDecodedLine->decodeSubMode;
            }
            break;
        }
        case 6:
            uid++;
            break;
        case 0:
            reportError("flow::Build : hit a bad ientry.at line %d\n", lineIndex);
            currentDecodedLine->refreshInstructionOperands(lineIndex);
            return false;
        default:
            reportError("calculateFlowAndGLobalUID's : hit a totally bogus ientry\n");
            return false;
        }
        if (lineIndex && currentDecodedLine[-1].decodeMode == 6)
            uid++;
        if (currentDecodedLine->labels.combinedFlags())
            uid++;
        int rawLoopId = currentDecodedLine->rawLoopId;
        if ((short)rawLoopId > 0) {
            if (mode != 3) {
                reportError("+ve flow uid in non raw sequence\n");
                return false;
            }
            if ((short)rawLoopId != lastUid) {
                uid++;
                lastUid = (short)rawLoopId;
            }
        } else if (lastUid > 0) {
            lastUid = 0;
            uid++;
        }
        if (currentDecodedLine->isBranchTarget)
            uid++;
        int flowId = uid;
        for (int blockInstructionIndex = 0; blockInstructionIndex != 2; blockInstructionIndex++) {
            Instr *blockInstruction = currentDecodedLine->instructions[blockInstructionIndex];
            if (!blockInstruction)
                continue;
            int serial = nextInstructionSerial++;
            blockInstruction->instructionSerial = serial;
            char syntheticKind = blockInstruction->syntheticKind;
            switch ((unsigned char)syntheticKind) {
            case 1:
            case 3:
            case 10:
            case 11:
                uid++;
                flowId++;
                break;
            case 2:
            case 4:
            case 5:
                uid++;
                break;
            }
            if ((syntheticKind || mode == 1) && delay) {
                reportUserMessage("found a branch delay slot with no instruction in in RAW mode..\n");
                reportUserMessage("to make life easier for itself - vcl treats this as an error..\n");
                reportUserMessage("this includes.. having a BDL slot as a branch target.. in this case\n");
                return false;
            }
            if (blockInstruction->branchFlags) {
                if (delay) {
                    reportUserMessage("found a branch in a branch delay slot\n");
                    blockInstruction->leDump();
                    return false;
                }
                delay = (mode == 3) + 1;
            }
        }
        if (delay) {
            if (delay == 1)
                uid++;
            delay--;
        }
        currentDecodedLine->flowId = flowId;
    }
    program->flowIdCount = uid;

    mList unused;
    lineIndex = 0;
    while (lineIndex < count) {
        DecodedLine *blockStartLine = &blocks[lineIndex];
        int end = lineIndex;
        short blockFlowId = blockStartLine->flowId;
        mode = blockStartLine->decodeMode;
        sub = blockStartLine->decodeSubMode;
        if (end < count) {
            DecodedLine *scanLine = blockStartLine;
            do {
                int candidateId = scanLine->flowId;
                if (blockFlowId != (short)candidateId)
                    break;
                if (scanLine->hasFlowStartSynthetic()) {
                    if (mode == 1) {
                        mode = 4;
                    } else {
                        reportUserMessage("mismatched synth instruction encountered\n");
                        return false;
                    }
                }
                end++;
                scanLine++;
            } while (end < count);
        }
        int streamLineCount = end - lineIndex;
        instStream *stream = new instStream;
        if (!stream) goto allocationOfStreamFailed;
        goto afterNHandler;
        allocationOfStreamFailed:
            reportError("flowMon::build - new failed\n");
            return false;
        afterNHandler:;
        stream->streamId = blockFlowId;
        mList *labels = &blockStartLine->labels;
        if (!labels->nodeCount) {
            int nameLength = mstrlen(getUniqueName(program));
            char *name = new char[nameLength + 128];
            if (name) {
                formatIntoBuffer(name, "__v_%s_%d", getUniqueName(program), blockFlowId);
                if (!stream->labels.appendNamedNode(name)) {
                    reportError("failed to creat add a default name node\n");
                    delete[] name;
                }
                delete[] name;
            } else {
                goto labelError;
            }
        } else if (!stream->labels.transferNodesFrom(labels)) {
            goto transferError;
        }
        stream->leadingLabelFlags |= stream->labels.takeCombinedFlags();
        goto afterTError;
        transferError:
            reportError("failed to grab entries from in at top\n");
            return false;
        afterTError:;
        goto afterAError;
        labelError:
            reportError("failed to allocate new label\n");
            return false;
        afterAError:;
        switch (mode) {
        case 1:
            if (!buildUta(program, stream, blockStartLine, streamLineCount)) {
                reportInfo("buildUta table failed for some reason\n");
                return false;
            }
            break;
        case 6:
            if (!buildPassThru(program, stream, blockStartLine, streamLineCount)) {
                reportInfo("buildPass failed for some reason\n");
                return false;
            }
            break;
        case 3:
            if (!buildRaw(program, stream, blockStartLine, streamLineCount, sub)) {
                reportInfo("buildRaw table failed for some reason\n");
                return false;
            }
            break;
        case 4:
            if (!buildSynth(program, stream, blockStartLine, streamLineCount)) {
                reportInfo("buildSynth table failed for some reason\n");
                return false;
            }
            break;
        default:
            reportError("bad decMode for builder switch\n");
            return false;
        }
        for (int lineOffset = 0; lineOffset != streamLineCount; lineOffset++)
            blocks[lineIndex + lineOffset].releaseContents();
        if (!program->addStream(stream))
            goto streamError;
        stream->workTablePtr = &stream->sourceTable;
        lineIndex = end;
        goto afterSHandler;
        streamError:
            reportError("flowBuild: failed to add istream\n");
            return false;
        afterSHandler:;
    }
    if (!expandRawTables(program)) {
        reportError("failed to expand raw tables\n");
        return false;
    }
    if (!resolveStreamBranches(program)) {
        reportError("failed to resolve branches after constructing streams\n");
        return false;
    }
    if (program->proc.uniqueAll && !program->doUnique()) {
        reportError("failed to do unique processing\n");
        return false;
    }
    if (!bindDependencies(program)) {
        reportError("bindDependencies1 failed\n");
        return false;
    }
    if (!recalcFlowUIDs(program)) {
        reportError("failed to recalc flow uids\n");
        return false;
    }
    return true;
}
