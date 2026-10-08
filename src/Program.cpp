#include "Program.h"
#include "Processor.h"
#include "instStream.h"
#include "os.h"
#include "flowBlock.h"

void disposeBackTrace(Program *p);
void func_0806be68(Program *p);
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
    unk_9c = symbolic;
    unk_a4 = 0;
    unk_a0 = 0;
    unk_a8 = 0;
    unk_b0 = 0;
}

Program::~Program()
{
    dispose();
}

bool Program::Init(CpuDef *cpu, const char *uniqueName, bool uniqueAll)
{
    if (!dispose()) {
        func_0804ad38("flowMon::Init() - failed to dispose\n");
        return false;
    }
    if (!cpu) {
        func_0804ad38("flowMon::init - bad cpu type\n");
        dispose();
        return false;
    }
    if (!proc.setUniqueName(uniqueName)) {
        func_0804ad38("flowMon::Init - failed to set name\n");
        dispose();
        return false;
    }
    proc.unk_60 = uniqueAll;
    if (!proc.Init(cpu)) {
        func_0804ad38("flowMon::Init failed to init cpu object\n");
        return false;
    }
    return true;
}

bool Program::dispose()
{
    instStream *s = unk_a0;
    while (s) {
        instStream *next = s->unk_108;
        s->reset();
        delete s;
        s = next;
    }
    unk_a4 = 0;
    unk_a0 = 0;
    unk_a8 = 0;
    if (unk_b0)
        delete[] unk_b0;
    unk_b0 = 0;
    disposeBackTrace(this);
    unk_88.func_0804b1b0();
    return true;
}

bool Program::addStream(instStream *s)
{
    if (!s)
        return false;
    if (s->unk_104) {
        func_0804ad38("addistream - owner already set\n");
        return false;
    }
    if (unk_a0) {
        unk_a4->unk_108 = s;
        s->unk_10c = unk_a4;
        s->unk_108 = 0;
        unk_a4 = s;
    } else {
        unk_a0 = s;
        unk_a4 = s;
        s->unk_108 = 0;
        s->unk_10c = 0;
    }
    s->unk_104 = this;
    unk_a8++;
    return true;
}

bool Program::insertStreamAfter(instStream *after, instStream *s)
{
    if (!s)
        return false;
    if (s->unk_104) {
        func_0804ad38("addistream - owner already set\n");
        return false;
    }
    s->unk_10c = after;
    if (after) {
        s->unk_108 = after->unk_108;
        if (after->unk_108)
            after->unk_108->unk_10c = s;
        else
            unk_a4 = s;
        after->unk_108 = s;
    } else {
        s->unk_108 = unk_a0;
        if (unk_a0)
            unk_a0->unk_10c = s;
        unk_a0 = s;
    }
    s->unk_104 = this;
    unk_a8++;
    return true;
}

bool Program::removeStream(instStream *s)
{
    if (s->unk_104 != this) {
        func_0804ad38("didn't have owner! or not us!\n");
        return false;
    }
    if (s->unk_108)
        s->unk_108->unk_10c = s->unk_10c;
    else
        unk_a4 = s->unk_10c;
    if (s->unk_10c)
        s->unk_10c->unk_108 = s->unk_108;
    else
        unk_a0 = s->unk_108;
    s->unk_104 = 0;
    s->unk_108 = 0;
    s->unk_10c = 0;
    unk_a8--;
    return true;
}

void func_0806ed78(Program *p, int b)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108)
        s->dump(b);
    func_0806be68(p);
}

instStream *Program::findStreamById(int id)
{
    instStream *s;
    for (s = unk_a0; s && s->unk_2c0 != id; s = s->unk_108)
        ;
    return s;
}

instStream *Program::findStream(const char *name)
{
    if (!name)
        return 0;
    instStream *s;
    for (s = unk_a0; s && !s->unk_110.func_0804b284(name); s = s->unk_108)
        ;
    return s;
}

