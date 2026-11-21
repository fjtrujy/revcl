#include <stdlib.h>
#include "iGroupLog.h"
#include "Instr.h"
#include "workTable.h"
#include "RegPool.h"
#include "ol_s.h"

int func_0804acb0(const char *fmt, ...);
int func_0804ad38(const char *fmt, ...);

Instr **iGroupLog::table;
int iGroupLog::count;
int iGroupLog::size;

iGroupLog::iGroupLog()
{
    table = 0;
    count = 0;
    size = 0;
}

iGroupLog::~iGroupLog()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
}

int iGroupLog::init()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
    return 1;
}

int iGroupLog::reset()
{
    if (table)
        delete[] table;
    count = 0;
    size = 0;
    return 1;
}

int iGroupLog::add(Instr *ins)
{
    if (!ins) {
        func_0804ad38("iGroup - add instruction - no instruction to add!\n");
        return 0;
    }
    if (count > size) {
        func_0804ad38("overflow for the iGroup log\n");
        return 0;
    }
    if (count == size) {
        size = count * 2 + 0x2000;
        int bytes = size * sizeof(Instr *);
        table = (Instr **)realloc(table, bytes);
        if (!table) {
            func_0804ad38("iGroup - failed to realloc table to %d\n", bytes);
            return 0;
        }
    }
    table[count] = ins;
    return count++;
}

int iGroupLog::logTable(workTable *wt)
{
    if (table) {
        func_0804ad38("logTable - called - but the table is already open\n");
        return 0;
    }
    for (regUse *p = wt->unk_14.unk_04; p; p = p->unk_04) {
        RegPoolEntry *reg = p->unk_08;
        for (int i = 0; i != reg->unk_28; i++) {
            if (((p->unk_0c & p->unk_0d) >> i) & 1) {
            Instr *in;
            for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
                in->unk_0e = 0;
            for (Instr *cur = (in = wt->unk_88.unk_00); in; cur = (in = in->unk_1c.unk_00))
                for (int j = 0; j != cur->unk_138b; j++) {
                    Operand *op = &cur->unk_38[j];
                    RegPoolEntry *r = op->unk_18;
                    if (r == reg && op->unk_27)
                        for (int k = 0; k != r->unk_28; k++) {
                            if (isRegisterOperand(op, 2) && ((op->unk_27 >> k) & 1))
                                in->func_08055aa8();
                            if (isRegisterOperand(op, 1) && ((op->unk_27 >> k) & 1))
                                in->func_08055a88();
                        }
                }
            bool changed;
            do {
                changed = false;
                for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
                    if (in->unk_0e_1)
                        for (int j = 0; j != in->unk_138b; j++) {
                            Operand *op = &in->unk_38[j];
                            RegPoolEntry *r = registerOperand(op, 2);
                            if (r)
                                for (int k = 0; k != r->unk_28; k++)
                                    for (ol_s *o = op->unk_2c[k].unk_54.func_08058370(); o; o = o->func_080583ac()) {
                                        Instr *t = o->unk_04;
                                        changed = changed || t->func_08055a88();
                                    }
                        }
                wtInstrList *list = &wt->unk_88;
                for (in = list->unk_04; in; in = in->unk_1c.unk_04)
                    if (in->unk_0e_0)
                        for (int j = 0; j != in->unk_138b; j++) {
                            Operand *op = &in->unk_38[j];
                            RegPoolEntry *r = registerOperand(op, 1);
                            if (r)
                                for (int k = 0; k != r->unk_28; k++)
                                    for (ol_s *o = op->unk_2c[k].unk_54.func_08058370(); o; o = o->func_080583ac())
                                        o->unk_04->unk_0e_0 = 1;
                        }
            } while (changed);
            func_0804acb0("  dumping scan for %s\n", reg->maskName(1 << i));
            int n = 0;
            for (in = wt->unk_88.unk_00; in; in = in->unk_1c.unk_00)
                if (in->unk_0e == 3) {
                    const char *name = in->unk_34 ? in->unk_34->unk_04 : "NO_BASE_INST";
                    func_0804acb0("  [%4d] %d %s\n", n++, in->unk_0c, name);
                }
            func_0804acb0("  ..%d matches\n", n);
            }
        }
    }
    return 1;
}
