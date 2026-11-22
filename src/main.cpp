// Translation unit of main (CIE group 0).

#define TDEBUG_LOCKS 0
#define TDEBUG_MOVES 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "os.h"
#include "Program.h"
#include "Parser.h"

void usage();
bool validateFlowDependenciesAndHazards(Program *p, bool a, bool b, bool c);
bool analyzeAndPropagateMoves(Program *p);
bool warnAboutMissingPreprocessing(const char *buf);
bool usesGaspPreprocessor(unsigned char flags);
bool usesGccPreprocessor(unsigned char flags);
bool matchesToolOption(unsigned char mask, char opt);
void enableGccPreprocessing(unsigned char *flags);
void enableGaspPreprocessing(unsigned char *flags);
void enableAlternateGaspPreprocessing(unsigned char *flags);
char *runSelectedPreprocessors(unsigned char flags, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

bool gatherAllStreamStatistics(Program *p);
bool tidyProgramStreams(Program *p);
bool concatenateCompatibleStreams(Program *p);
bool Prune(Program *p);
bool globalPrune(Program *p);
bool resolveProgramImmediateFixups(Program *p, bool init);
bool explodeRaw(Program *p);
void dumpProgramStreamsAndRegisters(Program *p, int a);
bool patchAllStreamLatencies(Program *p);
char *emitProgram(Program *p, char *buf, int comments);
char *emitLabels(Program *p, char *buf);
bool loadStalls(Program *p);
bool doLinear(Program *p, int smart);
bool doLoop(Program *p, int smart);
bool doRawAndSynth(Program *p, int smart);
bool relaxOpt(Program *p);
bool optimizeProgramScheduling(Program *p, int smart, bool sizeReduce);
int insertAllRegisterCaps(Program *p);
bool prepareExternalSchedulingForProgram(Program *p);
bool bindExternals(Program *p);
bool noopFlowBuildHook(Program *p);
bool buildRegWebs(Program *p);
void dumpThru(Program *p);
bool assignExtRegisters(Program *p);

bool rebuildFlowDependencies(Program *p, bool a);
bool collapseProgramStreams(Program *p);
bool noopMovePropagationHook(Program *p);
bool detectMoves(Program *p);
void reportUBS(Program *p);
void reportStalls(Program *p);

extern int errorOccurred;
extern char *currentOutputFileName;
extern bool quietDiagnostics;
extern bool unhandledOptionQSeen;
extern bool unhandledOptionPSeen;
extern bool staggerMemoryAccess;
struct CpuDef;
extern CpuDef vuProcessorDefinition;

extern bool symbolicOutputRequested;
extern bool cautiousMemoryAccessMode;
extern bool disableLoopProcessing;
extern int optimizationTimeoutSeconds;
extern bool unk_080b3014;
extern bool enableCodeSizeReduction;
extern bool disableOutputAlignment;
extern bool disableEndBits;
extern bool enableInstructionCollapse;

void noopMainStartupHook()
{
}

int main(int argc, char **argv)
{
    int argumentIndex = 1;
    char *inFile = 0;
    char *outFile = 0;
    char *labelFile = 0;
    bool dumb = false;
    int comments = 0;
    unsigned char gaspFlags = 0;
    bool keepTemps = false;
    bool newSyntax = false;
    bool noPreprocess = false;
    const char *uniqueName = 0;
    bool uniqueAll = false;
    bool dumpRegs = false;
    int mValue = -1;
    int len;

    for (; argumentIndex < argc && argv[argumentIndex][0] == '-'; argumentIndex++) {
        if (matchesToolOption(0xff, argv[argumentIndex][1]))
            continue;
        switch (argv[argumentIndex][1]) {
        case 'H':
        case 'h':
            usage();
            exitAfterCleanup(0);
        case 'o':
            outFile = argv[argumentIndex] + 2;
            currentOutputFileName = outFile;
            break;
        case 'C':
            reportInfo("disabling code size reduction pass..\n");
            enableCodeSizeReduction = false;
            break;
        case 'D':
            dumpRegs = true;
            break;
        case 'G':
            enableGccPreprocessing(&gaspFlags);
            break;
        case 'K':
            reportInfo("keep temporary files in /tmp enabled (/tmp/vclin. etc\n");
            keepTemps = true;
            break;
        case 'L':
            disableLoopProcessing = true;
            break;
        case 'M':
            cautiousMemoryAccessMode = true;
            reportWarning("timid memory access mode enabled\n");
            break;
        case 'P':
            reportWarning(" prune disabled (not) this options was deprecated, and now is disabled\n");
            break;
        case 'Q':
            unhandledOptionQSeen = true;
            break;
        case 'S':
            reportInfo("stagger ram access...\n");
            staggerMemoryAccess = true;
            break;
        case 'U':
            uniqueAll = true;
            break;
        case 'Z':
            reportWarning(" disabled immediate fixups - this option is now deprecated and removed\n");
            break;
        case 'c':
            comments = 1;
            break;
        case 'd':
            dumb = true;
            disableLoopProcessing = true;
            break;
        case 'e':
            disableEndBits = true;
            reportInfo("disabling [e] bit setting\n");
            break;
        case 'F':
        case 'f':
            disableOutputAlignment = true;
            reportInfo("will skip align emit\n");
            break;
        case 'A':
        case 'a':
            reportInfo("-a option is now on always\n");
            break;
        case 'g':
            if (argv[argumentIndex][2] == '+')
                enableAlternateGaspPreprocessing(&gaspFlags);
            else {
                if (argv[argumentIndex][2]) {
                    reportUserMessage("bad command line option '%s' .. only -g+ or -g allowed\n", argv[argumentIndex]);
                    exitAfterCleanup(-1);
                }
                enableGaspPreprocessing(&gaspFlags);
            }
            break;
        case 'j':
            labelFile = argv[argumentIndex] + 2;
            break;
        case 'm':
            mValue = parseInteger(argv[argumentIndex] + 2);
            break;
        case 'n':
            newSyntax = true;
            break;
        case 'p':
            unhandledOptionPSeen = true;
            break;
        case 'q':
            quietDiagnostics = true;
            break;
        case 's':
            symbolicOutputRequested = true;
            break;
        case 't':
            optimizationTimeoutSeconds = atoi(argv[argumentIndex] + 2);
            if (optimizationTimeoutSeconds <= 0)
                optimizationTimeoutSeconds = 1;
            break;
        case 'u':
            uniqueName = argv[argumentIndex] + 2;
            break;
        default:
            reportError("bad args '%s', try -h for help\n", argv[argumentIndex]);
            exitAfterCleanup(-1);
            break;
        case 'W':
        case 'w':
            noPreprocess = true;
            break;
        }
    }

    if (!OS_Init()) {
        reportError("vcl failed to init OS related things\n");
        exitAfterCleanup(-1);
    }

    switch (argc - argumentIndex) {
    case 0:
        reportMessage("will try to read stdin\n");
        break;
    case 1:
        inFile = argv[argumentIndex];
        break;
    default:
        reportMessage("bad args (-o now specifies output file..)\n");
        exitAfterCleanup(-1);
        break;
    }
    if (outFile && inFile && !strcmp(inFile, outFile)) {
        reportMessage("can't have infile==outfile\n");
        exitAfterCleanup(-1);
    }
    if (symbolicOutputRequested)
        comments = 2;

    int inputDescriptor;
    if (inFile) {
        inputDescriptor = open(inFile, O_RDONLY);
        if (inputDescriptor == -1) {
            reportError("failed to open file '%s'\n", inFile);
            exitAfterCleanup(-1);
        }
    } else {
        inFile = "_stdin_";
        inputDescriptor = 0;
    }

    char *buf = readDescriptorIntoBuffer(inputDescriptor, &len, true);
    if (!buf) {
        reportError("failed to read input file\n");
        exitAfterCleanup(-1);
    }
    if (!noPreprocess) {
        buf = runSelectedPreprocessors(gaspFlags, buf, &len, 1, argumentIndex, argv, keepTemps);
        if (!buf) {
            reportError("One of the preprocessing stages failed\n");
            exitAfterCleanup(-1);
        }
    }
    if (!warnAboutMissingPreprocessing(buf)) {
        reportUserMessage("Simple buffer check failed.. please read the above USER comments\n");
        exitAfterCleanup(-1);
    }

    Parser parser;
    if (!parser.initializeParser(buf, inFile, usesGaspPreprocessor(gaspFlags), usesGccPreprocessor(gaspFlags))) {
        reportError("failed to init parser\n");
        exitAfterCleanup(-1);
    }
    if (!uniqueName)
        uniqueName = inFile;
    if (uniqueName && strlen(uniqueName) > 1023) {
        reportUserMessage("unique name is too long '%s'\n", uniqueName);
        exitAfterCleanup(-1);
    }

    Program flow(symbolicOutputRequested);
    if (!flow.Init(&vuProcessorDefinition, uniqueName, uniqueAll)) {
        reportError("failed to set int name for flow\n");
        exitAfterCleanup(-1);
    }
    flow.proc.mpgTargetAddress = mValue;
    if (!flow.decodeAndSplitStreams(&parser, true, newSyntax)) {
        reportError("failed to build flow from parser!\n");
        exitAfterCleanup(-1);
    }
    if (!explodeRaw(&flow)) {
        reportError("failed to explode raw loops\n");
        exitAfterCleanup(-1);
    }
    if (!globalPrune(&flow)) {
        reportError("failed to do a global prune\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, true, false)) {
        reportError("generic dependency failed\n");
        exitAfterCleanup(-1);
    }
    if (!patchAllStreamLatencies(&flow)) {
        reportError("failed to patch some instruction latencies\n");
        exitAfterCleanup(-1);
    }
    if (!noopFlowBuildHook(&flow)) {
        reportError("failed to bind dependency info into the stream\n");
        return 0;
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
        exitAfterCleanup(-1);
    if (!Prune(&flow)) {
        reportError("Failed to prune..\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false)) {
        reportError("generic dependency code decided to die\n");
        exitAfterCleanup(-1);
    }
    if (!buildRegWebs(&flow)) {
        reportError("failed to gather flow\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false)) {
        reportError("generic dependency code decided to die\n");
        exitAfterCleanup(-1);
    }
    if (!analyzeAndPropagateMoves(&flow)) {
        reportError("failed to do basic operation processing\n");
        return 0;
    }
    if (dumpRegs)
        dumpThru(&flow);
    if (!assignExtRegisters(&flow)) {
        dumpProgramStreamsAndRegisters(&flow, false);
        reportError("failed to assign externally visible stream registers\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false)) {
        reportError("generic dependency code decided to die\n");
        exitAfterCleanup(-1);
    }
    int smart = dumb = !dumb;
    if (!disableLoopProcessing) {
        if (!resolveProgramImmediateFixups(&flow, true)) {
            reportError("failed to initialise immediate fixups\n");
            exitAfterCleanup(-1);
        }
        if (!gatherAllStreamStatistics(&flow)) {
            reportError("failed to restat\n");
            exitAfterCleanup(-1);
        }
        if (!doLoop(&flow, smart)) {
            reportError("failed to uta loop->raw_loop\n");
            exitAfterCleanup(-1);
        }
        if (!explodeRaw(&flow)) {
            reportError("failed to explode raw loops after loop opt\n");
            exitAfterCleanup(-1);
        }
        if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false)) {
            reportError("generic dependency failed after loop explode\n");
            exitAfterCleanup(-1);
        }
        if (!resolveProgramImmediateFixups(&flow, false)) {
            reportError("imm fixup failed to patch something\n");
            exitAfterCleanup(-1);
        }
        if (!Prune(&flow)) {
            reportError("Failed to prune..\n");
            exitAfterCleanup(-1);
        }
        if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
            exitAfterCleanup(-1);
        if (!bindExternals(&flow)) {
            reportError("failed to bind externals\n");
            exitAfterCleanup(-1);
        }
        if (!buildRegWebs(&flow)) {
            reportError("failed to gather flow\n");
            exitAfterCleanup(-1);
        }
        if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
            exitAfterCleanup(-1);
    }
    if (!analyzeAndPropagateMoves(&flow))
        reportError("failed to do basic ops 2\n");
    if (!resolveProgramImmediateFixups(&flow, true)) {
        reportError("failed to initialise immediate fixups\n");
        exitAfterCleanup(-1);
    }
    if (!doRawAndSynth(&flow, smart)) {
        reportError("failed to process raw tables\n");
        exitAfterCleanup(-1);
    }
    if (!doLinear(&flow, smart)) {
        reportError("failed to convert all uta linear->raw\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
        exitAfterCleanup(-1);
    if (!resolveProgramImmediateFixups(&flow, false)) {
        reportError("imm fixup failed to patch something\n");
        exitAfterCleanup(-1);
    }
    if (!concatenateCompatibleStreams(&flow)) {
        reportError("failed to concatenate streams\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
        exitAfterCleanup(-1);
    if (!analyzeAndPropagateMoves(&flow)) {
        reportError("basic operation pass failed\n");
        exitAfterCleanup(-1);
    }
    if (!relaxOpt(&flow)) {
        reportError("failed to relax opt\n");
        exitAfterCleanup(-1);
    }
    if (!resolveProgramImmediateFixups(&flow, true)) {
        reportError("imm fixup failed to patch something\n");
        exitAfterCleanup(-1);
    }
    if (!optimizeProgramScheduling(&flow, smart, enableCodeSizeReduction)) {
        reportError("raw and Synth pass2 failed\n");
        dumpProgramStreamsAndRegisters(&flow, false);
        exitAfterCleanup(-1);
    }
    if (!tidyProgramStreams(&flow)) {
        reportError("failed to concatenate streams\n");
        exitAfterCleanup(-1);
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
        exitAfterCleanup(-1);
    if (!resolveProgramImmediateFixups(&flow, false)) {
        reportError("imm fixup failed to patch something\n");
        exitAfterCleanup(-1);
    }
    if (enableCodeSizeReduction) {
        int insertedCapCount = insertAllRegisterCaps(&flow);
        if (insertedCapCount < 0)
            exitWithError("failed to add capping instructions for non-blocking operands, or something\n");
        if (insertedCapCount && !validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, false))
            exitWithError("after adding capping instructions - failed to chase dependencies\n");
        if (!prepareExternalSchedulingForProgram(&flow))
            exitWithError("failed to remove nops from the sequence\n");
    }
    if (!validateFlowDependenciesAndHazards(&flow, cautiousMemoryAccessMode, false, true))
        exitAfterCleanup(-1);
    if (!loadStalls(&flow)) {
        reportError("failed to load stalls!\n");
        exitAfterCleanup(-1);
    }

    char *obuf = new char[10000000];
    if (!obuf) {
        reportError("failed to get obuffer\n");
        exitAfterCleanup(-1);
    }
    if (!emitProgram(&flow, obuf, comments)) {
        reportError("failed to write string\n");
        exitAfterCleanup(-1);
    }
    char *ojbuf = new char[10000000];
    if (!ojbuf) {
        reportError("failed to get ojbuffer\n");
        exitAfterCleanup(-1);
    }
    if (!emitLabels(&flow, ojbuf)) {
        reportError("failed to write label list\n");
        exitAfterCleanup(-1);
    }

    if (outFile) {
        int ofd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
        if (ofd == -1) {
            reportError("failed to create outfile\n");
            exitAfterCleanup(-1);
        }
        int outputLength = strlen(obuf);
        if (write(ofd, obuf, outputLength) != outputLength) {
            reportError("failed to write all of out buffer\n");
            close(ofd);
            exitAfterCleanup(-1);
        }
        close(ofd);
    } else {
        puts(";-------------------------");
        puts(";-------------------------");
        puts(";-----VCL CODE------------");
        puts(";-------------------------");
        puts(";-------------------------");
        printf("%s", obuf);
        puts(";-------------------------");
        puts(";-------------------------");
        puts(";-------------------------");
        puts(";-------------------------");
        puts(";-------------------------");
    }
    if (labelFile) {
        int ofd = open(labelFile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
        if (ofd == -1) {
            reportError("failed to create outfile for labels\n");
            exitAfterCleanup(-1);
        }
        int labelOutputLength = strlen(ojbuf);
        if (write(ofd, ojbuf, labelOutputLength) != labelOutputLength) {
            reportError("failed to write all of out buffer\n");
            close(ofd);
            exitAfterCleanup(-1);
        }
        close(ofd);
    }
    OS_End();
    return 0;
}

void usage()
{
    reportMessage("vcl compiled at %s on %s Sony Computer Entertainment America (c) 2001 for %s\n",
                  "15:26:57", "Dec 17 2004", "Linux");
    reportMessage(" [internals : vcl 1.4beta7 TDEBUG_LOCKS %d, TDEBUG_MOVES %d]\n", TDEBUG_LOCKS, TDEBUG_MOVES);
    reportMessage("options:\n");
    reportMessage("\tvcl [-v] [-D<modules>] [-h] [-o<outputfile>] [-g] <inputfile>\n");
    reportMessage("\t-o<outputfile>\tset output file.. if not specified, then write to stdout\n");
    reportMessage("\t-j<headerfile>\tgenerate asm header file of labels\n");
    reportMessage("\t<inputfile>\tinput filename, if not end in vcl, stop. if not specified, read stdin\n");
    reportMessage("\t-h\t\tprint this help\n");
    reportMessage("\t-t<nseconds>\tset timeout for optimiser to <nseconds>, default is 4,minimum is 1\n");
    reportMessage("\t-L\t\tglobally disable loop code generation\n");
    reportMessage("\t-G\t\trun cpp over input file before any other processing takes place\n");
    reportMessage("\t-g\t\trun ee-gasp over input file for source line info + macro expansion\n");
    reportMessage("\t-g+\t\trun ee-gasp over input file for source line info + macro expansion.. with -a as alternate syntax option\n");
    reportMessage("\t-c\t\temit (nearly) original source as comments\n");
    reportMessage("\t-s\t\temit symbolic names only.\n");
    reportMessage("\t-n\t\tenable new synax mode\n");
    reportMessage("\t-Z\t\tdisable immediate field fixup code\n");
    reportMessage("\t-m\t\twrite out .mpg .dsm stuff as well\n");
    reportMessage("\t-S\t\tenabled staggered memory accesses in loop mode\n");
    reportMessage("\t-M\t\ttimid memory access mode\n");
    reportMessage("\t-f\t     disable alignment directives\n");
    reportMessage("\t-e        disable [e]bit in output code.. though I'd suggest using --exitm with no argument\n");
    reportMessage(" -u<name>  set unique string for this source to 'name', rather than <inputfile> \n");
    reportMessage(" -U        make all but .globals unique names\n");
    reportMessage(" -K        keep temporary files..their location is os-dependent - so check vcl output to find them..\n");
    reportMessage(" -q        quiet mode, disable all but warnings and error messages\n");
    reportMessage(" -p        ms error/warning compatibility mode\n");
    reportMessage(" -C\t\tdisable code size reduction pass\n");
    reportMessage("unsupported:\n");
    reportMessage("   -D        dump register during external allocation state\n");
    reportMessage("\t-d\t\tenable only dumb code generation.\n");
    reportMessage("\t-P\t\tdisable unused instruction/operand pruning pass.. this might break your code, particularly with size reduction enabled\n");
    reportMessage("\t-s\t\tskip register assign phase and only emit symbolic names\n");
    reportMessage("\n");
    exitAfterCleanup(0);
}

void markInputError()
{
    errorOccurred = 1;
}

static inline void reportErrors(Program *program)
{
    reportUserMessage("vcl thinks it found problem(s) in the input source:\n");
    reportUBS(program);
}

static inline void reportWarnings(Program *program)
{
    reportWarning("vcl thinks it found problem(s) in the input source:\n");
    reportStalls(program);
}

bool validateFlowDependenciesAndHazards(Program *program, bool forceDependencyChecks, bool reportStalls, bool allowFinalPassUBS)
{
    if (!rebuildFlowDependencies(program, forceDependencyChecks)) {
        reportError("vcl found a bug in itself while chasing dependencies\n");
        return false;
    }
    if (program->proc.usedBeforeSet.count) {
        if (!allowFinalPassUBS) {
            reportErrors(program);
            return false;
        }
        reportInfo("on the final pass, was detected, but , it should be ok\n");
    }
    if (reportStalls && program->proc.stalls.count)
        reportWarnings(program);
    return true;
}

bool analyzeAndPropagateMoves(Program *program)
{
    if (enableInstructionCollapse && !collapseProgramStreams(program)) {
        reportError("failed to collapse instructions\n");
        return false;
    }
    if (!detectMoves(program)) {
        reportError("failed to detect moves!\n");
        return false;
    }
    if (!validateFlowDependenciesAndHazards(program, cautiousMemoryAccessMode, false, false)) {
        reportError("generic dependency code decided to die\n");
        return false;
    }
    if (!noopMovePropagationHook(program)) {
        reportError("failed to prune moves\n");
        return false;
    }
    if (!validateFlowDependenciesAndHazards(program, cautiousMemoryAccessMode, false, false))
        return false;
    return true;
}
