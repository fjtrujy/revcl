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
    int i = 0;
    if (!blocks) {
        reportError("flowMon::Build - passed a null table\n");
        return false;
    }
    if (count <= 0) {
        reportError("flowMon::Build - strange number of entries in the input table\n");
        return false;
    }

    for (; i != count; i++) {
        DecodedLine *decodedLine = &blocks[i];
        for (int instructionIndex = 0; instructionIndex != 2; instructionIndex++) {
            VclPtrWord in = (VclPtrWord)decodedLine->instructions[instructionIndex];
            if (!in || !((Instr *)in)->branchFlags)
                continue;
            char *target = ((Instr *)in)->getBranchTargetImmediate();
            if (target) {
                DecodedLine *f = findDecodedLineByLabel(blocks, count, target);
                if (!f) {
                    reportUserMessage("vcl couldn't find the label '%s' refered to in instruction :\n", target);
                    ((Instr *)in)->leDump(-1);
                    return false;
                }
                f->isBranchTarget = 1;
            } else {
                char *tags = ((Instr *)in)->branchLabelTags;
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
                    DecodedLine *f = findDecodedLineByLabel(blocks, count, buf);
                    if (!f) {
                        reportError("failed to find a tagged label '%s' attached to branch:\n", buf);
                        ((Instr *)in)->leDump(-1);
                        return false;
                    }
                    if (!program->uniqueNamePrefixes.findByExactName(buf) && !program->uniqueNamePrefixes.findOrAppendByExactName(buf)) {
                        reportError("failed to add tag name '%s' to the global tag list\n", buf);
                        return false;
                    }
                    f->isBranchTarget = 1;
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
    for (i = 0; i != count; i++) {
        DecodedLine *b = &blocks[i];
        char kind = b->decodeMode;
        switch ((unsigned char)kind) {
        case 1:
        case 3: {
            unsigned int nextMode = (unsigned char)kind;
            if (mode != nextMode || sub != (unsigned char)b->decodeSubMode) {
                uid++;
                mode = nextMode;
                sub = b->decodeSubMode;
            }
            break;
        }
        case 6:
            uid++;
            break;
        case 0:
            reportError("flow::Build : hit a bad ientry.at line %d\n", i);
            b->refreshInstructionOperands(i);
            return false;
        default:
            reportError("calculateFlowAndGLobalUID's : hit a totally bogus ientry\n");
            return false;
        }
        if (i && b[-1].decodeMode == 6)
            uid++;
        if (b->labels.combinedFlags())
            uid++;
        int f = b->rawLoopId;
        if ((short)f > 0) {
            if (mode != 3) {
                reportError("+ve flow uid in non raw sequence\n");
                return false;
            }
            if ((short)f != lastUid) {
                uid++;
                lastUid = (short)f;
            }
        } else if (lastUid > 0) {
            lastUid = 0;
            uid++;
        }
        if (b->isBranchTarget)
            uid++;
        int id = uid;
        for (int k = 0; k != 2; k++) {
            Instr *in = b->instructions[k];
            if (!in)
                continue;
            int serial = nextInstructionSerial++;
            in->instructionSerial = serial;
            char t = in->syntheticKind;
            switch ((unsigned char)t) {
            case 1:
            case 3:
            case 10:
            case 11:
                uid++;
                id++;
                break;
            case 2:
            case 4:
            case 5:
                uid++;
                break;
            }
            if ((t || mode == 1) && delay) {
                reportUserMessage("found a branch delay slot with no instruction in in RAW mode..\n");
                reportUserMessage("to make life easier for itself - vcl treats this as an error..\n");
                reportUserMessage("this includes.. having a BDL slot as a branch target.. in this case\n");
                return false;
            }
            if (in->branchFlags) {
                if (delay) {
                    reportUserMessage("found a branch in a branch delay slot\n");
                    in->leDump();
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
        b->flowId = id;
    }
    program->flowIdCount = uid;

    mList unused;
    i = 0;
    while (i < count) {
        DecodedLine *b = &blocks[i];
        int end = i;
        short id = b->flowId;
        mode = b->decodeMode;
        sub = b->decodeSubMode;
        if (end < count) {
            DecodedLine *q = b;
            do {
                int candidateId = q->flowId;
                if (id != (short)candidateId)
                    break;
                if (q->hasFlowStartSynthetic()) {
                    if (mode == 1) {
                        mode = 4;
                    } else {
                        reportUserMessage("mismatched synth instruction encountered\n");
                        return false;
                    }
                }
                end++;
                q++;
            } while (end < count);
        }
        int n = end - i;
        instStream *stream = new instStream;
        if (!stream) goto allocationOfStreamFailed;
        goto afterNHandler;
        allocationOfStreamFailed:
            reportError("flowMon::build - new failed\n");
            return false;
        afterNHandler:;
        stream->streamId = id;
        mList *labels = &b->labels;
        if (!labels->nodeCount) {
            int nameLength = mstrlen(getUniqueName(program));
            char *name = new char[nameLength + 128];
            if (name) {
                formatIntoBuffer(name, "__v_%s_%d", getUniqueName(program), id);
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
            if (!buildUta(program, stream, b, n)) {
                reportInfo("buildUta table failed for some reason\n");
                return false;
            }
            break;
        case 6:
            if (!buildPassThru(program, stream, b, n)) {
                reportInfo("buildPass failed for some reason\n");
                return false;
            }
            break;
        case 3:
            if (!buildRaw(program, stream, b, n, sub)) {
                reportInfo("buildRaw table failed for some reason\n");
                return false;
            }
            break;
        case 4:
            if (!buildSynth(program, stream, b, n)) {
                reportInfo("buildSynth table failed for some reason\n");
                return false;
            }
            break;
        default:
            reportError("bad decMode for builder switch\n");
            return false;
        }
        for (int k = 0; k != n; k++)
            blocks[i + k].releaseContents();
        if (!program->addStream(stream))
            goto streamError;
        stream->workTablePtr = &stream->sourceTable;
        i = end;
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
