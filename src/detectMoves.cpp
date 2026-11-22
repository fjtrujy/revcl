#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "moveRec.h"

bool resolveMovePipe(workTable *wt, Instr *mv, int pipe);

static inline bool isRawTable(workTable *wt)
{
    return wt->unk_09 == 3;
}

static inline bool isPipeMode(workTable *wt)
{
    return isRawTable(wt) && (wt->unk_0a == 1 || wt->unk_0a == 3);
}

static inline void dumpMove(Instr *in)
{
    in->func_08054008(false);
}

bool detectMoves(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
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
                if (!detectMove(in, opc, mask, dstOp, srcOp))
                    continue;
                RegPoolEntry *srcReg = in->operands[srcOp].unk_18;
                RegPoolEntry *dstReg = in->operands[dstOp].unk_18;
                iInfo *info = func_0806bf74(opc);
                Instr *mv = new Instr(info);
                if (!mv) {
                    reportError("failed to make a move\n");
                    return false;
                }
                mv->unk_142c = opc;
                mv->unk_142e = 0;
                Operand *sr = &mv->operands[1];
                Operand *d = &mv->operands[0];
                if (dstReg == dstReg->ownerPool->zeroRegister) {
                    reportError("found a move to a const zero register\n");
                    dumpMove(in);
                    return false;
                }
                sr->unk_18 = srcReg;
                d->unk_18 = dstReg;
                mv->unk_138f = mask;
                sr->unk_07 = mask;
                d->unk_07 = mask;
                sr->unk_14 = in->operands[srcOp].unk_14;
                d->unk_14 = in->operands[dstOp].unk_14;
                sr->unk_10 = in->operands[srcOp].unk_10;
                d->unk_10 = in->operands[dstOp].unk_10;
                if (isRawTable(wt)) {
                    int pipe = in->unk_13e8.unk_02;
                    if (!resolveMovePipe(wt, mv, pipe)) {
                        reportError("failed to resolve a detected move for the pipe I found it in\n");
                        return false;
                    }
                }
                if (!wt->replaceInstruction(in, mv)) {
                    reportError("failed to replace instruction!\n");
                    return false;
                }
                delete in;
            }
        }
    }
    return true;
}
