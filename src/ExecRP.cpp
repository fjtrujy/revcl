// ExecRP register-pool init handlers, CIE group 3.
#include "Parser.h"
#include "RegPool.h"
#include "Program.h"
#include "os.h"

char *dupString(const char *s);
extern bool disableOutputAlignment;
extern int errorOccurred;
extern bool quietDiagnostics;

bool asciiCaseEqual(const char *a, const char *b);
RegPool *GetRP(unsigned t);
bool setRegisterRangeEnabled(LocTag *t, RegPool *pool, bool b);
bool pushVisibilityScope(Program *prog, LocTag *t, int v);
bool popVisibilityScope(Program *prog, LocTag *t, int v);
bool initializeMemoryRegisterArea(RegPool *pool, const char *name, int size);
int mstrlen(const char *s);
bool handleInitVfDirective(Program *prog, LocTag *t);
bool handleInitViDirective(Program *prog, LocTag *t);
bool handleRemoveVfDirective(Program *prog, LocTag *t);
bool handleRemoveViDirective(Program *prog, LocTag *t);
bool handleInitAllVfDirective(Program *prog, LocTag *t);
bool handleInitAllViDirective(Program *prog, LocTag *t);
bool handleDisableAlignmentDirective(Program *prog, LocTag *t);
bool handleDisableEbitDirective(Program *prog, LocTag *t);
bool handleProgramNameDirective(Program *prog, LocTag *t);
bool handleVuDirective(Program *prog, LocTag *t);
bool handleMpgDirective(Program *prog, LocTag *t);
bool handleGlobalDirective(Program *prog, LocTag *t);
bool handleBeginRawDirective(Program *prog, LocTag *t);
bool handleEndRawDirective(Program *prog, LocTag *t);
bool handleBeginRawLoopDirective(Program *prog, LocTag *t);
bool handleEndRawLoopDirective(Program *prog, LocTag *t);
bool handleVuWorkAreaDirective(Program *prog, LocTag *t);
bool handleAlignmentDirective(Program *prog, LocTag *t);
bool handleHideDirective(Program *prog, LocTag *t);
bool handleShowDirective(Program *prog, LocTag *t);
bool handleEndHideDirective(Program *prog, LocTag *t);
bool handleEndShowDirective(Program *prog, LocTag *t);
bool handleUniqueDirective(Program *prog, LocTag *t);
bool handleSyntaxDirective(Program *prog, LocTag *t);

Directive directiveDispatchTable[28] = {
    {".vu", handleVuDirective},
    {".mpg", handleMpgDirective},
    {".vsm", handleBeginRawDirective},
    {".endvsm", handleEndRawDirective},
    {".vsmloop", handleBeginRawLoopDirective},
    {".endvsmloop", handleEndRawLoopDirective},
    {".init_vf", handleInitVfDirective},
    {".init_vi", handleInitViDirective},
    {".rem_vf", handleRemoveVfDirective},
    {".rem_vi", handleRemoveViDirective},
    {".init_vf_all", handleInitAllVfDirective},
    {".init_vi_all", handleInitAllViDirective},
    {".vu_work_area", handleVuWorkAreaDirective},
    {".name", handleProgramNameDirective},
    {".syntax", handleSyntaxDirective},
    {".align", handleAlignmentDirective},
    {".global", handleGlobalDirective},
    {".noaligns", handleDisableAlignmentDirective},
    {".noebits", handleDisableEbitDirective},
    {".raw", handleBeginRawDirective},
    {".endraw", handleEndRawDirective},
    {".rawloop", handleBeginRawLoopDirective},
    {".endrawloop", handleEndRawLoopDirective},
    {".unique", handleUniqueDirective},
    {".show", handleShowDirective},
    {".hide", handleHideDirective},
    {".endshow", handleEndShowDirective},
    {".endhide", handleEndHideDirective},
};

bool handleInitVfDirective(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(9), true);
}

bool handleInitViDirective(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(8), true);
}

bool handleRemoveVfDirective(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(9), false);
}

bool handleRemoveViDirective(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(8), false);
}

inline bool ExecRP_init_reg_all(RegPool *pool)
{
    if (!pool) {
        reportError("ExecRP_init_reg_all: passed a null pool\n");
        return false;
    }
    return pool->markAllocatablePhysicalRegistersUsed();
}

bool handleInitAllVfDirective(Program *prog, LocTag *sourceLine)
{
    return ExecRP_init_reg_all(GetRP(9));
}

bool handleInitAllViDirective(Program *prog, LocTag *sourceLine)
{
    return ExecRP_init_reg_all(GetRP(8));
}

bool handleDisableAlignmentDirective(Program *prog, LocTag *sourceLine)
{
    disableOutputAlignment = true;
    reportInfo("dh_align_disable() - disabling .align emit\n");
    return 1;
}

