#include "os.h"
#include "workTable.h"
#include "instStream.h"
#include "Program.h"

bool prepExtInfo(Program *p, instStream *s, bool b);
int func_0807aaf4(Program *p, instStream *s);

int func_0807aa28(Program *p)
{
    int total = 0;
    instStream *s;
    for (s = p->unk_a0; s; s = s->unk_108) {
        s->unk_2dc = 0;
        if (!s->func_0806d9a0())
            s->unk_2dc = 1;
    }
    for (s = p->unk_a0; s; s = s->unk_108) {
        if (s->unk_2dc)
            continue;
        if (!prepExtInfo(p, s, 0)) {
            func_0804ad38("failed to prep external info for stream %s\n", s->func_0806d674());
            return -1;
        }
        int n = func_0807aaf4(p, s);
        if (n < 0) {
            func_0804ad38("failed to sred a table from %s\n", s->func_0806d674());
            return -1;
        }
        s->unk_2dc = 1;
        total += n;
    }
    return total;
}
