#include "os.h"

#include "backTrace_shared.h"

static inline regUseSlot *registerPart(regUse *u, int part)
{
    return &u->slots[part];
}

bool linkMatchList(Program *p, instStream *t, RegPoolEntry *reg, int part, int ofs, bool ext)
{
    regUse *tr = t->backtraceRegisterUse;
    if (!tr) {
        reportError("no rfu ?\n");
        return false;
    }
    for (instStream *s = p->matchedStreams; s; s = s->nextMatchedStream) {
        if (s->visitState != 3) {
            reportError("not end_match on the newmatchlist\n");
            return false;
        }
        int minKey = buildMinimumPathKey(p, s);
        if (minKey <= 0) {
            reportError("minimal path key was bad\n");
            return false;
        }
        int maxKey = -1;
        if (p->trackMaximumPathLength) {
            maxKey = getMaxPathKey(p, s);
            if (maxKey <= 0) {
                reportError("error getting max path key\n");
                return false;
            }
        }
        regUse *sr = s->backtraceRegisterUse;
        if (!sr) {
            reportError("an entry on the new match list had no regref?\n");
            return false;
        }
        if (s->visitState != 3) {
            reportError("newmatchlist had non -end-match \n");
            return false;
        }
        int len = ofs + s->minimumPathLength;
        int d = s->earliestOutputLocation - len;
        bool extended = false;
        if (d > 0) {
            if (ext) {
                if (!p->proc.stalls.addStall(registerPart(sr, part)->outputLocations.head,
                                        registerPart(tr, part)->inputLocations.head, 0, d)) {
                    reportError("failed to add an external stall\n");
                    return false;
                }
            } else {
                s->visitState = 2;
                extended = true;
                if (!extendBackTrace(p, s)) {
                    reportError("failed to extend this back\n");
                    return false;
                }
            }
        }
        for (ol_s *ol = tr->slots[part].inputLocations.head; ol; ol = ol->next) {
            ol_s *last = sr->slots[part].findPreviousOutputAtPosition(0, true);
            ol_s *match = 0;
            if (ext)
                match = last;
            else {
                int mn = s->minimumPathLength;
                int mx = s->maximumPathLength;
                if (mx < mn) {
                    reportError("max_ilen<min_ilen for a non-blocking instruction match.. %d < %d and logmax is %d\n",
                                  mx, mn, p->trackMaximumPathLength);
                    return false;
                }
                if (mn < 0) {
                    reportError("minimal length<0\n");
                    return false;
                }
                int pos = ol->instruction->scheduledCycle;
                if (pos < 0) {
                    reportError("instruction not inserted or emu'd??\n");
                    return false;
                }
                ol_s *min_s = sr->slots[part].findPreviousOutputAtPosition(pos + mn, false);
                ol_s *max_s = sr->slots[part].findPreviousOutputAtPosition(pos + mx, false);
                if (min_s) {
                    if (!max_s) {
                        reportError("min_s set, but couldn't find max_s\n");
                        return false;
                    }
                    if (min_s != max_s) {
                        int mode = t->getRawSchedulingMode();
                        if (mode >= 2 && mode <= 4) {
                            reportWarning("multiple same-sequence output operands match to a fixed stream %s..setting hazard flags\n",
                                          t->streamName());
                            tr->hazardMask |= 1 << part;
                            sr->sourcePathHazardMask |= 1 << part;
                            s->hazardFlag = 1;
                        } else {
                            reportWarning("vcl found a RAW stream with a non blocking operation for '%s' where by going down differing paths, an operand was set in different places..\n",
                                          reg->registerName());
                            reportWarning("this is a bit dodgy - min_ilen=%d.. max_ilen=%d.. target is %s, and match is  %s\n",
                                          mn, mx, t->streamName(), s->streamName());
                            reportWarning("I can't really guarantee that the code is correct after this - but - since vcl would stop anyway here, let's carry on and see what happens... \n");
                            reportWarning("carrying on.. fixed raw mode. carrying on..\n");
                            reportWarning("paths - min..\n");
                            reportWarning("well - setting hiBits and localH this time..\n");
                            tr->hazardMask |= 1 << part;
                            sr->sourcePathHazardMask |= 1 << part;
                            s->hazardFlag = 1;
                        }
                    }
                }
                if (min_s != last) {
                    s->hazardFlag = 1;
                    sr->sourcePathHazardMask |= 1 << part;
                    tr->hazardMask |= 1 << part;
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
                sr->externalOutputMask |= 1 << part;
                if (!link_oParts(match, ol, minKey, maxKey)) {
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