bool handleDisableEbitDirective(Program *prog, LocTag *sourceLine)
{
    disableOutputAlignment = true;
    reportInfo("dh_no_ebits() - disabling [e]bit emit\n");
    return 1;
}

bool handleProgramNameDirective(Program *prog, LocTag *sourceLine)
{
    char *nameToken = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (!nameToken) {
        reportError("failed to get global name\n");
        return 0;
    } else {
        prog->proc.globalName = dupString(nameToken);
    }
    return 1;
}

bool handleVuDirective(Program *prog, LocTag *sourceLine)
{
    prog->proc.vuDirectiveSeen = true;
    return 1;
}

bool handleMpgDirective(Program *prog, LocTag *sourceLine)
{
    char *addressToken = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (!addressToken) {
        reportError("must specify target address for mpg mode!\n");
        return 0;
    }
    prog->proc.mpgTargetAddress = parseInteger(addressToken);
    return 1;
}

bool handleGlobalDirective(Program *prog, LocTag *sourceLine)
{
    char *symbolToken = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (symbolToken)
        prog->proc.exportedSymbols.findOrAppendByExactName(symbolToken);
    return 1;
}

bool handleBeginRawDirective(Program *prog, LocTag *sourceLine)
{
    if (prog->proc.insideRawLoop) {
        reportUserMessage("hit a raw directive in a rawloop block\n");
        return 0;
    }
    if (prog->proc.decodeMode == 3)
        reportWarning("hit a raw directive while already in a raw block\n");
    prog->proc.decodeMode = 3;
    prog->proc.decodeSubMode = 4;
    char *reorderOption = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (reorderOption) {
        if (asciiCaseEqual(reorderOption, "reorder"))
            prog->proc.decodeSubMode = 3;
        else if (asciiCaseEqual(reorderOption, "noreorder"))
            prog->proc.decodeSubMode = 4;
        else {
            reportUserMessage("bad option '%s' to raw directive\n", reorderOption);
            return 0;
        }
    }
    return 1;
}

bool handleEndRawDirective(Program *prog, LocTag *sourceLine)
{
    if (prog->proc.decodeMode == 1) {
        reportWarning("hit an endraw directive while not in raw mode\n");
        return 0;
    }
    if (prog->proc.insideRawLoop) {
        reportUserMessage("hit an endraw directive inside a rawloop\n");
        return 0;
    }
    prog->proc.decodeMode = 1;
    prog->proc.decodeSubMode = 0;
    return 1;
}

bool handleBeginRawLoopDirective(Program *prog, LocTag *sourceLine)
{
    if (prog->proc.insideRawLoop) {
        reportUserMessage("encountered a .rawloop directive, while already in loop mode\n");
        return 0;
    }
    prog->proc.rawLoopId++;
    prog->proc.savedDecodeMode = prog->proc.decodeMode;
    prog->proc.decodeMode = 3;
    prog->proc.decodeSubMode = 4;
    prog->proc.insideRawLoop = true;
    char *reorderOption = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (reorderOption) {
        if (asciiCaseEqual(reorderOption, "reorder"))
            prog->proc.decodeSubMode = 3;
        else if (asciiCaseEqual(reorderOption, "noreorder"))
            prog->proc.decodeSubMode = 4;
        else {
            reportUserMessage("bad option '%s' to raw loop directive\n", reorderOption);
            return 0;
        }
    }
    return 1;
}

bool handleEndRawLoopDirective(Program *prog, LocTag *sourceLine)
{
    if (!prog->proc.insideRawLoop) {
        reportUserMessage("reached an .endloop directive but when wasn't in loop mode\n");
        return 0;
    }
    if (prog->proc.decodeMode != 3) {
        reportUserMessage("hit end raw directive, but wasn't in VSM mode..(mode is %d)?\n", prog->proc.decodeMode);
        return 0;
    }
    prog->proc.decodeMode = prog->proc.savedDecodeMode;
    prog->proc.decodeSubMode = prog->proc.savedDecodeSubMode;
    prog->proc.insideRawLoop = false;
    return 1;
}

