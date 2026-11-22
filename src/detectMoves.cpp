#include "os.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "moveRec.h"

bool resolveMovePipe(ScheduleTable *wt, Instr *mv, int pipe);

static inline bool isRawTable(ScheduleTable *scheduleTable)
{
    return scheduleTable->codeModeValue == VCL_CODE_RAW;
}

static inline bool isPipeMode(ScheduleTable *scheduleTable)
{
    return isRawTable(scheduleTable) && (scheduleTable->rawSchedulingMode == VCL_RAW_COMPILER_FREE || scheduleTable->rawSchedulingMode == VCL_RAW_USER_FREE);
}

static inline void dumpMove(Instr *instruction)
{
    instruction->dumpOperandDiagnostics(false);
}

bool detectMoves(Program *program)
{
    for (InstructionStream *stream = program->streamHead; stream; stream = stream->nextStream) {
        ScheduleTable *scheduleTable = stream->getTable();
        if (scheduleTable->codeModeValue == VCL_CODE_UTA || isPipeMode(scheduleTable)) {
            Instr *next;
            for (Instr *currentInstruction = scheduleTable->linearInstructions.head; currentInstruction; currentInstruction = next) {
                next = currentInstruction->linearLink.nextInstruction;
                int opc;
                unsigned char mask;
                int dstOp;
                int srcOp;
                opc = -1;
                srcOp = -1;
                dstOp = -1;
                mask = 0;
                if (!detectMove(currentInstruction, opc, mask, dstOp, srcOp))
                    continue;
                RegisterEntry *srcReg = currentInstruction->operands[srcOp].assignedRegister;
                RegisterEntry *dstReg = currentInstruction->operands[dstOp].assignedRegister;
                OpcodeDescriptor *opcodeInfo = getOpcodeInfoById(opc);
                Instr *newMoveInstruction = new Instr(opcodeInfo);
                if (!newMoveInstruction) {
                    reportError("failed to make a move\n");
                    return false;
                }
                newMoveInstruction->abstractMoveOpcode = opc;
                newMoveInstruction->concreteMoveOpcode = 0;
                Operand *operand = &newMoveInstruction->operands[1];
                Operand *destinationOperand = &newMoveInstruction->operands[0];
                if (dstReg == dstReg->ownerPool->zeroRegister) {
                    reportError("found a move to a const zero register\n");
                    dumpMove(currentInstruction);
                    return false;
                }
                operand->assignedRegister = srcReg;
                destinationOperand->assignedRegister = dstReg;
                newMoveInstruction->destinationMask = mask;
                operand->componentMask = mask;
                destinationOperand->componentMask = mask;
                operand->originalRegister = currentInstruction->operands[srcOp].originalRegister;
                destinationOperand->originalRegister = currentInstruction->operands[dstOp].originalRegister;
                operand->forcedBaseRegister = currentInstruction->operands[srcOp].forcedBaseRegister;
                destinationOperand->forcedBaseRegister = currentInstruction->operands[dstOp].forcedBaseRegister;
                if (isRawTable(scheduleTable)) {
                    int pipe = currentInstruction->position.pipelineIndex;
                    if (!resolveMovePipe(scheduleTable, newMoveInstruction, pipe)) {
                        reportError("failed to resolve a detected move for the pipe I found it in\n");
                        return false;
                    }
                }
                if (!scheduleTable->replaceInstruction(currentInstruction, newMoveInstruction)) {
                    reportError("failed to replace instruction!\n");
                    return false;
                }
                delete currentInstruction;
            }
        }
    }
    return true;
}
