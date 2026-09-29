#include "Instr.h"
#include "os.h"

bool cls0805bcc4::setOwner(int part, Instr *instr, int opnd)
{
    if (!instr) {
        func_0804ad38("setowner - passed null\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_24) {
        func_0804ad38("owner already set for rlock set owner\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_2d) {
        func_0804ad38("owner: type wasn't null when try to set\n");
        return false;
    }
    if (unk_04) {
        func_0804ad38("setowner - lock was already on list\n");
        func_0804adf0(-1);
        return false;
    }
    unk_2d = 1;
    unk_28 = part;
    unk_29 = opnd;
    unk_24 = instr;
    return true;
}

bool cls0805bcc4::setOwner(int part, regUse *rf)
{
    if (!rf) {
        func_0804ad38("setowner - passed null rf\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_24) {
        func_0804ad38("owner already set for rlock set owner\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_2d) {
        func_0804ad38("owner: type wasn't null when try to set\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_04) {
        func_0804ad38("setowner - lock was already on list\n");
        func_0804adf0(-1);
        return false;
    }
    unk_29 = 0;
    unk_28 = part;
    unk_2a = 0;
    unk_2d = 2;
    unk_24 = rf;
    unk_32 = -1;
    unk_30 = -1;
    return true;
}

bool cls0805bcc4::clearOwner()
{
    unk_24 = 0;
    unk_2d = 0;
    unk_14 = 0;
    unk_04 = 0;
    unk_28 = -1;
    unk_29 = -1;
    return true;
}

Instr *cls0805bcc4::ownerInstr()
{
    if (unk_2d == 1)
        return (Instr *)unk_24;
    return 0;
}

regUse *cls0805bcc4::ownerRegUse()
{
    if (unk_2d == 2)
        return (regUse *)unk_24;
    return 0;
}

Operand *cls0805bcc4::ownerOperand()
{
    if (unk_2d == 1) {
        int opnd = unk_29;
        Instr *instr = (Instr *)unk_24;
        return &instr->unk_38[opnd];
    }
    return 0;
}

int cls0805bcc4::ownerOpnd()
{
    if (unk_2d == 1)
        return unk_29;
    return -1;
}

int cls0805bcc4::getPart()
{
    if (unk_2d == 1 || unk_2d == 2)
        return unk_28;
    func_0804ad38("failed to get part - lock incorrect oType\n");
    func_0804adf0(-1);
    return -1;
}