bool handleVuWorkAreaDirective(Program *prog, LocTag *sourceLine)
{
    int device = -1;
    int limit = -1;
    char *cpu = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (!cpu) {
        reportError("failed to get token cpu (1) from .work directive\n");
        return 0;
    }
    if (asciiCaseEqual(cpu, "core")) {
        reportError("work: core cpu work area.. not yet\n");
        return 0;
    }
    if (asciiCaseEqual(cpu, "vu0")) {
        device = 1;
        limit = 0x100;
    }
    if (asciiCaseEqual(cpu, "vu1")) {
        device = 2;
        limit = 0x400;
    }
    if (limit < 0) {
        reportError("work: target device not set defining a work area.. must be one of core,vu0 or vu1!\n");
        return 0;
    }
    char *addressToken = sourceLine->isCommentToken(2) ? 0 : sourceLine->tokenTextAt(2, 0);
    if (!addressToken) {
        reportError("failed to get effective address token (2) from .work directive\n");
        return 0;
    }
    char *sizeToken = sourceLine->isCommentToken(3) ? 0 : sourceLine->tokenTextAt(3, 0);
    if (!sizeToken) {
        reportError("failed to get size token (3) from .work directive\n");
        return 0;
    }
    int addr = parseInteger(addressToken);
    int size = parseInteger(sizeToken);
    if (size <= 0) {
        reportError("size is <=0 for a work area\n");
        return 0;
    }
    if (addr < 0) {
        reportError("ea is negative for a work area\n");
        return 0;
    }
    if (addr + size > limit) {
        reportError("work area overruns size of device\n");
        return 0;
    }
    RegPool *pool = GetRP(11);
    if (!pool) {
        reportError("init work area - failed to get vu mem pool\n");
        return 0;
    }
    if (!initializeMemoryRegisterArea(pool, addressToken, size)) {
        reportError("failed to init the mem area!\n");
        return 0;
    }
    reportWarning("work_area: created work area for device %d - [%4d:%-4d]\n", device, addr, size);
    return 1;
}

inline bool ExecRP_skip_directive(LocTag *sourceLine)
{
    reportWarning("skipping directive:\n");
    sourceLine->dumpSourceLine(-1);
    return true;
}

bool handleAlignmentDirective(Program *prog, LocTag *sourceLine)
{
    return ExecRP_skip_directive(sourceLine);
}

bool handleHideDirective(Program *prog, LocTag *sourceLine)
{
    return pushVisibilityScope(prog, sourceLine, 0);
}

bool handleShowDirective(Program *prog, LocTag *sourceLine)
{
    return pushVisibilityScope(prog, sourceLine, 1);
}

bool handleEndHideDirective(Program *prog, LocTag *sourceLine)
{
    return popVisibilityScope(prog, sourceLine, 0);
}

bool handleEndShowDirective(Program *prog, LocTag *sourceLine)
{
    return popVisibilityScope(prog, sourceLine, 1);
}

bool handleUniqueDirective(Program *prog, LocTag *sourceLine)
{
    char *name = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (name) {
        if (mstrlen(name) > 128) {
            reportUserMessage("unique name too long\n");
            return 0;
        }
        prog->proc.setUniqueName(name);
    }
    prog->proc.uniqueAll = 1;
    return 1;
}

bool handleSyntaxDirective(Program *prog, LocTag *sourceLine)
{
    char *syntaxOption = sourceLine->isCommentToken(1) ? 0 : sourceLine->tokenTextAt(1, 0);
    if (!syntaxOption) {
        reportUserMessage("must specify an argument to '.syntax', valid ones are 'new' or 'old'\n");
        return 0;
    }
    if (asciiCaseEqual(syntaxOption, "new")) {
        prog->proc.newSyntax = true;
        return 1;
    }
    if (asciiCaseEqual(syntaxOption, "old")) {
        prog->proc.newSyntax = false;
        return 1;
    }
    reportUserMessage("invalid argument to .syntax, options are 'new' or 'old'\n");
    return 0;
}

inline char *ExecRP_directive_name(LocTag *sourceLine)
{
    return sourceLine->tokenTextAt(0, 0);
}

Directive *findRegisteredDirective(LocTag *sourceLine)
{
    if (!sourceLine)
        return 0;
    char *name = ExecRP_directive_name(sourceLine);
    Directive *directive = directiveDispatchTable;
    for (int directiveIndex = 0; directiveIndex != 28; directiveIndex++, directive++)
        if (asciiCaseEqual(name, directive->name))
            return directive;
    return 0;
}

