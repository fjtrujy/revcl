#include "Program.h"
#include "Processor.h"
#include "instStream.h"
#include "os.h"
#include "decodedLine.h"

void disposeBackTrace(Program *p);
void dumpProcessorRegisterPools(Program *p);
bool parseReset(Program *p);
const char *linkTypeName(int type);
bool func_0804c03c(const char *a, const char *b);
bool emitReset(Program *p);
char *emitStats(Program *p, char *buf);
char *emitFileStart(Program *p, char *buf);
char *emitFileEnd(Program *p, char *buf);
const char *getOptimiserName(int id);

Program::Program(bool symbolic)
{
    symbolicOutput = symbolic;
    streamTail = 0;
    streamHead = 0;
    streamCount = 0;
    streamsById = 0;
}

Program::~Program()
{
    dispose();
}

bool Program::Init(CpuDef *cpu, const char *uniqueName, bool uniqueAll)
{
    if (!dispose()) {
        reportError("flowMon::Init() - failed to dispose\n");
        return false;
    }
    if (!cpu) {
        reportError("flowMon::init - bad cpu type\n");
        dispose();
        return false;
    }
    if (!proc.setUniqueName(uniqueName)) {
        reportError("flowMon::Init - failed to set name\n");
        dispose();
        return false;
    }
    proc.uniqueAll = uniqueAll;
    if (!proc.Init(cpu)) {
        reportError("flowMon::Init failed to init cpu object\n");
        return false;
    }
    return true;
}

bool Program::dispose()
{
    instStream *s = streamHead;
    while (s) {
        instStream *next = s->nextStream;
        s->reset();
        delete s;
        s = next;
    }
    streamTail = 0;
    streamHead = 0;
    streamCount = 0;
    if (streamsById)
        delete[] streamsById;
    streamsById = 0;
    disposeBackTrace(this);
    uniqueNamePrefixes.clearNodes();
    return true;
}


bool Program::addStream(instStream *s)
{
    if (!s)
        return false;
    if (s->owningProgram) {
        reportError("addistream - owner already set\n");
        return false;
    }
    if (streamHead) {
        streamTail->nextStream = s;
        s->previousStream = streamTail;
        s->nextStream = 0;
        streamTail = s;
    } else {
        streamHead = s;
        streamTail = s;
        s->nextStream = 0;
        s->previousStream = 0;
    }
    s->owningProgram = this;
    streamCount++;
    return true;
}

bool Program::insertStreamAfter(instStream *after, instStream *s)
{
    if (!s)
        return false;
    if (s->owningProgram) {
        reportError("addistream - owner already set\n");
        return false;
    }
    s->previousStream = after;
    if (after) {
        s->nextStream = after->nextStream;
        if (after->nextStream)
            after->nextStream->previousStream = s;
        else
            streamTail = s;
        after->nextStream = s;
    } else {
        s->nextStream = streamHead;
        if (streamHead)
            streamHead->previousStream = s;
        streamHead = s;
    }
    s->owningProgram = this;
    streamCount++;
    return true;
}

bool Program::removeStream(instStream *s)
{
    if (s->owningProgram != this) {
        reportError("didn't have owner! or not us!\n");
        return false;
    }
    if (s->nextStream)
        s->nextStream->previousStream = s->previousStream;
    else
        streamTail = s->previousStream;
    if (s->previousStream)
        s->previousStream->nextStream = s->nextStream;
    else
        streamHead = s->nextStream;
    s->owningProgram = 0;
    s->nextStream = 0;
    s->previousStream = 0;
    streamCount--;
    return true;
}

void dumpProgramStreamsAndRegisters(Program *p, int b)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream)
        s->dump(b);
    dumpProcessorRegisterPools(p);
}

instStream *Program::findStreamById(int id)
{
    instStream *s;
    for (s = streamHead; s && s->streamId != id; s = s->nextStream)
        ;
    return s;
}

instStream *Program::findStream(const char *name)
{
    if (!name)
        return 0;
    instStream *s;
    for (s = streamHead; s && !s->labels.findByNameIgnoreCase(name); s = s->nextStream)
        ;
    return s;
}

