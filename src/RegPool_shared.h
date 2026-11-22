#ifndef REGPOOL_SHARED_H
#define REGPOOL_SHARED_H

#include "RegPool.h"
#include "ol_s_inline.h"
#include "os.h"
#include "VUStr.h"
#include "workTable.h"

struct iInfo;
iInfo *findOpcodeInfoByMnemonic(const char *name, bool);
bool asciiCaseEqual(const char *a, const char *b);

#endif
