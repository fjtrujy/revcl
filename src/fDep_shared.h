#ifndef FDEP_SHARED_H
#define FDEP_SHARED_H

#include "os.h"
#include "Instr.h"
#include "ol_s.h"
#include "mValue.h"
#include "instStream.h"
#include "workTable.h"
#include "fDep.h"

Operand *getEA(Instr *i);
bool sameEA(Instr *a, Instr *b);
bool BuildfDepList(workTable *wt, bool force, bool);
bool tagsMatch(Instr *a, Instr *b);
bool func_08065fd4(Instr *in, Instr *other);
bool copyCloneFdeps(Instr *in, ol_s *o, olList *dst);

#endif
