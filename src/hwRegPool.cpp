#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "hwRegPool.h"

RegPool *GetRP(unsigned t);

void dumpThru(Program *prog)
{
    reportInfo("dump through registers ==============================================\n");
    for (instStream *s = prog->streamHead; s; s = s->nextStream) {
        int i;
        RegPool *rp;
        workTable *t = s->getTable();
        reportInfo("%s:\n", s->streamName());
        for (i = 0; i != 12; i++) {
            rp = GetRP(i);
            if (!rp || !rp->virtualPartCount)
                continue;
            reportInfo("    %s\n", rp->poolName);
            for (regUse *u = t->registerUses.head; u; u = u->next) {
                int mask = u->throughputMask & ~(u->inputMask | u->outputMask | u->externalInputMask | u->externalOutputMask);
                RegPoolEntry *reg = u->registerEntry;
                if (mask && rp->ownsRegisterClass(reg))
                    reportInfo("        %s\n", reg->maskName(mask));
            }
        }
    }
    reportInfo("dump_thru_end ==============================================\n");
}

hwRegPool::hwRegPool()
{
    poolKind = 3;
}

hwRegPool::~hwRegPool()
{
}

bool hwRegPool::assignRegistersForWorkTable(workTable *)
{
    return true;
}

bool hwRegPool::assignExt(instStream *)
{
    return true;
}

bool hwRegPool::discardDisjointExternalRegister(instStream *, RegPoolEntry *)
{
    return true;
}

extern const char memRegPoolAllocError[] = "RegPool::Init - couldn't allocate table\n";
