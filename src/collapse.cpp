#include "os.h"
#include "mValue.h"
#include "Program.h"
#include "instStream.h"
#include "workTable.h"
#include "Instr.h"
#include "ol_s.h"

RegPoolEntry *collapseImmediateSourceRegister;
mValue *collapseImmediateValue;
int collapseImmediateOffset;
int decodeCollapsibleMoveImmediate(Instr *in);
int expandOneLoopIntoRegions(Program *p, instStream *s);

static inline int fitsSigned(int v, int bits)
{
    int r = 0;
    int hi = 1 << (bits - 1);
    int lo = -hi;
    if (v >= lo && v < hi)
        r = 1;
    return r;
}

static inline bool fitsUnsigned(int v, int bits)
{
    return v >= 0 && v < (1 << bits);
}
bool immCalc(Instr *in, int *srcOp, int *immOp, int *val, int *opc);

bool pushImmediate(ol_s *operandLocation)
{
    for (; operandLocation; operandLocation = operandLocation->next) {
        Instr *instruction = operandLocation->instruction;
        int val = 0;
        int srcOp = -1;
        int immOp = -1;
        int opc = 0;
        if (!immCalc(instruction, &srcOp, &immOp, &val, &opc)) {
            reportError("imm_calc failed during push\n");
            return false;
        }
        int nopc = opc;
        if (opc == 0x6e && val < 0)
            nopc = 0x70;
        if (opc == 0x70 && val < 0)
            nopc = 0x6e;
        if (opc != nopc) {
            val = -val;
            instruction->opcodeInfo = getOpcodeInfoById(nopc);
        }
        if (!instruction->operands[immOp].immediateValue.setInt(val)) {
            reportError("failed to set new immediate value\n");
            return false;
        }
    }
    return true;
}

bool collapseProgramStreams(Program *program)
{
    bool changed = false;
    instStream *stream;
    for (stream = program->streamHead; stream; stream = stream->nextStream) {
        workTable *table = stream->getTable();
        for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
            if (__builtin_expect(instruction->opcodeId() != 0x6d, 1))
                continue;
            mValue *value = &instruction->operands[2].immediateValue;
            if (value->type != mValue::STRING) {
                int val = 0;
                if (!value->getInt(&val)) {
                    reportError("failed to get immediate value for an iaddi\n");
                    return false;
                }
                int opc;
                if (val < 0) {
                    val = -val;
                    opc = 0x70;
                } else
                    opc = 0x6e;
                instruction->opcodeInfo = getOpcodeInfoById(opc);
                if (!instruction->opcodeInfo) {
                    reportError("couldn't find replacement instruction\n");
                    return false;
                }
                if (!value->setInt(val)) {
                    reportError("failed to set new val\n");
                    return false;
                }
            }
        }
    }
    do {
        changed = false;
        for (stream = program->streamHead; stream; stream = stream->nextStream) {
            workTable *table = stream->getTable();
            for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
                int k = decodeCollapsibleMoveImmediate(instruction);
                if (k == -2) {
                    reportError("an immediate is wrong someplace\n");
                    return false;
                }
                if (k < 0)
                    continue;
                bool ok = true;
                ol_s *list = instruction->operands[k].parts[0].operandLocations.head;
                for (ol_s *operandLocation = list; operandLocation && ok; operandLocation = operandLocation->next) {
                    Instr *user = operandLocation->instruction;
                    Operand *operand = operandAtLocation(operandLocation);
                    if (user == instruction) {
                        ok = false;
                        break;
                    }
                    int idx = operandLocation->operandIndex;
                    int immOp = -1;
                    int srcOp = -1;
                    int val;
                    int opc = 0;
                    int same;
                    if (!immCalc(user, &srcOp, &immOp, &val, &opc))
                        same = false;
                    else
                        same = idx == srcOp;
                    if (!(char)same) {
                        ok = false;
                        break;
                    }
                    for (ol_s *userLocation = operand->parts[0].operandLocations.head; userLocation; userLocation = userLocation->next)
                        if (userLocation->instruction != instruction) {
                            ok = false;
                            break;
                        }
                }
                if (ok) {
                    if (!pushImmediate(list)) {
                        reportError("failed to push an immediate into a register\n");
                        return false;
                    }
                    collapseImmediateValue->setInt(0);
                    changed = true;
                }
            }
        }
    } while (changed);
    return true;
}

static inline bool isStringValue(mValue *value)
{
    return value->type == mValue::STRING;
}

int decodeCollapsibleMoveImmediate(Instr *instruction)
{
    int opc = instruction->opcodeId();
    int bits = 0;
    int idx = -1;
    bool neg = false;
    bool isSigned = false;
    int val = 0;
    collapseImmediateValue = 0;
    collapseImmediateSourceRegister = 0;
    switch (opc) {
    case 0x6e:
        bits = 15;
        idx = 0;
        isSigned = false;
        collapseImmediateSourceRegister = instruction->operands[1].assignedRegister;
        collapseImmediateValue = &instruction->operands[2].immediateValue;
        break;
    case 0x70:
        bits = 15;
        isSigned = false;
        collapseImmediateSourceRegister = instruction->operands[1].assignedRegister;
        collapseImmediateValue = &instruction->operands[2].immediateValue;
        idx = 0;
        neg = true;
        break;
    }
    if (idx < 0)
        return -1;
    if (isStringValue(collapseImmediateValue))
        return -1;
    if (!collapseImmediateValue->getInt(&val)) {
        reportError("failed to get immediate value\n");
        return -2;
    }
    if (val == 0)
        return -1;
    if (isSigned) {
        if (!(char)fitsSigned(val, bits)) {
            reportWarning("immediate %d doesn't fit as signed value in %d bits - skipping\n", val, bits);
            return -1;
        }
    } else if (!fitsUnsigned(val, bits)) {
        reportWarning("immediate %d doesn't fit as unsigned value in %d bits - skipping\n", val, bits);
        return -1;
    }
    collapseImmediateOffset = neg ? -val : val;
    return idx;
}

static inline int immFitsUnsigned(int v, int bits)
{
    int r = 0;
    if (v >= 0 && v < (1 << bits))
        r = 1;
    return r;
}

bool immCalc(Instr *instruction, int *srcOp, int *immOp, int *val, int *opc)
{
    *srcOp = -1;
    *immOp = -1;
    *opc = instruction->opcodeId();
    bool sgn = false;
    bool sub = false;
    switch (*opc) {
    case 0x9b:
    case 0x9c:
    case 0xa0:
    case 0xa1:
        *srcOp = 1;
        sgn = true;
        *immOp = 1;
        break;
    case 0x6e:
        *srcOp = 1;
        *immOp = 2;
        break;
    case 0x70:
        *srcOp = 1;
        *immOp = 2;
        sub = true;
        break;
    default:
        return false;
    }
    mValue *value = &instruction->operands[*immOp].immediateValue;
    if (isStringValue(value))
        return false;
    int x = 0;
    if (!value->getInt(&x)) {
        reportError("failed to get immediate value\n");
        return false;
    }
    x = sub ? x - collapseImmediateOffset : x + collapseImmediateOffset;
    if (sgn) {
        if (!(char)fitsSigned(x, 11))
            return false;
    } else if (!(char)immFitsUnsigned(x < 0 ? -x : x, 15)) {
        return false;
    }
    *val = x;
    return true;
}
