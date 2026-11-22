#include "os.h"

#include "backTrace_shared.h"

static inline regUseSlot *registerPart(regUse *registerUse, int part)
{
    return &registerUse->slots[part];
}

bool linkMatchList(Program *program, instStream *targetStream, RegPoolEntry *reg, int part, int ofs, bool ext)
{
    regUse *registerUse = targetStream->backtraceRegisterUse;
    if (!registerUse) {
        reportError("no rfu ?\n");
        return false;
    }
    for (instStream *matchedStream = program->matchedStreams; matchedStream; matchedStream = matchedStream->nextMatchedStream) {
        if (matchedStream->visitState != 3) {
            reportError("not end_match on the newmatchlist\n");
            return false;
        }
        int minKey = buildMinimumPathKey(program, matchedStream);
        if (minKey <= 0) {
            reportError("minimal path key was bad\n");
            return false;
        }
        int maxKey = -1;
        if (program->trackMaximumPathLength) {
            maxKey = getMaxPathKey(program, matchedStream);
            if (maxKey <= 0) {
                reportError("error getting max path key\n");
                return false;
            }
        }
        regUse *matchedRegisterUse = matchedStream->backtraceRegisterUse;
        if (!matchedRegisterUse) {
            reportError("an entry on the new match list had no regref?\n");
            return false;
        }
        if (matchedStream->visitState != 3) {
            reportError("newmatchlist had non -end-match \n");
            return false;
        }
        int len = ofs + matchedStream->minimumPathLength;
        int outputDistance = matchedStream->earliestOutputLocation - len;
        bool extended = false;
        if (outputDistance > 0) {
            if (ext) {
                if (!program->proc.stalls.addStall(registerPart(matchedRegisterUse, part)->outputLocations.head,
                                        registerPart(registerUse, part)->inputLocations.head, 0, outputDistance)) {
                    reportError("failed to add an external stall\n");
                    return false;
                }
            } else {
                matchedStream->visitState = 2;
                extended = true;
                if (!extendBackTrace(program, matchedStream)) {
                    reportError("failed to extend this back\n");
                    return false;
                }
            }
        }
        for (ol_s *location = registerUse->slots[part].inputLocations.head; location; location = location->next) {
            ol_s *last = matchedRegisterUse->slots[part].findPreviousOutputAtPosition(0, true);
            ol_s *match = 0;
            if (ext)
                match = last;
            else {
                int mn = matchedStream->minimumPathLength;
                int mx = matchedStream->maximumPathLength;
                if (mx < mn) {
                    reportError("max_ilen<min_ilen for a non-blocking instruction match.. %d < %d and logmax is %d\n",
                                  mx, mn, program->trackMaximumPathLength);
                    return false;
                }
                if (mn < 0) {
                    reportError("minimal length<0\n");
                    return false;
                }
                int pos = location->instruction->scheduledCycle;
                if (pos < 0) {
                    reportError("instruction not inserted or emu'd??\n");
                    return false;
                }
                ol_s *min_s = matchedRegisterUse->slots[part].findPreviousOutputAtPosition(pos + mn, false);
                ol_s *max_s = matchedRegisterUse->slots[part].findPreviousOutputAtPosition(pos + mx, false);
                if (min_s) {
                    if (!max_s) {
                        reportError("min_s set, but couldn't find max_s\n");
                        return false;
                    }
                    if (min_s != max_s) {
                        int mode = targetStream->getRawSchedulingMode();
                        if (mode >= 2 && mode <= 4) {
                            reportWarning("multiple same-sequence output operands match to a fixed stream %s..setting hazard flags\n",
                                          targetStream->streamName());
                            registerUse->hazardMask |= 1 << part;
                            matchedRegisterUse->sourcePathHazardMask |= 1 << part;
                            matchedStream->hazardFlag = 1;
                        } else {
                            reportWarning("vcl found a RAW stream with a non blocking operation for '%s' where by going down differing paths, an operand was set in different places..\n",
                                          reg->registerName());
                            reportWarning("this is a bit dodgy - min_ilen=%d.. max_ilen=%d.. target is %s, and match is  %s\n",
                                          mn, mx, targetStream->streamName(), matchedStream->streamName());
                            reportWarning("I can't really guarantee that the code is correct after this - but - since vcl would stop anyway here, let's carry on and see what happens... \n");
                            reportWarning("carrying on.. fixed raw mode. carrying on..\n");
                            reportWarning("paths - min..\n");
                            reportWarning("well - setting hiBits and localH this time..\n");
                            registerUse->hazardMask |= 1 << part;
                            matchedRegisterUse->sourcePathHazardMask |= 1 << part;
                            matchedStream->hazardFlag = 1;
                        }
                    }
                }
                if (min_s != last) {
                    matchedStream->hazardFlag = 1;
                    matchedRegisterUse->sourcePathHazardMask |= 1 << part;
                    registerUse->hazardMask |= 1 << part;
                }
                if (min_s)
                    match = min_s;
                else if (max_s) {
                    reportWarning("min_s not set.. but max_s is..so, taking that\n");
                    match = max_s;
                }
            }
            if (match) {
                markLocationExternallyUsed(match);
                matchedRegisterUse->externalOutputMask |= 1 << part;
                if (!link_oParts(match, location, minKey, maxKey)) {
                    reportError("failed to link_oparts external\n");
                    match->dumpInstructionContext();
                    return false;
                }
            }
            if (!ext && !match && !extended) {
                reportError("non blocking, no match, but didn't extend\n");
                return false;
            }
        }
    }
    return true;
}
