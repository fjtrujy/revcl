#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"

Operand *getEA(Instr *i);
bool sameEA(Instr *a, Instr *b);
bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool addAdditionalClonePredecessors(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

static inline bool fdep_LinkInline(Instr *pre, Instr *post)
{
    if (!pre && !post) {
        reportError("fdep_Link -passed a null ptr\n");
        return false;
    }
    if (!pre->successorDependencies.addInstructionLocation(post)) {
        reportError("failed to add post to the post list\n");
        return false;
    }
    if (!post->predecessorDependencies.addInstructionLocation(pre)) {
        reportError("failed to add pre to the pre list\n");
        return false;
    }
    return true;
}


bool calculateEarliestDependencyRow(Instr *instruction, int *row)
{
    *row = 0;
    for (ol_s *operandLocation = instruction->predecessorDependencies.head; operandLocation; operandLocation = operandLocation->next) {
        Instr *in = operandLocation->instruction;
        if (!in) {
            reportError("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->position.linearRow;
        if (r < 0) {
            *row = -1;
            return true;
        }
        if (r + 1 > *row)
            *row = r + 1;
    }
    return true;
}



bool calculateLatestDependencyRow(Instr *instruction, int *row)
{
    *row = 30000;
    for (ol_s *operandLocation = instruction->successorDependencies.head; operandLocation; operandLocation = operandLocation->next) {
        Instr *in = operandLocation->instruction;
        if (!in) {
            reportError("BuildCandidateList - in_f not set in ol\n");
            return false;
        }
        int r = in->position.linearRow;
        if (r >= 0 && r < *row)
            *row = r;
    }
    return true;
}

bool instStream::buildForwardDependencies(bool forceDependencyChecks, bool b)
{
    if (!workTablePtr) {
        reportWarning("BuildfDepList - no work table set for %s!\n", streamName());
        return false;
    }
    return BuildfDepList(workTablePtr, forceDependencyChecks, b);
}

bool BuildfDepList(workTable *table, bool force, bool)
{
    Instr *instruction;
    Instr *phaseHead;
    instruction = table->linearInstructions.head;
    if (!instruction)
        goto finalize;
    for (; instruction; instruction = instruction->linearLink.nextInstruction) {
        instruction->hasExtraPredecessors = 0;
        instruction->specialRegisterReadMask = 0;
        instruction->specialRegisterWriteMask = 0;
        if (!instruction->predecessorDependencies.clear()) {
            reportError("failed to dispose of existing prefdeplist\n");
            return false;
        }
        if (!instruction->successorDependencies.clear()) {
            reportError("failed to dispose of existing postfdeplist\n");
            return false;
        }
    }
    {
        phaseHead = table->linearInstructions.head;
        instruction = phaseHead;
        if (!phaseHead)
            goto finalize_ready;
        int haveInstruction = phaseHead != 0;
        while (__builtin_expect(haveInstruction, 1)) {
            bool b40 = false;
            bool b10 = false;
            int ramBits = 0;
            Operand *addressOperand = 0;
            bool b20 = false;
            if (instruction->opcodeFlags & 0x08) {
                reportError("found an instruction barrier fdep - this should be done with --barrier fixup!\n");
                instruction->leDump();
                return false;
            }
            if (instruction->opcodeFlags & 0x10)
                b10 = true;
            if (instruction->opcodeFlags & 0x20)
                b20 = true;
            if (instruction->opcodeFlags & 0x40)
                b40 = true;
            if (instruction->ramAccessType == 2) {
                addressOperand = getEA(instruction);
                if (!addressOperand) {
                    reportError("failed to get ea op for store\n");
                    return false;
                }
                b10 = true;
                b20 = true;
                ramBits = instruction->getRamBits();
                if (!ramBits)
                    reportWarning("found store instruction with no ram bits\n");
            }
            if (instruction->ramAccessType == 1) {
                addressOperand = getEA(instruction);
                if (!addressOperand) {
                    reportError("failed to get ea op for load\n");
                    return false;
                }
                b20 = true;
                ramBits = instruction->getRamBits();
                if (!ramBits)
                    reportWarning("found load instruction with no ram bits\n");
            }
            if (b20 || b10 || b40) {
                for (Instr *otherInstruction = instruction->linearLink.previousInstruction; otherInstruction; otherInstruction = otherInstruction->linearLink.previousInstruction) {
                    bool dep = false;
                    int type = (unsigned char)otherInstruction->ramAccessType;
                    if (((b20 && type == 2) || (b10 && type == 1))
                        && (!addressOperand || ((otherInstruction->getRamBits() & ramBits)
                                    && (force || tagsMatch(otherInstruction, instruction) || sameEA(instruction, otherInstruction)))))
                        dep = true;
                    else if (b40 && (otherInstruction->opcodeFlags & 0x40)) {
                        b40 = false;
                        dep = true;
                    } else {
                        if (instruction->ramAccessType == 2 && (otherInstruction->opcodeFlags & 0x20))
                            dep = true;
                        if (instruction->ramAccessType == 1 && (otherInstruction->opcodeFlags & 0x10))
                            dep = true;
                    }
                    if (dep) {
                        if (!fdep_LinkInline(otherInstruction, instruction)) {
                            reportError("failed to add ol for fdeps \n");
                            exitAfterCleanup(-1);
                        }
                    }
                }
            } else if (instruction->branchLabelTags) {
                reportError("tag string, but no match criteria set\n");
                return false;
            }
            for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
                Operand *operand = &instruction->operands[operandIndex];
                if (operand) {
                    RegPoolEntry *registerEntry = registerOperand(operand, 2);
                    if (registerEntry) {
                        if (registerEntry->registerIndex == 48)
                            instruction->specialRegisterWriteMask = operand->componentMask;
                        if (registerEntry->registerIndex == 48 || registerEntry->registerIndex == 49) {
                            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++)
                                if ((operand->componentMask >> partIndex) & 1)
                                    for (ol_s *operandLocation = operand->parts[partIndex].operandLocations.head; operandLocation; operandLocation = operandLocation->next) {
                                        Instr *linkedInstruction = operandLocation->instruction;
                                        if (linkedInstruction)
                                            addAdditionalClonePredecessors(instruction, linkedInstruction);
                                    }
                        }
                    }
                    registerEntry = registerOperand(operand, 1);
                    if (registerEntry && registerEntry->registerIndex == 48)
                        instruction->specialRegisterReadMask = operand->componentMask;
                }
            }
            instruction = instruction->linearLink.nextInstruction;
            haveInstruction = instruction != 0;
        }
    }
finalize:
    phaseHead = table->linearInstructions.head;
finalize_ready:
    instruction = phaseHead;
    Instr *last[4] = { 0, 0, 0, 0 };
    // After entry, phaseHead is only a non-null continuation flag.
    for (; phaseHead; phaseHead = (Instr *)((instruction = instruction->linearLink.nextInstruction) != 0)) {
        int p;
        for (p = 0; p != 4; p++)
            if (((instruction->specialRegisterWriteMask >> p) & 1) && !((instruction->specialRegisterReadMask >> p) & 1))
                last[p] = instruction;
        for (p = 0; p != 4; p++) {
            if ((instruction->specialRegisterReadMask >> p) & 1) {
                Instr *writerInstruction = last[p];
                if (writerInstruction)
                    for (ol_s *operandLocation = instruction->predecessorDependencies.head; operandLocation; operandLocation = operandLocation->next) {
                        Instr *pre = operandLocation->instruction;
                        if (pre->linearOrderIndex < writerInstruction->linearOrderIndex) {
                            writerInstruction->hasExtraPredecessors = 1;
                            writerInstruction->predecessorDependencies.addInstructionLocation(pre);
                        }
                    }
            }
        }
        instruction->dumpOperandDiagnostics(false);
        instruction->predecessorDependencies.validateOperandLocations();
        instruction->successorDependencies.validateOperandLocations();
    }
    return true;
}

bool tagsMatch(Instr *instruction, Instr *secondInstruction)
{
    char *firstTags = instruction->branchLabelTags;
    if (!firstTags)
        return false;
    char *q0 = secondInstruction->branchLabelTags;
    if (!q0)
        return false;
    while (*firstTags) {
        char *secondTags = q0;
        while (*secondTags) {
            char *firstTagCursor = firstTags;
            while (*secondTags && *firstTagCursor && *secondTags != ':' && *firstTagCursor != ':') {
                secondTags++;
                firstTagCursor++;
            }
            if (*secondTags == *firstTagCursor)
                return true;
            while (*secondTags && *secondTags != ':')
                secondTags++;
            if (*secondTags == ':')
                secondTags++;
        }
        while (*firstTags && *firstTags != ':')
            firstTags++;
        if (*firstTags == ':')
            firstTags++;
    }
    return false;
}

bool fixupCloneFdeps(workTable *table)
{
    int n = 0;
    for (Instr *instruction = table->linearInstructions.head; instruction; instruction = instruction->linearLink.nextInstruction) {
        Instr *orig = instruction->cloneImage;
        n += 8;
        instruction->linearOrderIndex = n;
        if (!orig) {
            reportError("no original set\n");
            exitAfterCleanup(-1);
        }
        instruction->hasExtraPredecessors = orig->hasExtraPredecessors;
        if (!copyCloneFdeps(instruction, orig->predecessorDependencies.head, &instruction->predecessorDependencies)) {
            reportError("failed to fixup the pre fdep list\n");
            return false;
        }
        if (!copyCloneFdeps(instruction, orig->successorDependencies.head, &instruction->successorDependencies)) {
            reportError("failed to fix the post fdeps\n");
            return false;
        }
    }
    return true;
}

bool fdep_Link(Instr *pre, Instr *post)
{
    if (!pre && !post) {
        reportError("fdep_Link -passed a null ptr\n");
        return false;
    }
    if (!pre->successorDependencies.addInstructionLocation(post)) {
        reportError("failed to add post to the post list\n");
        return false;
    }
    if (!post->predecessorDependencies.addInstructionLocation(pre)) {
        reportError("failed to add pre to the pre list\n");
        return false;
    }
    return true;
}

bool copyCloneFdeps(Instr *, ol_s *operandLocation, olList *dst)
{
    for (; operandLocation; operandLocation = operandLocation->next) {
        Instr *instruction = operandLocation->instruction;
        if (!instruction) {
            reportError("qInstruction.. fDepList in_f not set in ol\n");
            return false;
        }
        Instr *clone = instruction->cloneImage;
        if (!clone) {
            reportError("qInstruction - fdep list - clone image not set during rebuild\n");
            return false;
        }
        if (!dst->addInstructionLocation(clone)) {
            reportError("qInstruction - failed to add an OL to the fdep list from clone\n");
            return false;
        }
    }
    return true;
}

bool addAdditionalClonePredecessors(Instr *instruction, Instr *other)
{
    instruction->dumpOperandDiagnostics(false);
    other->dumpOperandDiagnostics(false);
    for (int operandIndex = 0; operandIndex != other->operandCount; operandIndex++) {
        Operand *operand = &other->operands[operandIndex];
        RegPoolEntry *registerEntry = registerOperand(operand, 1);
        if (registerEntry) {
            for (int partIndex = 0; partIndex != registerEntry->partCount; partIndex++)
                if ((operand->componentMask >> partIndex) & 1)
                    for (ol_s *operandLocation = operand->parts[partIndex].operandLocations.head; operandLocation; operandLocation = operandLocation->next) {
                        Instr *linkedInstruction = operandLocation->instruction;
                        if (linkedInstruction && linkedInstruction != instruction && linkedInstruction->linearOrderIndex < instruction->linearOrderIndex) {
                            instruction->hasExtraPredecessors = 1;
                            if (!instruction->predecessorDependencies.addInstructionLocation(linkedInstruction)) {
                                reportError("failed to add\n");
                                exitAfterCleanup(-1);
                            }
                            linkedInstruction->dumpOperandDiagnostics(false);
                        }
                    }
        }
    }
    return true;
}
