#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

extern RegPoolEntry *g_080de6ac;
extern mValue *g_080de6b0;
extern int g_080de6b4;
int func_0806aa54(Instr *in);
int func_08067e18(Program *p, instStream *s);


bool explodeRaw(Program *p)
{
    instStream *next;
    for (instStream *s = p->unk_a0; s; s = next) {
        next = s->unk_108;
        workTable *t = s->getTable();
        if (!t) {
            func_0804ad38("floBuild - there's no work table!\n");
            return false;
        }
        if (!t->func_080741b4())
            continue;
        if (t->unk_09 != 3) {
            func_0804ad38("explodeRaw: a non raw stream has a loop size set\n");
            return false;
        }
        if (s->unk_270 <= 0) {
            func_0804ac7c("setting known min count to 1 for %s\n", s->func_0806d674());
            s->unk_270 = 1;
        }
        if (s->unk_274 < 0) {
            func_0804ac7c("setting minimum safe count to 1000 for %s\n", s->func_0806d674());
            s->unk_274 = 10000;
        }
        int r = func_08067e18(p, s);
        if (r < 0) {
            func_0804ad38("failed to construct epilogue/prologues\n");
            return false;
        }
        if (r == 0) {
            func_0804ad38("no new streams added during build?\n");
            return false;
        }
        delete s;
    }
    return true;
}
