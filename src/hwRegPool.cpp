#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "hwRegPool.h"

RegPool *getRegisterPoolByIndex(unsigned t);

void dumpThroughputRegisters(Program *program)
{
    reportInfo("dump through registers ==============================================\n");
    for (instStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        int poolIndex;
        RegPool *registerPool;
        workTable *scheduleTable = stream->getTable();
        reportInfo("%s:\n", stream->streamName());
        for (poolIndex = 0; poolIndex != 12; poolIndex++) {
            registerPool = getRegisterPoolByIndex(poolIndex);
            if (!registerPool || !registerPool->virtualPartCount)
                continue;
            reportInfo("    %s\n", registerPool->poolName);
            for (regUse *registerUse = scheduleTable->registerUses.head; registerUse; registerUse = registerUse->next) {
                int mask = registerUse->throughputMask & ~(registerUse->inputMask | registerUse->outputMask | registerUse->externalInputMask | registerUse->externalOutputMask);
                RegPoolEntry *registerEntry = registerUse->registerEntry;
                if (mask && registerPool->ownsRegisterClass(registerEntry))
                    reportInfo("        %s\n", registerEntry->maskName(mask));
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