bool Program::retarget(instStream *from, instStream *to)
{
    if (from == to)
        return true;
    linkNode *next;
    for (linkNode *in = from->unk_128.unk_00; in; in = next) {
        next = in->unk_08;
        instStream *src = in->unk_0c;
        if (!src) {
            func_0804ad38("null ist pointer found on ist list\n");
            return false;
        }
        linkNode *out = src->unk_134.func_0806dff4(from, in->unk_14);
        if (!out) {
            func_0804ad38("Retarget:: failed to find matching output\n");
            return false;
        }
        out->unk_0c = to;
        if (!from->unk_128.unlink(in)) {
            func_0804ad38("Retarget::failed to unlink input from old\n");
            return false;
        }
        if (!to->unk_128.relink(in)) {
            func_0804ad38("Retarget::failed to relink input to new\n");
            return false;
        }
        switch (out->unk_14) {
        case 2:
        case 4:
        case 5:
            func_0804ac7c("flowMon::can't reliably retarget a branch-to-register instruction\n");
            break;
        case 0:
            break;
        case 1:
        case 3: {
            Instr *br = src->getBranch();
            if (br && !br->changeBranchImmediate(to->func_0806d674())) {
                func_0804ad38("failed to change branch immediate\n");
                func_0804adf0(-1);
            }
            break;
        }
        default:
            func_0804ad38("flowMon::Retarget - unexpected output type %d %s\n",
                          out->unk_14, linkTypeName(out->unk_14));
            func_0804ad38("error retargeting %s->%s\n",
                          from->func_0806d674(), to->func_0806d674());
            return false;
        }
    }
    return true;
}

bool Program::func_0806efc4(Parser *parser, int a, bool newSyntax)
{
    if (!parseReset(this)) {
        func_0804ad38("flow::failed to reset cpu\n");
        return false;
    }
    int count = 0;
    flowBlock *blocks = func_08083768(this, parser, a, newSyntax, &count);
    if (!blocks) {
        func_0804ad38("flow: failed to build it from parse\n");
        return false;
    }
    if (count <= 0) {
        func_0804ad38("<=0 number of decoded line entries from source??\n");
        delete blocks;
        return false;
    }
    bool ok = splitParserStream(this, blocks, count);
    if (!ok) {
        for (int i = 0; i != count; i++)
            blocks[i].func_080842f0();
        func_0804ad38("flow: failed to split parser stream\n");
    }
    delete[] blocks;
    return ok;
}

bool func_0806f0e8(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (!s->patchLatencies())
            func_0804ad38("flowMon::PatchLatencies - failed for '%s'\n", s->func_0806d674());
    }
    return true;
}

char *emitProgram(Program *p, char *out, int comments)
{
    instStream *s;
    int total = 0;
    if (!out) {
        func_0804ad38("flowMon::Emit - null output pointer\n");
        return 0;
    }
    char *buf = out;
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_00) {
            buf += func_0804adbc(buf, "; === __LP__ %s:\n", s->func_0806d674());
            for (int i = 0; i != s->unk_00; i++) {
                if (s->unk_04[i].unk_00 > 0) {
                    buf += func_0804adbc(buf, "; === %16s : optimal=%d clid=%d mlid=%d size=(%d) ",
                                         getOptimiserName(i), s->unk_04[i].unk_04, s->unk_04[i].unk_0c,
                                         s->unk_04[i].unk_08, s->unk_04[i].unk_00);
                    buf += func_0804adbc(buf, "\n");
                }
            }
        }
    }
    buf += func_0804adbc(buf, "; =================================================\n");
    buf += func_0804adbc(buf, "; flowMon::Emit() vcl 1.4beta7 produced this code:\n");
    emitReset(p);
    buf = emitFileStart(p, buf);
    if (!buf) {
        func_0804ad38("failed to emit the start of the source file\n");
        return 0;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        s->dump(0);
        int n = 0, m = 0;
        s->unk_2c8 = total;
        buf = s->emit(buf, comments, &n, &m);
        if (!buf) {
            func_0804ad38("flowMon : failed to write a stream\n");
            return 0;
        }
        total += n;
    }
    buf = emitFileEnd(p, buf);
    if (!buf) {
        func_0804ad38("failed to emit end of file\n");
        return 0;
    }
    buf += func_0804adbc(buf, ";\tiCount=%d\n", total);
    buf = emitStats(p, buf);
    if (!buf) {
        func_0804ad38("failed to emit cpu stats\n");
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
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        buf = s->emitLabels(buf);
        if (!buf) {
            func_0804ad38("failed to emit user labels for '%s'\n", s->func_0806d674());
            return 0;
        }
    }
    return buf;
}

