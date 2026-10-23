#include "iInfo_shared.h"

#include "Instr.h"
#include "Block.h"
#include "os.h"


Instr *makeMove(Block *blk, RegPoolEntry *reg)
{
    int type = 0;
    Instr *mv = 0;
    if (blk->unk_b4 + blk->unk_b8 + blk->unk_b0 >= 2 * blk->unk_38)
        return 0;
    {
        switch (reg->unk_24) {
        case 8:
            type = 1;
            if (blk->unk_b8 >= blk->unk_38) {
                blk->unk_ad = 1;
                return mv;
            }
            break;
        case 9:
            type = 2;
            break;
        default:
            func_0804ad38("can't make move for reg %s\n", reg->func_0805a8e0());
            func_0804aeb0();
        }
        mv = getMove(type, 0);
        if (mv == 0) {
            func_0804ad38("failed to get a free move\n");
            func_0804aeb0();
        }
        Operand *in = mv->getOp(1);
        if (!in) {
            func_0804ad38("failed to get move input!\n");
            func_0804adf0(1);
        }
        Operand *out = mv->getOp(0);
        if (!out) {
            func_0804ad38("failed to get move output!\n");
            func_0804adf0(1);
        }
        RegPoolEntry *dup = out->unk_18;
        if (!dup) {
            func_0804ad38("failed to dup  a reg\n");
            func_0804aeb0();
        }
        mv->unk_141a = blk->unk_38;
        in->unk_18 = reg;
        in->unk_2b = 1;
        in->unk_10 = 0;
        out->unk_2b = 1;
        out->unk_10 = 0;
        mv->unk_13d1 = 1;
        mv->unk_138e = 0;
    }
    return mv;
}

extern const char getMoveParentUninsertedError[] = "parent not inserted during move\n";
extern const char getMoveNegativeDistanceError[] = "dff was <0\n";
extern const char getMoveNoRegisterError[] = "getMove: didn't set rg_old during pass\n";
extern const char getMoveNoParentError[] = "getMove: failed to get parent for something I wanted to move!\n";
extern const char getMoveOldStateError[] = "cant age input - but the old register status is now invalid\n";
extern const char getMoveSpreadStateError[] = "neither old new bits are valid - valid register state is spread over too many different registers\n";
extern const char getMoveInputOwnerError[] = "getMove : failed to set owner for input\n";
extern const char getMoveOutputOwnerError[] = "getMove :failed to set owner for output\n";
extern const char getMoveInputRegisterError[] = "failed to register input from move instruction to old output\n";
extern const char getMoveOutputLockError[] = "getMove : failed to register output lock for move\n";
extern const char getMoveInputDeregisterError[] = "failed to deregister an input during move create\n";
extern const char getMoveOutputInputError[] = "failed to register input to a moves output\n";
extern const char getMoveCandidateError[] = "hmm candidate for moving wasn't one after all\n";
extern const char getMoveNewInputError[] = "failed to register an input during move create\n";
extern const char getMoveRetargetDeregisterError[] = "retargetOutputOperands: failed to deregister output\n";
extern const char getMoveRetargetReregisterError[] = "retargetOutputOperands : failed to reregister output\n";
extern const char getMoveTodoHeadError[] = "getMove : failed to add move to the head of the to todo list\n";

// Shared diagnostics preserve the original aligned message pool across the split.
extern const char undoMoveInputLockError[] = "undoMove : failed to register input lock\n";
extern const char undoMoveParentError[] = "undoMove: failed to get parent for old lock!\n";
extern const char undoMoveListError[] = "failed to get lock list for old reg in move undo\n";
extern const char undoMoveOutputRegisterError[] = "undo move - failed to register output\n";
extern const char undoMoveOutputDeregisterError[] = "failed to deregister output lock for undo move\n";
extern const char undoMoveInputRegisterError[] = "undo move - failed to register input\n";
extern const char undoMoveInputDeregisterError[] = "failed to deregister input lock for undo move\n";
extern const char undoMoveUnlinkError[] = "failed to unlink from todolist!\n";
