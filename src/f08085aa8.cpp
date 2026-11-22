#include "bindExternals_shared.h"

#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "os.h"
#include "flowBlock.h"


bool splitParserStream(Program *p, flowBlock *blocks, int count)
{
    int i = 0;
    if (!blocks) {
        func_0804ad38("flowMon::Build - passed a null table\n");
        return false;
    }
    if (count <= 0) {
        func_0804ad38("flowMon::Build - strange number of entries in the input table\n");
        return false;
    }

    for (; i != count; i++) {
        flowBlock *b = &blocks[i];
        for (int k = 0; k != 2; k++) {
            VclPtrWord in = (VclPtrWord)b->unk_0c[k];
            if (!in || !((Instr *)in)->unk_1388)
                continue;
            char *target = ((Instr *)in)->getBranchTargetImmediate();
            if (target) {
                flowBlock *f = func_0808435c(blocks, count, target);
                if (!f) {
                    func_0804ad84("vcl couldn't find the label '%s' refered to in instruction :\n", target);
                    ((Instr *)in)->leDump(-1);
                    return false;
                }
                f->unk_04 = 1;
            } else {
                char *tags = ((Instr *)in)->unk_139c;
                if (!tags)
                    continue;
                char *tag = tags;
                if (!*tag)
                    continue;
                do {
                    char buf[2064];
                    char *scan = tag;
                    char *d = buf;
                    while (*scan != ':')
                        *d++ = *scan++;
                    *d = 0;
                    flowBlock *f = func_0808435c(blocks, count, buf);
                    if (!f) {
                        func_0804ad38("failed to find a tagged label '%s' attached to branch:\n", buf);
                        ((Instr *)in)->leDump(-1);
                        return false;
                    }
                    if (!p->unk_88.func_0804b234(buf) && !p->unk_88.func_0804b2d4(buf)) {
                        func_0804ad38("failed to add tag name '%s' to the global tag list\n", buf);
                        return false;
                    }
                    f->unk_04 = 1;
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
        flowBlock *b = &blocks[i];
        char kind = b->unk_05;
        switch ((unsigned char)kind) {
        case 1:
        case 3: {
            unsigned int nextMode = (unsigned char)kind;
            if (mode != nextMode || sub != (unsigned char)b->unk_06) {
                uid++;
                mode = nextMode;
                sub = b->unk_06;
            }
            break;
        }
        case 6:
            uid++;
            break;
        case 0:
            func_0804ad38("flow::Build : hit a bad ientry.at line %d\n", i);
            b->func_08084268(i);
            return false;
        default:
            func_0804ad38("calculateFlowAndGLobalUID's : hit a totally bogus ientry\n");
            return false;
        }
        if (i && b[-1].unk_05 == 6)
            uid++;
        if (b->unk_14.func_0804b794())
            uid++;
        int f = b->unk_0a;
        if ((short)f > 0) {
            if (mode != 3) {
                func_0804ad38("+ve flow uid in non raw sequence\n");
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
        if (b->unk_04)
            uid++;
        int id = uid;
        for (int k = 0; k != 2; k++) {
            Instr *in = b->unk_0c[k];
            if (!in)
                continue;
            int serial = g_080b33a8++;
            in->unk_13e0 = serial;
            char t = in->unk_13cd;
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
                func_0804ad84("found a branch delay slot with no instruction in in RAW mode..\n");
                func_0804ad84("to make life easier for itself - vcl treats this as an error..\n");
                func_0804ad84("this includes.. having a BDL slot as a branch target.. in this case\n");
                return false;
            }
            if (in->unk_1388) {
                if (delay) {
                    func_0804ad84("found a branch in a branch delay slot\n");
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
        b->unk_08 = id;
    }
    p->unk_ac = uid;

    mList unused;
    i = 0;
    while (i < count) {
        flowBlock *b = &blocks[i];
        int end = i;
        short id = b->unk_08;
        mode = b->unk_05;
        sub = b->unk_06;
        if (end < count) {
            flowBlock *q = b;
            do {
                int candidateId = q->unk_08;
                if (id != (short)candidateId)
                    break;
                if (q->func_080843d0()) {
                    if (mode == 1) {
                        mode = 4;
                    } else {
                        func_0804ad84("mismatched synth instruction encountered\n");
                        return false;
                    }
                }
                end++;
                q++;
            } while (end < count);
        }
        int n = end - i;
        instStream *s = new instStream;
        if (!s) goto allocationOfStreamFailed;
        goto afterNHandler;
        allocationOfStreamFailed:
            func_0804ad38("flowMon::build - new failed\n");
            return false;
        afterNHandler:;
        s->unk_2c0 = id;
        mList *labels = &b->unk_14;
        if (!labels->unk_08) {
            int nameLength = mstrlen(getUniqueName(p));
            char *name = new char[nameLength + 128];
            if (name) {
                func_0804adbc(name, "__v_%s_%d", getUniqueName(p), id);
                if (!s->unk_110.func_0804b444(name)) {
                    func_0804ad38("failed to creat add a default name node\n");
                    delete[] name;
                }
                delete[] name;
            } else {
                goto labelError;
            }
        } else if (!s->unk_110.func_0804b4d0(labels)) {
            goto transferError;
        }
        s->unk_2d4 |= s->unk_110.func_0804b7b4();
        goto afterTError;
        transferError:
            func_0804ad38("failed to grab entries from in at top\n");
            return false;
        afterTError:;
        goto afterAError;
        labelError:
            func_0804ad38("failed to allocate new label\n");
            return false;
        afterAError:;
        switch (mode) {
        case 1:
            if (!buildUta(p, s, b, n)) {
                func_0804acb0("buildUta table failed for some reason\n");
                return false;
            }
            break;
        case 6:
            if (!buildPassThru(p, s, b, n)) {
                func_0804acb0("buildPass failed for some reason\n");
                return false;
            }
            break;
        case 3:
            if (!buildRaw(p, s, b, n, sub)) {
                func_0804acb0("buildRaw table failed for some reason\n");
                return false;
            }
            break;
        case 4:
            if (!buildSynth(p, s, b, n)) {
                func_0804acb0("buildSynth table failed for some reason\n");
                return false;
            }
            break;
        default:
            func_0804ad38("bad decMode for builder switch\n");
            return false;
        }
        for (int k = 0; k != n; k++)
            blocks[i + k].func_080842f0();
        if (!p->addStream(s))
            goto streamError;
        s->unk_218 = &s->unk_148;
        i = end;
        goto afterSHandler;
        streamError:
            func_0804ad38("flowBuild: failed to add istream\n");
            return false;
        afterSHandler:;
    }
    if (!expandRawTables(p)) {
        func_0804ad38("failed to expand raw tables\n");
        return false;
    }
    if (!resolveStreamBranches(p)) {
        func_0804ad38("failed to resolve branches after constructing streams\n");
        return false;
    }
    if (p->proc.unk_60 && !p->doUnique()) {
        func_0804ad38("failed to do unique processing\n");
        return false;
    }
    if (!bindDependencies(p)) {
        func_0804ad38("bindDependencies1 failed\n");
        return false;
    }
    if (!recalcFlowUIDs(p)) {
        func_0804ad38("failed to recalc flow uids\n");
        return false;
    }
    return true;
}
