#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"

bool detectMove(Instr *in, int *opc, unsigned char *mask, int *dstOp, int *srcOp);
bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);

static inline bool isPipeMode(workTable *wt)
{
    return wt->unk_09 == 3 && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

bool detectMoves(Program *p)
{
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        workTable *wt = s->getTable();
        if (wt->unk_09 == 1 || isPipeMode(wt)) {
            Instr *next;
            for (Instr *in = wt->unk_88.unk_00; in; in = next) {
                next = in->unk_1c.unk_00;
                int opc;
                unsigned char mask;
                int dstOp;
                int srcOp;
                opc = -1;
                srcOp = -1;
                dstOp = -1;
                mask = 0;
                if (!detectMove(in, &opc, &mask, &dstOp, &srcOp))
                    continue;
                RegPoolEntry *srcReg = in->unk_38[srcOp].unk_18;
                RegPoolEntry *dstReg = in->unk_38[dstOp].unk_18;
                iInfo *info = func_0806bf74(opc);
                Instr *mv = new Instr(info);
                if (!mv) {
                    func_0804ad38("failed to make a move\n");
                    return false;
                }
                mv->unk_142c = opc;
                mv->unk_142e = 0;
                Operand *d = &mv->unk_38[0];
                Operand *sr = &mv->unk_38[1];
                if (dstReg == dstReg->unk_04->unk_10) {
                    func_0804ad38("found a move to a const zero register\n");
                    in->func_08054008(false);
                    return false;
                }
                sr->unk_18 = srcReg;
                d->unk_18 = dstReg;
                mv->unk_138f = mask;
                sr->unk_07 = mask;
                d->unk_07 = mask;
                sr->unk_14 = in->unk_38[srcOp].unk_14;
                d->unk_14 = in->unk_38[dstOp].unk_14;
                sr->unk_10 = in->unk_38[srcOp].unk_10;
                d->unk_10 = in->unk_38[dstOp].unk_10;
                if (wt->unk_09 == 3) {
                    if (!resolveMovePipe(wt, mv, in->unk_13e8.unk_02)) {
                        func_0804ad38("failed to resolve a detected move for the pipe I found it in\n");
                        return false;
                    }
                }
                if (!wt->replaceInstruction(in, mv)) {
                    func_0804ad38("failed to replace instruction!\n");
                    return false;
                }
                delete in;
            }
        }
    }
    return true;
}
