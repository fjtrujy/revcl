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

void cls0805bcc4::printOwner()
{
    switch (unk_2d) {
    case 0:
        break;
    case 1: {
        Instr *instr = unk_2d == 1 ? (Instr *)unk_24 : 0;
        if (!instr) {
            func_0804ad38("type inst..  can't get it though!\n");
            func_0804adf0(-1);
        }
        instr->func_08054008(false);
        break;
    }
    case 2: {
        regUse *rf = unk_2d == 2 ? (regUse *)unk_24 : 0;
        if (!rf) {
            func_0804ad38("type regref..  can't get it though!\n");
            func_0804adf0(-1);
        }
        break;
    }
    default:
        func_0804ad38("??????????? unkown lock\n");
        func_0804adf0(-1);
    }
}

cls0805bcc4 *cls0805bcc4::getParent()
{
    if (unk_14 != 1) {
        func_0804ad38("getParent - called on non input lock\n");
        print();
        return 0;
    }
    if (!unk_0c) {
        func_0804ac7c("getParent: not set!\n");
        print();
        return 0;
    }
    if (unk_0c->unk_14 != 2) {
        func_0804ad38("parent was not output!\n");
        func_0804ad38("we are:\n");
        print();
        func_0804ad38("parent is\n");
        unk_0c->print();
        func_0804adf0(-1);
    }
    return unk_0c;
}

bool cls0805bcc4::setInOut(cls0805bcc4 *olk)
{
    if (!olk) {
        func_0804ad38("setinout - passed null\n");
        func_0804adf0(-1);
    }
    if (olk->unk_14 != 2) {
        func_0804ad38("setinout - olk wasn't output\n");
        func_0804adf0(-1);
    }
    if (!unk_24) {
        func_0804ad38("owner not set for inout\n");
        func_0804adf0(-1);
    }
    if (unk_24 != olk->unk_24) {
        func_0804ad38("setinout -- inst bad\n");
        func_0804adf0(-1);
    }
    if (unk_28 != olk->unk_28) {
        func_0804ad38("set in out - part mismatched\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_1c) {
        func_0804ad38("rlock - set inout - already set\n");
        func_0804adf0(-1);
        return false;
    }
    if (unk_14 != 1) {
        func_0804ad38("set inout on non input\n");
        print();
        func_0804adf0(-1);
        return false;
    }
    unk_1c = olk;
    return true;
}

bool cls0805bcc4::incRef()
{
    unk_22++;
    return true;
}

bool cls0805bcc4::decRef()
{
    unk_22--;
    return true;
}

void cls0805bcc4::print()
{
    switch (unk_14) {
    case 0:
    case 1:
        break;
    case 2:
        for (cls0805bcc4 *c = unk_0c; c; c = c->unk_08)
            c->print();
        break;
    }
    printOwner();
}

cls0805c244::cls0805c244()
{
    unk_04 = 0;
    unk_08 = 0;
    for (int i = 0; i != 4; i++)
        unk_0c[i] = 0;
    unk_2a = 0;
    unk_2b = 0;
    unk_1c = 0;
    unk_24 = 0;
    unk_26 = 0;
    unk_28 = 0;
    unk_29 = 1;
    unk_20 = 0;
    unk_00 = 0;
}
