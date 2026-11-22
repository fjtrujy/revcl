#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "hwRegPool.h"

RegPool *GetRP(unsigned t);

void dumpThru(Program *prog)
{
    reportInfo("dump through registers ==============================================\n");
    for (instStream *s = prog->streamHead; s; s = s->unk_108) {
        int i;
        RegPool *rp;
        workTable *t = s->getTable();
        reportInfo("%s:\n", s->streamName());
        for (i = 0; i != 12; i++) {
            rp = GetRP(i);
            if (!rp || !rp->unk_18)
                continue;
            reportInfo("    %s\n", rp->unk_2c);
            for (regUse *u = t->unk_14.unk_04; u; u = u->unk_04) {
                int mask = u->unk_10 & ~(u->unk_0e | u->unk_0f | u->unk_0c | u->unk_0d);
                RegPoolEntry *reg = u->unk_08;
                if (mask && rp->func_08059e20(reg))
                    reportInfo("        %s\n", reg->maskName(mask));
            }
        }
    }
    reportInfo("dump_thru_end ==============================================\n");
}

hwRegPool::hwRegPool()
{
    unk_0e = 3;
}

hwRegPool::~hwRegPool()
{
}

bool hwRegPool::func_08088e54(workTable *)
{
    return true;
}

bool hwRegPool::assignExt(instStream *)
{
    return true;
}

bool hwRegPool::func_08088e6c(instStream *, RegPoolEntry *)
{
    return true;
}

extern const char memRegPoolAllocError[] = "RegPool::Init - couldn't allocate table\n";