bool isReferencedLabel(Program *p, char *name)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_2cc == 2) {
            linkNode *l = s->unk_134.unk_00;
            if (l) {
                linkNode *next = l->unk_08;
                if (next) {
                    func_0804ad38("found entry point with more than one exit\n");
                    func_0804adf0(-1);
                }
                instStream *t = l->unk_0c;
                if (!t) {
                    func_0804ad38("no stream for ir\n");
                    func_0804adf0(-1);
                }
                if (t->unk_110.func_0804b234(name))
                    return true;
            }
        }
        if (s->getBranch()) {
            char *tgt = s->getBranchTargetImmediate();
            if (tgt && func_0804c03c(tgt, name))
                return true;
        }
    }
    return p->unk_88.func_0804b234(name) != 0;
}

bool func_0806f4a0(Program *p, instStream *s)
{
    mList *l = &s->unk_110;
    for (mNode *n = l->unk_0c; n; n = n->unk_04) {
        if (p->proc.unk_04.func_0804b234(n->unk_14))
            return true;
    }
    return false;
}

bool func_0806f4e8(Program *p, char *name)
{
    return p->proc.unk_04.func_0804b234(name) != 0;
}

bool recalcFlowUIDs(Program *p)
{
    instStream *s = p->unk_a0;
    int n = 0;
    p->unk_ac = -1;
    for (; s; s = s->unk_108) {
        if (!s->func_0806d908()) {
            func_0804ad38("recalcFlowUIDs - found bad code mode at %s\n", s->func_0806d674());
            return false;
        }
        s->unk_2c0 = n++;
    }
    if (n != p->unk_a8) {
        func_0804ad38("recalcFlowUIDs - there was a hole somewhere?\n");
        return false;
    }
    p->unk_ac = n;
    int size = p->unk_ac + 1;
    if (p->unk_b0) {
        delete p->unk_b0;
        p->unk_b0 = 0;
    }
    p->unk_b0 = new instStream *[size];
    if (!p->unk_b0) {
        func_0804ad38("failed to get a new linear table\n");
        return false;
    }
    for (s = p->unk_a0; s; s = s->unk_108)
        p->unk_b0[s->unk_2c0] = s;
    return true;
}

bool func_0806f610(Program *p)
{
    if (!p->proc.func_0806bd40()) {
        func_0804ad38("failed to alloc reset the cpu\n");
        return false;
    }
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (!wt) {
            func_0804ad38("failed to get work table\n");
            return false;
        }
        wt->unk_08 = 1;
    }
    return true;
}

bool func_0806f6a4(Program *p)
{
    if (!p->proc.func_0806bd88()) {
        func_0804ad38("failed to alloc reset the cpu\n");
        return false;
    }
    return true;
}

static inline void printLink(const char *fmt, linkNode *l)
{
    const char *name = l->unk_0c->func_0806d674();
    int other = l->unk_0c->unk_2c0;
    const char *type = linkTypeName(l->unk_14);
    func_0804acb0(fmt, type, other, name);
}

static inline bool isEntryStream(instStream *s) { return s->unk_2cc == 2; }
static inline bool isExitStream(instStream *s) { return s->unk_2cd == 3; }

void dumpFlow(Program *p)
{
    instStream *s = p->unk_a0;
    func_0804acb0("dumpFlow-------------------\n");
    for (; s; s = s->unk_108) {
        const char *tag = "";
        if (isEntryStream(s))
            tag = " ---ENTRY";
        if (isExitStream(s))
            tag = " ---EXIT";
        bool fallThrough = false;
        int id = s->unk_2c0;
        func_0804acb0("[%5d] %s %s   <--\n", id, s->func_0806d674(), tag);
        linkNode *l;
        for (l = s->unk_128.unk_00; l; l = l->unk_08) {
            if (l->unk_14) {
                printLink("       <--[(%s) [%5d]%s]\n", l);
            }
        }
        for (l = s->unk_134.unk_00; l; l = l->unk_08) {
            if (l->unk_14) {
                printLink("     --> %s [%5d]%s\n", l);
            } else
                fallThrough = true;
        }
        if (fallThrough) {
            func_0804acb0("    |\n");
            func_0804acb0("    V\n");
        } else
            func_0804acb0("\n");
    }
    func_0804acb0("dumpFlow - done\n");
}