bool Program::retarget(instStream *from, instStream *to)
{
    if (from == to)
        return true;
    linkNode *next;
    for (linkNode *in = from->inputLinks.head; in; in = next) {
        next = in->next;
        instStream *src = in->otherStream;
        if (!src) {
            reportError("null ist pointer found on ist list\n");
            return false;
        }
        linkNode *out = src->outputLinks.findLinkToStream(from, in->linkType);
        if (!out) {
            reportError("Retarget:: failed to find matching output\n");
            return false;
        }
        out->otherStream = to;
        if (!from->inputLinks.unlink(in)) {
            reportError("Retarget::failed to unlink input from old\n");
            return false;
        }
        if (!to->inputLinks.relink(in)) {
            reportError("Retarget::failed to relink input to new\n");
            return false;
        }
        switch (out->linkType) {
        case 2:
        case 4:
        case 5:
            reportWarning("flowMon::can't reliably retarget a branch-to-register instruction\n");
            break;
        case 0:
            break;
        case 1:
        case 3: {
            Instr *br = src->getBranch();
            if (br && !br->changeBranchImmediate(to->streamName())) {
                reportError("failed to change branch immediate\n");
                exitAfterCleanup(-1);
            }
            break;
        }
        default:
            const char *typeName = linkTypeName(out->linkType);
            reportError("flowMon::Retarget - unexpected output type %d %s\n",
                          out->linkType, typeName);
            reportError("error retargeting %s->%s\n",
                          from->streamName(), to->streamName());
            return false;
        }
    }
    return true;
}

bool Program::decodeAndSplitStreams(Parser *parser, int a, bool newSyntax)
{
    if (!parseReset(this)) {
        reportError("flow::failed to reset cpu\n");
        return false;
    }
    int count = 0;
    DecodedLine *blocks = decodeSourceLines(this, parser, a, newSyntax, &count);
    if (!blocks) {
        reportError("flow: failed to build it from parse\n");
        return false;
    }
    if (count <= 0) {
        reportError("<=0 number of decoded line entries from source??\n");
        delete blocks;
        return false;
    }
    bool ok = splitParserStream(this, blocks, count);
    if (!ok) {
        for (int i = 0; i != count; i++)
            blocks[i].releaseContents();
        reportError("flow: failed to split parser stream\n");
    }
    delete[] blocks;
    return ok;
}

bool patchAllStreamLatencies(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (!s->patchLatencies())
            reportError("flowMon::PatchLatencies - failed for '%s'\n", s->streamName());
    }
    return true;
}

char *emitProgram(Program *p, char *out, int comments)
{
    instStream *s;
    int total = 0;
    if (!out) {
        reportError("flowMon::Emit - null output pointer\n");
        return 0;
    }
    char *buf = out;
    for (s = p->streamHead; s; s = s->nextStream) {
        if (s->unk_00) {
            buf += formatIntoBuffer(buf, "; === __LP__ %s:\n", s->streamName());
            for (int i = 0; i != s->unk_00; i++) {
                if (s->unk_04[i].unk_00 > 0) {
                    buf += formatIntoBuffer(buf, "; === %16s : optimal=%d clid=%d mlid=%d size=(%d) ",
                                         getOptimiserName(i), s->unk_04[i].unk_04, s->unk_04[i].unk_0c,
                                         s->unk_04[i].unk_08, s->unk_04[i].unk_00);
                    buf += formatIntoBuffer(buf, "\n");
                }
            }
        }
    }
    buf += formatIntoBuffer(buf, "; =================================================\n");
    buf += formatIntoBuffer(buf, "; flowMon::Emit() vcl 1.4beta7 produced this code:\n");
    emitReset(p);
    buf = emitFileStart(p, buf);
    if (!buf) {
        reportError("failed to emit the start of the source file\n");
        return 0;
    }
    for (s = p->streamHead; s; s = s->nextStream) {
        s->dump(0);
        int n = 0, m = 0;
        s->unk_2c8 = total;
        buf = s->emit(buf, comments, &n, &m);
        if (!buf) {
            reportError("flowMon : failed to write a stream\n");
            return 0;
        }
        total += n;
    }
    buf = emitFileEnd(p, buf);
    if (!buf) {
        reportError("failed to emit end of file\n");
        return 0;
    }
    buf += formatIntoBuffer(buf, ";\tiCount=%d\n", total);
    buf = emitStats(p, buf);
    if (!buf) {
        reportError("failed to emit cpu stats\n");
        return 0;
    }
    return buf;
}

bool setUniqueName(Program *p, const char *name)
{
    return p->proc.setUniqueName(name);
}

const char *getUniqueName(Program *p)
{
    return p->proc.getUniqueName();
}

