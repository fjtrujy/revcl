#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "os.h"

int expandRaw(workTable *wt)
{
    if (wt->unk_09 != 5) {
        func_0804ad38("expand raw - table not raw!\n");
        return 0;
    }
    return wt->changeMode(3);
}

bool expandRawTables(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (!s->unk_218) {
            func_0804ad38("work table not set in %s\n", s->func_0806d674());
            func_0804adf0(-1);
        }
        workTable *wt = s->unk_218;
        if (wt->codeMode() == 3) {
            func_0804ad38("hit a raw table during expraw\n");
            return false;
        }
        if (wt->codeMode() == 5 && !wt->changeMode(3)) {
            func_0804ad38("processor failed to expand raw table\n");
            return false;
        }
    }
    return true;
}

extern const char emuNullTableWarn[] = "emu - passed a null table ptr!\n";