bool setRegisterRangeEnabled(LocTag *sourceLine, RegPool *pool, bool flag)
{
    char buf[2][0x800];
    int tokenIndex = 1;
    if (!pool) {
        reportError("ExecRP_init_reg: passed a null pool\n");
        return false;
    }
    for (; tokenIndex < getLineTokenCount(sourceLine); tokenIndex++) {
        char *tok = sourceLine->isCommentToken(tokenIndex) ? 0 : sourceLine->tokenTextAt(tokenIndex, 0);
        if (!tok)
            continue;
        int range = 0;
        char *outputCursor = buf[0];
        for (char *inputCursor = tok; *inputCursor; inputCursor++) {
            if (*inputCursor == '-') {
                if (range) {
                    reportUserMessage("too many '-' chars in %s\n", tok);
                    return false;
                }
                range = 1;
                *outputCursor = 0;
                outputCursor = buf[1];
            } else
                *outputCursor++ = *inputCursor;
        }
        *outputCursor = 0;
        int first = -1, last;
        int j;
        for (j = 0; j <= range; j++) {
            RegPoolEntry *registerEntry = pool->findPhysicalByName(buf[j]);
            if (!registerEntry) {
                reportError("not find %s\n", tok);
                return false;
            }
            if (!registerEntry->allocatable) {
                reportError("init_reg: %s is not allocable\n", registerEntry->registerName());
                return false;
            }
            if (registerEntry->isVirtual) {
                reportError("init_reg: reg is virtual! %s\n", registerEntry->registerName());
                return false;
            }
            if (j == 0)
                first = last = registerEntry->registerIndex;
            else
                last = registerEntry->registerIndex;
        }
        if (last < first) {
            reportWarning("range %s decreasing\n", tok);
            int swapIndex = last;
            last = first;
            first = swapIndex;
        }
        for (j = first; j <= last; j++) {
            RegPoolEntry *registerEntry = pool->findByRegisterIndex(j);
            if (!registerEntry) {
                reportError("not find %s\n", tok);
                return false;
            }
            if (!registerEntry->allocatable) {
                reportError("init_reg: %s is not allocable\n", registerEntry->registerName());
                return false;
            }
            if (registerEntry->isVirtual) {
                reportError("init_reg: reg is virtual! %s\n", registerEntry->registerName());
                return false;
            }
            registerEntry->enabledPhysicalRegister = flag;
        }
    }
    return true;
}

int handleInitHwRegisterType1(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(1), true);
}

int handleInitHwRegisterType2(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(2), true);
}

int handleRemoveHwRegisterType1(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(1), false);
}

int handleRemoveHwRegisterType2(Program *prog, LocTag *sourceLine)
{
    return setRegisterRangeEnabled(sourceLine, GetRP(2), false);
}

int enableAllRegistersInPool(LocTag *sourceLine, RegPool *pool)
{
    if (!pool) {
        reportError("ExecRP_init_reg_all: passed a null pool\n");
        return 0;
    }
    return pool->markAllocatablePhysicalRegistersUsed();
}

int handleInitAllHwRegisterType1(Program *prog, LocTag *sourceLine)
{
    return ExecRP_init_reg_all(GetRP(1));
}

int handleInitAllHwRegisterType2(Program *prog, LocTag *sourceLine)
{
    return ExecRP_init_reg_all(GetRP(2));
}

int alwaysAcceptDirective(Program *prog, LocTag *sourceLine)
{
    return 1;
}

int alwaysRejectDirective(Program *prog, LocTag *sourceLine)
{
    return 0;
}

bool pushVisibilityScope(Program *prog, LocTag *sourceLine, int visibilityMode)
{
    char buf[0x800];
    int left = 0x7ff;
    char *outputCursor = buf;
    for (int tokenIndex = 1; tokenIndex < getLineTokenCount(sourceLine); tokenIndex++) {
        char *tok = sourceLine->isCommentToken(tokenIndex) ? 0 : sourceLine->tokenTextAt(tokenIndex, 0);
        if (!tok)
            continue;
        while (*tok) {
            *outputCursor++ = *tok++;
            if (--left == 0) {
                reportError("ran out of local space for visible list\n");
                return false;
            }
        }
        *outputCursor++ = ':';
    }
    *outputCursor = 0;
    mNode *scopeNode = prog->proc.visibilityScopes.appendNamedNode(buf);
    if (!scopeNode) {
        reportError("failed to process  a visibility directive\n");
        return false;
    }
    scopeNode->flags = visibilityMode;
    return true;
}

inline void ExecRP_show_line(LocTag *sourceLine)
{
    sourceLine->dumpSourceLine(-1);
}

bool popVisibilityScope(Program *prog, LocTag *sourceLine, int visibilityMode)
{
    mNode *scopeNode = prog->proc.visibilityScopes.tail;
    if (!scopeNode) {
        reportWarning("closing visibility directive and no tail at:\n");
        int savedErrorFlag = errorOccurred;
        errorOccurred = !quietDiagnostics;
        ExecRP_show_line(sourceLine);
        errorOccurred = savedErrorFlag;
        return true;
    }
    if (scopeNode->flags != visibilityMode) {
        reportUserMessage("mode mismatch closing a visibility scope\n");
        int savedErrorFlag = errorOccurred;
        errorOccurred = !quietDiagnostics;
        ExecRP_show_line(sourceLine);
        errorOccurred = savedErrorFlag;
        return false;
    }
    if (!prog->proc.visibilityScopes.detachNode(scopeNode)) {
        reportError("failed to remove tail node\n");
        return false;
    }
    delete scopeNode;
    return true;
}
