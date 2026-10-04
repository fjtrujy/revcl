#include <string.h>
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "os.h"

bool UnlinkFromLinear(Block *blk, Instr *n);
bool func_0806f4a0(Program *p, instStream *s);
bool appendStream(Program *p, instStream *a, instStream *b);

static inline void dumpRawStream(instStream *s)
{
    s->dump(0);
}

bool func_08063154(Program *p)
{
    instStream *s = p->unk_a0;
    while (s) {
        if (s->func_0806d908() != 4 && !s->disposeInputTable()) {
            func_0804ad38("failed to dispose input table\n");
            return 0;
        }
        if (s->func_0806d908() == 3 && !s->func_0806d968()) {
            func_0804ad38("bad raw mode\n");
            dumpRawStream(s);
            return 0;
        }
        instStream *n = s->unk_108;
        if (n && !n->unk_2d6 && !s->unk_2d7) {
        linkList *out = &s->unk_134;
        if (out->unk_08 == 1 && n->unk_128.unk_08 == 1
            && s->func_0806d9a0() && n->func_0806d9a0()
            && s->unk_2cd == 1 && n->unk_2cc == 1
            && !s->getBranch()
            && out->getStream(0) == n
            && !n->unk_2d4 && !s->unk_2d5
            && !func_0806f4a0(p, n)) {
            if (!appendStream(p, s, n)) {
                func_0804ad38("failed to append %s onto %s\n", n->func_0806d674(), s->func_0806d674());
                return 0;
            }
            if (!p->unlinkStreams(s, n, 0)) {
                func_0804ad38("failed to unlink streams\n");
                return 0;
            }
            if (out->unk_08) {
                func_0804ad38("there's still out ist's after unlink\n");
                return 0;
            }
            linkNode *l;
            while ((l = n->unk_134.unk_00) != 0) {
                instStream *to = l->unk_0c;
                int type = l->unk_14;
                if (!p->unlinkStreams(n, to, type)) {
                    func_0804ad38("failed to unlink streams\n");
                    return 0;
                }
                if (!p->linkStreams(s, to, type)) {
                    func_0804ad38("failed to link together streams\n");
                    return 0;
                }
            }
            if (!p->removeStream(n)) {
                func_0804ad38("failed to unlink ist_n\n");
                return 0;
            }
            delete n;
            continue;
        }
        }
        s = s->unk_108;
    }
    return 1;
}

// workTable is passed where Block / cls_unk30 are expected (same class, not yet unified)
bool catOnTo(workTable *t1, workTable *t2)
{
    if (!t1 || !t2) {
        func_0804ad38("catOnTo - passed a null pointer\n");
        return 0;
    }
    if (!t1->unk_2c || !t2->unk_2c) {
        func_0804ad38("it_1 or it_2 one of them has no table\n");
        return 0;
    }
    int end = t1->func_080741c0();
    if (!t1->grow(end + t2->getPhysicalLen() + 8)) {
        func_0804ad38("failed to grow table1\n");
        return 0;
    }
    int dst = t1->unk_34;
    int i;
    for (i = t2->func_0807411c(); i < t2->func_080741c0(); dst++, i++) {
        wtRow *src = &t2->unk_2c[i];
        memcpy(&t1->unk_2c[dst], src, 14);
        for (int j = 0; j != 2; j++) {
            Instr *in = src->unk_00[j].unk_00;
            if (in) {
                if (!UnlinkFromLinear((Block *)t2, in)) {
                    func_0804ad38("failed to remlinear\n");
                    return 0;
                }
                in->unk_30 = 0;
                in->setOwner((cls_unk30 *)t1);
            }
            src->unk_00[j].unk_00 = 0;
        }
    }
    for (i = t1->unk_30; i < t1->unk_34; i++) {
        wtRow *r = &t1->unk_2c[i];
        for (int j = 0; j != 2; j++) {
            Instr *in = r->unk_00[j].unk_00;
            if (in && !UnlinkFromLinear((Block *)t1, in)) {
                func_0804ad38("failed to link in from it1\n");
                return 0;
            }
        }
    }
    t1->unk_34 = dst;
    t2->unk_34 = 0;
    t2->unk_30 = 0;
    if (!t1->prepRawTable()) {
        func_0804ad38("failed to prep it_1\n");
        return 0;
    }
    if (!t2->prepRawTable()) {
        func_0804ad38("failed to prep it_2 after removing insts\n");
        return 0;
    }
    return 1;
}

bool appendStream(Program *p, instStream *a, instStream *b)
{
    if (!a || !b) {
        func_0804ad38("cat streams - passed a null pointer\n");
        return 0;
    }
    if (b->unk_2d4) {
        func_0804ad38("dbgTop set for ist_2 ->>>>>>>>>>>>>>please code this!\n");
        return 0;
    }
    if (a->unk_2d5) {
        func_0804ad38("dbgEnd set for ist_1 ->>>>>>>>>>>>>>please code this!\n");
        return 0;
    }
    workTable *t1 = a->getWorkTable();
    workTable *t2 = b->getWorkTable();
    if (!catOnTo(t1, t2)) {
        func_0804ad38("failed to cat table2 onto table1\n");
        return 0;
    }
    a->unk_2cd = b->unk_2cd;
    a->unk_2ce = b->unk_2ce;
    a->unk_2d5 = b->unk_2d5;
    a->unk_2d7 = b->unk_2d7;
    return 1;
}
