#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "hwRegPool.h"

RegPool *getRegisterPoolByIndex(unsigned t);

void dumpThroughputRegisters(Program *program)
{
    reportInfo("dump through registers ==============================================\n");
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        int poolIndex;
        RegPool *registerPool;
        ScheduleTable *scheduleTable = stream->getTable();
        reportInfo("%s:\n", stream->streamName());
        for (poolIndex = 0; poolIndex != 12; poolIndex++) {
            registerPool = getRegisterPoolByIndex(poolIndex);
            if (!registerPool || !registerPool->virtualPartCount)
                continue;
            reportInfo("    %s\n", registerPool->poolName);
            for (RegisterUse *registerUse = scheduleTable->registerUses.head; registerUse; registerUse = registerUse->next) {
                int mask = registerUse->throughputMask & ~(registerUse->inputMask | registerUse->outputMask | registerUse->externalInputMask | registerUse->externalOutputMask);
                RegisterEntry *registerEntry = registerUse->registerEntry;
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

bool hwRegPool::assignRegistersForWorkTable(ScheduleTable * unusedTable)
{
    return true;
}

bool hwRegPool::assignExt(InstructionStream * unusedStream)
{
    return true;
}

bool hwRegPool::discardDisjointExternalRegister(InstructionStream *, RegisterEntry * unusedRegister)
{
    return true;
}

extern const char memRegPoolAllocError[] = "RegPool::Init - couldn't allocate table\n";