char *emitLabels(Program *p, char *buf)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        buf = s->emitLabels(buf);
        if (!buf) {
            reportError("failed to emit user labels for '%s'\n", s->streamName());
            return 0;
        }
    }
    return buf;
}

bool isReferencedLabel(Program *p, char *name)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        if (s->startType == 2) {
            linkNode *l = s->outputLinks.head;
            if (l) {
                linkNode *next = l->next;
                if (next) {
                    reportError("found entry point with more than one exit\n");
                    exitAfterCleanup(-1);
                }
                instStream *t = l->otherStream;
                if (!t) {
                    reportError("no stream for ir\n");
                    exitAfterCleanup(-1);
                }
                if (t->labels.findByExactName(name))
                    return true;
            }
        }
        if (s->getBranch()) {
            char *tgt = s->getBranchTargetImmediate();
            if (tgt && func_0804c03c(tgt, name))
                return true;
        }
    }
    return p->uniqueNamePrefixes.findByExactName(name) != 0;
}

bool func_0806f4a0(Program *p, instStream *s)
{
    mList *l = &s->labels;
    for (mNode *n = l->head; n; n = n->next) {
        if (p->proc.exportedSymbols.findByExactName(n->name))
            return true;
    }
    return false;
}

bool isExportedSymbol(Program *p, char *name)
{
    return p->proc.exportedSymbols.findByExactName(name) != 0;
}

bool recalcFlowUIDs(Program *p)
{
    instStream *s = p->streamHead;
    int n = 0;
    p->flowIdCount = -1;
    for (; s; s = s->nextStream) {
        if (!s->getCodeMode()) {
            reportError("recalcFlowUIDs - found bad code mode at %s\n", s->streamName());
            return false;
        }
        s->streamId = n++;
    }
    if (n != p->streamCount) {
        reportError("recalcFlowUIDs - there was a hole somewhere?\n");
        return false;
    }
    p->flowIdCount = n;
    int size = p->flowIdCount + 1;
    if (p->streamsById) {
        delete p->streamsById;
        p->streamsById = 0;
    }
    p->streamsById = new instStream *[size];
    if (!p->streamsById) {
        reportError("failed to get a new linear table\n");
        return false;
    }
    for (s = p->streamHead; s; s = s->nextStream)
        p->streamsById[s->streamId] = s;
    return true;
}

bool func_0806f610(Program *p)
{
    if (!p->proc.func_0806bd40()) {
        reportError("failed to alloc reset the cpu\n");
        return false;
    }
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        if (!wt) {
            reportError("failed to get work table\n");
            return false;
        }
        wt->allocationMode = 1;
    }
    return true;
}

bool func_0806f6a4(Program *p)
{
    if (!p->proc.func_0806bd88()) {
        reportError("failed to alloc reset the cpu\n");
        return false;
    }
    return true;
}

static inline void printNamedLink(const char *fmt, linkNode *l, const char *name)
{
    int other = l->otherStream->streamId;
    const char *type = linkTypeName(l->linkType);
    reportInfo(fmt, type, other, name);
}

static inline bool isEntryStream(instStream *s) { return s->startType == 2; }
static inline bool isExitStream(instStream *s) { return s->endType == 3; }

void dumpFlow(Program *p)
{
    instStream *s = p->streamHead;
    reportInfo("dumpFlow-------------------\n");
    for (; s; s = s->nextStream) {
        const char *tag = "";
        if (isEntryStream(s))
            tag = " ---ENTRY";
        if (isExitStream(s))
            tag = " ---EXIT";
        bool fallThrough = false;
        int id = s->streamId;
        reportInfo("[%5d] %s %s   <--\n", id, s->streamName(), tag);
        linkNode *l;
        for (l = s->inputLinks.head; l; l = l->next) {
            if (l->linkType) {
                const char *name = l->otherStream->streamName();
                printNamedLink("       <--[(%s) [%5d]%s]\n", l, name);
            }
        }
        for (l = s->outputLinks.head; l; l = l->next) {
            if (l->linkType) {
                const char *name = l->otherStream->streamName();
                printNamedLink("     --> %s [%5d]%s\n", l, name);
            } else
                fallThrough = true;
        }
        if (fallThrough) {
            reportInfo("    |\n");
            reportInfo("    V\n");
        } else
            reportInfo("\n");
    }
    reportInfo("dumpFlow - done\n");
}
