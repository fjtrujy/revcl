#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "hwRegPool.h"

RegPool *GetRP(unsigned t);

void dumpThru(Program *prog)
{
    reportInfo("dump through registers ==============================================\n");
    for (instStream *stream = prog->streamHead; stream; stream = stream->nextStream) {
        int i;
        RegPool *registerPool;
        workTable *scheduleTable = stream->getTable();
        reportInfo("%s:\n", stream->streamName());
        for (i = 0; i != 12; i++) {
            registerPool = GetRP(i);
            if (!registerPool || !registerPool->virtualPartCount)
                continue;
            reportInfo("    %s\n", registerPool->poolName);
            for (regUse *registerUse = scheduleTable->registerUses.head; registerUse; registerUse = registerUse->next) {
                int mask = registerUse->throughputMask & ~(registerUse->inputMask | registerUse->outputMask | registerUse->externalInputMask | registerUse->externalOutputMask);
                RegPoolEntry *reg = registerUse->registerEntry;
                if (mask && registerPool->ownsRegisterClass(reg))
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
