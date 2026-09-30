#include "Program.h"
#include "Processor.h"
#include "instStream.h"
#include "os.h"

void disposeBackTrace(Program *p);

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
