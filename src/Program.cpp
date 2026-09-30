#include "Program.h"
#include "Processor.h"
#include "instStream.h"
#include "os.h"
#include "flowBlock.h"

void disposeBackTrace(Program *p);
void func_0806be68(Program *p);
bool parseReset(Program *p);
const char *linkTypeName(int type);

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
    bool ok = func_08085aa8(this, blocks, count, newSyntax);
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
