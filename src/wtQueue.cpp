#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

bool workTable::qRegRef(regUse *ru)
{
    if (!ru) {
        func_0804ad38("qRegRef: passed a null pointer\n");
        func_0804adf0(-1);
    }
    regUse *c = ru->Clone();
    if (!c) {
        func_0804ad38("failed to clone regref\n");
        func_0804adf0(-1);
    }
    RegPoolEntry *reg = c->unk_08;
    if (!reg) {
        func_0804ad38("no register attached to cloned rf during qRegRef\n");
        delete c;
        return false;
    }
    if (!unk_14.append(c)) {
        func_0804ad38("failed to add cloned regref to list\n");
        delete c;
        return false;
    }
    if (~c->unk_0f & c->unk_0d) {
        func_0804ad38("xobits set, but o bits not\n");
        c->func_0805b51c();
        return false;
    }
    if (~c->unk_0e & c->unk_0c) {
        func_0804ad38("xibits set, but i bits not\n");
        c->func_0805b51c();
        return false;
    }
    for (int i = 0; i != reg->unk_28; i++) {
        c->unk_20[i].unk_30 = ru->unk_20[i].unk_30;
        c->unk_20[i].unk_32 = ru->unk_20[i].unk_32;
        cls0805c244 *ll = getLockList(reg, i);
        if (!ll) {
            func_0804ad38("failed to get lock list ptr in register for rfv for reg %s\n", reg->func_0805a8e0());
            func_0804adf0(-1);
        }
        if (c->unk_20[i].unk_58) {
            func_0804ad38("rll is already set in rrp\n");
            func_0804adf0(-1);
        }
        c->unk_20[i].unk_58 = (int)ll;
        if (reg->func_0805a908(i)) {
            func_0804ad38("regref register part already had locklist\n");
            func_0804adf0(-1);
        }
        if (!reg->func_0805a924(i, (int)ll)) {
            func_0804ad38("failed to set curr lock list pointer in rf register\n");
            func_0804adf0(-1);
        }
        if ((c->unk_0f >> i) & 1)
            ll->unk_29 = 0;
        char xi = (c->unk_0c >> i) & 1;
        char t = (c->unk_10 >> i) & 1;
        bool sync = false;
        if (reg->unk_2c && g_080b336c == 0)
            sync = true;
        char nb = reg->unk_1c;
        if (!nb && (c->unk_10 & (c->unk_0f | c->unk_0d))) {
            func_0804ad38("t bits set for nbid register\n");
            c->func_0805b51c();
            return false;
        }
        if (t || xi || sync) {
            unsigned char nbz = nb == 0;
            cls0805bcc4 *lk = c->unk_20[i].func_0805b830();
            if (!lk)
                func_0804ad38("failed to get lock for %s.%d as %s\n",
                              reg->func_0805a8e0(), i, reg->func_0805a8e0());
            if (!lk->setOwner(i, c)) {
                func_0804ad38("falied to set owner\n");
                func_0804adf0(-1);
                return false;
            }
            if (!ll->addOutput(lk, 0, 1, nbz)) {
                func_0804ad38("failed to register output lock for regref\n");
                func_0804adf0(-1);
                return false;
            }
            if (t) {
                bool ext = !(((c->unk_0f | c->unk_0e) >> i) & 1);
                if (!ll->addExtOutput(lk, ext)) {
                    func_0804ad38("failed to add pass thru reg ref as ext output for register %s\n",
                                  reg->func_0805a8e0());
                    return false;
                }
                dump();
            }
            if (!ll->addOutputLock(lk)) {
                func_0804ad38("regRef::LoadLocks - failed to add output lock\n");
                return false;
            }
        }
    }
    return true;
}
