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
    return wt->codeModeValue == 3;
}

static inline bool isPipeMode(workTable *wt)
{
    return isRawTable(wt) && (wt->rawSchedulingMode == 1 || wt->rawSchedulingMode == 3);
}

static inline void dumpMove(Instr *in)
{
    in->dumpOperandDiagnostics(false);
}

bool detectMoves(Program *p)
{
    for (instStream *s = p->streamHead; s; s = s->nextStream) {
        workTable *wt = s->getTable();
        if (wt->codeModeValue == 1 || isPipeMode(wt)) {
            Instr *next;
            for (Instr *in = wt->linearInstructions.head; in; in = next) {
                next = in->linearLink.nextInstruction;
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
                RegPoolEntry *srcReg = in->operands[srcOp].assignedRegister;
                RegPoolEntry *dstReg = in->operands[dstOp].assignedRegister;
                iInfo *info = getOpcodeInfoById(opc);
                Instr *mv = new Instr(info);
                if (!mv) {
                    reportError("failed to make a move\n");
                    return false;
                }
                mv->abstractMoveOpcode = opc;
                mv->concreteMoveOpcode = 0;
                Operand *sr = &mv->operands[1];
                Operand *d = &mv->operands[0];
                if (dstReg == dstReg->ownerPool->zeroRegister) {
                    reportError("found a move to a const zero register\n");
                    dumpMove(in);
                    return false;
                }
                sr->assignedRegister = srcReg;
                d->assignedRegister = dstReg;
                mv->destinationMask = mask;
                sr->componentMask = mask;
                d->componentMask = mask;
                sr->originalRegister = in->operands[srcOp].originalRegister;
                d->originalRegister = in->operands[dstOp].originalRegister;
                sr->forcedBaseRegister = in->operands[srcOp].forcedBaseRegister;
                d->forcedBaseRegister = in->operands[dstOp].forcedBaseRegister;
                if (isRawTable(wt)) {
                    int pipe = in->position.pipelineIndex;
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
