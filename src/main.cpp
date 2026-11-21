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
bool func_0804a2b4(Program *p, bool a, bool b, bool c);
bool func_0804a358(Program *p);
bool func_0804a410(const char *buf);
bool func_0804a608(unsigned char flags);
bool func_0804a61c(unsigned char flags);
bool func_0804a62c(unsigned char mask, char opt);
void func_0804a658(unsigned char *flags);
void func_0804a664(unsigned char *flags);
void func_0804a670(unsigned char *flags);
char *func_0804a67c(unsigned char flags, char *buf, int *len, int argFirst, int argEnd,
                    char **argv, bool keepTemps);

bool func_080603bc(Program *p);
bool func_08062a88(Program *p);
bool func_08063154(Program *p);
bool Prune(Program *p);
bool globalPrune(Program *p);
bool func_08069324(Program *p, bool init);
bool explodeRaw(Program *p);
void func_0806ed78(Program *p, int a);
bool func_0806f0e8(Program *p);
char *emitProgram(Program *p, char *buf, int comments);
char *emitLabels(Program *p, char *buf);
bool loadStalls(Program *p);
bool doLinear(Program *p, int smart);
bool doLoop(Program *p, int smart);
bool doRawAndSynth(Program *p, int smart);
bool relaxOpt(Program *p);
bool func_08079fdc(Program *p, int smart, bool sizeReduce);
int func_0807aa28(Program *p);
bool func_0807aee4(Program *p);
bool bindExternals(Program *p);
bool func_08085a9c(Program *p);
bool buildRegWebs(Program *p);
void dumpThru(Program *p);
bool assignExtRegisters(Program *p);

bool func_08060018(Program *p, bool a);
bool func_0806a74c(Program *p);
bool func_0806ae58(Program *p);
bool detectMoves(Program *p);
void reportUBS(Program *p);
void reportStalls(Program *p);

extern int errorOccurred;
extern char *outName_080b303c;
extern bool quiet_080bbd98;
extern bool g_080bbd99;
extern bool g_080bbd9a;
extern bool stagger_080b33ac;
struct CpuDef;
extern CpuDef cpu_080bbb50;

extern bool symbolic_080b300c;
extern bool flag_080b300d;
extern bool noLoops_080b300e;
extern int timeout_080b3010;
extern bool unk_080b3014;
extern bool sizeReduce_080b3015;
extern bool noAlign_080b3016;
extern bool noEbit_080b3017;
extern bool flag_080b3018;

void func_08048c30()
{
}

int main(int argc, char **argv)
{
    int i = 1;
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

    for (; i < argc && argv[i][0] == '-'; i++) {
        if (func_0804a62c(0xff, argv[i][1]))
            continue;
        switch (argv[i][1]) {
        case 'H':
        case 'h':
            usage();
            func_0804adf0(0);
        case 'o':
            outFile = argv[i] + 2;
            outName_080b303c = outFile;
            break;
        case 'C':
            func_0804acb0("disabling code size reduction pass..\n");
            sizeReduce_080b3015 = false;
            break;
        case 'D':
            dumpRegs = true;
            break;
        case 'G':
            func_0804a658(&gaspFlags);
            break;
        case 'K':
            func_0804acb0("keep temporary files in /tmp enabled (/tmp/vclin. etc\n");
            keepTemps = true;
            break;
        case 'L':
            noLoops_080b300e = true;
            break;
        case 'M':
            flag_080b300d = true;
            func_0804ac7c("timid memory access mode enabled\n");
            break;
        case 'P':
            func_0804ac7c(" prune disabled (not) this options was deprecated, and now is disabled\n");
            break;
        case 'Q':
            g_080bbd99 = true;
            break;
        case 'S':
            func_0804acb0("stagger ram access...\n");
            stagger_080b33ac = true;
            break;
        case 'U':
            uniqueAll = true;
            break;
        case 'Z':
            func_0804ac7c(" disabled immediate fixups - this option is now deprecated and removed\n");
            break;
        case 'c':
            comments = 1;
            break;
        case 'd':
            dumb = true;
            noLoops_080b300e = true;
            break;
        case 'e':
            noEbit_080b3017 = true;
            func_0804acb0("disabling [e] bit setting\n");
            break;
        case 'F':
        case 'f':
            noAlign_080b3016 = true;
            func_0804acb0("will skip align emit\n");
            break;
        case 'A':
        case 'a':
            func_0804acb0("-a option is now on always\n");
            break;
        case 'g':
            if (argv[i][2] == '+')
                func_0804a670(&gaspFlags);
            else {
                if (argv[i][2]) {
                    func_0804ad84("bad command line option '%s' .. only -g+ or -g allowed\n", argv[i]);
                    func_0804adf0(-1);
                }
                func_0804a664(&gaspFlags);
            }
            break;
        case 'j':
            labelFile = argv[i] + 2;
            break;
        case 'm':
            mValue = func_0804add4(argv[i] + 2);
            break;
        case 'n':
            newSyntax = true;
            break;
        case 'p':
            g_080bbd9a = true;
            break;
        case 'q':
            quiet_080bbd98 = true;
            break;
        case 's':
            symbolic_080b300c = true;
            break;
        case 't':
            timeout_080b3010 = atoi(argv[i] + 2);
            if (timeout_080b3010 <= 0)
                timeout_080b3010 = 1;
            break;
        case 'u':
            uniqueName = argv[i] + 2;
            break;
        default:
            func_0804ad38("bad args '%s', try -h for help\n", argv[i]);
            func_0804adf0(-1);
            break;
        case 'W':
        case 'w':
            noPreprocess = true;
            break;
        }
    }

    if (!OS_Init()) {
        func_0804ad38("vcl failed to init OS related things\n");
        func_0804adf0(-1);
    }

    switch (argc - i) {
    case 0:
        func_0804ac50("will try to read stdin\n");
        break;
    case 1:
        inFile = argv[i];
        break;
    default:
        func_0804ac50("bad args (-o now specifies output file..)\n");
        func_0804adf0(-1);
        break;
    }
    if (outFile && inFile && !strcmp(inFile, outFile)) {
        func_0804ac50("can't have infile==outfile\n");
        func_0804adf0(-1);
    }
    if (symbolic_080b300c)
        comments = 2;

    int fd;
    if (inFile) {
        fd = open(inFile, O_RDONLY);
        if (fd == -1) {
            func_0804ad38("failed to open file '%s'\n", inFile);
            func_0804adf0(-1);
        }
    } else {
        inFile = "_stdin_";
        fd = 0;
    }

    char *buf = func_0804aff0(fd, &len, true);
    if (!buf) {
        func_0804ad38("failed to read input file\n");
        func_0804adf0(-1);
    }
    if (!noPreprocess) {
        buf = func_0804a67c(gaspFlags, buf, &len, 1, i, argv, keepTemps);
        if (!buf) {
            func_0804ad38("One of the preprocessing stages failed\n");
            func_0804adf0(-1);
        }
    }
    if (!func_0804a410(buf)) {
        func_0804ad84("Simple buffer check failed.. please read the above USER comments\n");
        func_0804adf0(-1);
    }

    Parser parser;
    if (!parser.func_0804fb94(buf, inFile, func_0804a608(gaspFlags), func_0804a61c(gaspFlags))) {
        func_0804ad38("failed to init parser\n");
        func_0804adf0(-1);
    }
    if (!uniqueName)
        uniqueName = inFile;
    if (uniqueName && strlen(uniqueName) > 1023) {
        func_0804ad84("unique name is too long '%s'\n", uniqueName);
        func_0804adf0(-1);
    }

    Program flow(symbolic_080b300c);
    if (!flow.Init(&cpu_080bbb50, uniqueName, uniqueAll)) {
        func_0804ad38("failed to set int name for flow\n");
        func_0804adf0(-1);
    }
    flow.proc.unk_44 = mValue;
    if (!flow.func_0806efc4(&parser, true, newSyntax)) {
        func_0804ad38("failed to build flow from parser!\n");
        func_0804adf0(-1);
    }
    if (!explodeRaw(&flow)) {
        func_0804ad38("failed to explode raw loops\n");
        func_0804adf0(-1);
    }
    if (!globalPrune(&flow)) {
        func_0804ad38("failed to do a global prune\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, true, false)) {
        func_0804ad38("generic dependency failed\n");
        func_0804adf0(-1);
    }
    if (!func_0806f0e8(&flow)) {
        func_0804ad38("failed to patch some instruction latencies\n");
        func_0804adf0(-1);
    }
    if (!func_08085a9c(&flow)) {
        func_0804ad38("failed to bind dependency info into the stream\n");
        return 0;
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false))
        func_0804adf0(-1);
    if (!Prune(&flow)) {
        func_0804ad38("Failed to prune..\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false)) {
        func_0804ad38("generic dependency code decided to die\n");
        func_0804adf0(-1);
    }
    if (!buildRegWebs(&flow)) {
        func_0804ad38("failed to gather flow\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false)) {
        func_0804ad38("generic dependency code decided to die\n");
        func_0804adf0(-1);
    }
    if (!func_0804a358(&flow)) {
        func_0804ad38("failed to do basic operation processing\n");
        return 0;
    }
    if (dumpRegs)
        dumpThru(&flow);
    if (!assignExtRegisters(&flow)) {
        func_0806ed78(&flow, false);
        func_0804ad38("failed to assign externally visible stream registers\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false)) {
        func_0804ad38("generic dependency code decided to die\n");
        func_0804adf0(-1);
    }
    int smart = dumb = !dumb;
    if (!noLoops_080b300e) {
        if (!func_08069324(&flow, true)) {
            func_0804ad38("failed to initialise immediate fixups\n");
            func_0804adf0(-1);
        }
        if (!func_080603bc(&flow)) {
            func_0804ad38("failed to restat\n");
            func_0804adf0(-1);
        }
        if (!doLoop(&flow, smart)) {
            func_0804ad38("failed to uta loop->raw_loop\n");
            func_0804adf0(-1);
        }
        if (!explodeRaw(&flow)) {
            func_0804ad38("failed to explode raw loops after loop opt\n");
            func_0804adf0(-1);
        }
        if (!func_0804a2b4(&flow, flag_080b300d, false, false)) {
            func_0804ad38("generic dependency failed after loop explode\n");
            func_0804adf0(-1);
        }
        if (!func_08069324(&flow, false)) {
            func_0804ad38("imm fixup failed to patch something\n");
            func_0804adf0(-1);
        }
        if (!Prune(&flow)) {
            func_0804ad38("Failed to prune..\n");
            func_0804adf0(-1);
        }
        if (!func_0804a2b4(&flow, flag_080b300d, false, false))
            func_0804adf0(-1);
        if (!bindExternals(&flow)) {
            func_0804ad38("failed to bind externals\n");
            func_0804adf0(-1);
        }
        if (!buildRegWebs(&flow)) {
            func_0804ad38("failed to gather flow\n");
            func_0804adf0(-1);
        }
        if (!func_0804a2b4(&flow, flag_080b300d, false, false))
            func_0804adf0(-1);
    }
    if (!func_0804a358(&flow))
        func_0804ad38("failed to do basic ops 2\n");
    if (!func_08069324(&flow, true)) {
        func_0804ad38("failed to initialise immediate fixups\n");
        func_0804adf0(-1);
    }
    if (!doRawAndSynth(&flow, smart)) {
        func_0804ad38("failed to process raw tables\n");
        func_0804adf0(-1);
    }
    if (!doLinear(&flow, smart)) {
        func_0804ad38("failed to convert all uta linear->raw\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false))
        func_0804adf0(-1);
    if (!func_08069324(&flow, false)) {
        func_0804ad38("imm fixup failed to patch something\n");
        func_0804adf0(-1);
    }
    if (!func_08063154(&flow)) {
        func_0804ad38("failed to concatenate streams\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false))
        func_0804adf0(-1);
    if (!func_0804a358(&flow)) {
        func_0804ad38("basic operation pass failed\n");
        func_0804adf0(-1);
    }
    if (!relaxOpt(&flow)) {
        func_0804ad38("failed to relax opt\n");
        func_0804adf0(-1);
    }
    if (!func_08069324(&flow, true)) {
        func_0804ad38("imm fixup failed to patch something\n");
        func_0804adf0(-1);
    }
    if (!func_08079fdc(&flow, smart, sizeReduce_080b3015)) {
        func_0804ad38("raw and Synth pass2 failed\n");
        func_0806ed78(&flow, false);
        func_0804adf0(-1);
    }
    if (!func_08062a88(&flow)) {
        func_0804ad38("failed to concatenate streams\n");
        func_0804adf0(-1);
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, false))
        func_0804adf0(-1);
    if (!func_08069324(&flow, false)) {
        func_0804ad38("imm fixup failed to patch something\n");
        func_0804adf0(-1);
    }
    if (sizeReduce_080b3015) {
        int n = func_0807aa28(&flow);
        if (n < 0)
            func_0804ae34("failed to add capping instructions for non-blocking operands, or something\n");
        if (n && !func_0804a2b4(&flow, flag_080b300d, false, false))
            func_0804ae34("after adding capping instructions - failed to chase dependencies\n");
        if (!func_0807aee4(&flow))
            func_0804ae34("failed to remove nops from the sequence\n");
    }
    if (!func_0804a2b4(&flow, flag_080b300d, false, true))
        func_0804adf0(-1);
    if (!loadStalls(&flow)) {
        func_0804ad38("failed to load stalls!\n");
        func_0804adf0(-1);
    }

    char *obuf = new char[10000000];
    if (!obuf) {
        func_0804ad38("failed to get obuffer\n");
        func_0804adf0(-1);
    }
    if (!emitProgram(&flow, obuf, comments)) {
        func_0804ad38("failed to write string\n");
        func_0804adf0(-1);
    }
    char *ojbuf = new char[10000000];
    if (!ojbuf) {
        func_0804ad38("failed to get ojbuffer\n");
        func_0804adf0(-1);
    }
    if (!emitLabels(&flow, ojbuf)) {
        func_0804ad38("failed to write label list\n");
        func_0804adf0(-1);
    }

    if (outFile) {
        int ofd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
        if (ofd == -1) {
            func_0804ad38("failed to create outfile\n");
            func_0804adf0(-1);
        }
        int n = strlen(obuf);
        if (write(ofd, obuf, n) != n) {
            func_0804ad38("failed to write all of out buffer\n");
            close(ofd);
            func_0804adf0(-1);
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
            func_0804ad38("failed to create outfile for labels\n");
            func_0804adf0(-1);
        }
        int n = strlen(ojbuf);
        if (write(ofd, ojbuf, n) != n) {
            func_0804ad38("failed to write all of out buffer\n");
            close(ofd);
            func_0804adf0(-1);
        }
        close(ofd);
    }
    OS_End();
    return 0;
}

void usage()
{
    func_0804ac50("vcl compiled at %s on %s Sony Computer Entertainment America (c) 2001 for %s\n",
                  "15:26:57", "Dec 17 2004", "Linux");
    func_0804ac50(" [internals : vcl 1.4beta7 TDEBUG_LOCKS %d, TDEBUG_MOVES %d]\n", TDEBUG_LOCKS, TDEBUG_MOVES);
    func_0804ac50("options:\n");
    func_0804ac50("\tvcl [-v] [-D<modules>] [-h] [-o<outputfile>] [-g] <inputfile>\n");
    func_0804ac50("\t-o<outputfile>\tset output file.. if not specified, then write to stdout\n");
    func_0804ac50("\t-j<headerfile>\tgenerate asm header file of labels\n");
    func_0804ac50("\t<inputfile>\tinput filename, if not end in vcl, stop. if not specified, read stdin\n");
    func_0804ac50("\t-h\t\tprint this help\n");
    func_0804ac50("\t-t<nseconds>\tset timeout for optimiser to <nseconds>, default is 4,minimum is 1\n");
    func_0804ac50("\t-L\t\tglobally disable loop code generation\n");
    func_0804ac50("\t-G\t\trun cpp over input file before any other processing takes place\n");
    func_0804ac50("\t-g\t\trun ee-gasp over input file for source line info + macro expansion\n");
    func_0804ac50("\t-g+\t\trun ee-gasp over input file for source line info + macro expansion.. with -a as alternate syntax option\n");
    func_0804ac50("\t-c\t\temit (nearly) original source as comments\n");
    func_0804ac50("\t-s\t\temit symbolic names only.\n");
    func_0804ac50("\t-n\t\tenable new synax mode\n");
    func_0804ac50("\t-Z\t\tdisable immediate field fixup code\n");
    func_0804ac50("\t-m\t\twrite out .mpg .dsm stuff as well\n");
    func_0804ac50("\t-S\t\tenabled staggered memory accesses in loop mode\n");
    func_0804ac50("\t-M\t\ttimid memory access mode\n");
    func_0804ac50("\t-f\t     disable alignment directives\n");
    func_0804ac50("\t-e        disable [e]bit in output code.. though I'd suggest using --exitm with no argument\n");
    func_0804ac50(" -u<name>  set unique string for this source to 'name', rather than <inputfile> \n");
    func_0804ac50(" -U        make all but .globals unique names\n");
    func_0804ac50(" -K        keep temporary files..their location is os-dependent - so check vcl output to find them..\n");
    func_0804ac50(" -q        quiet mode, disable all but warnings and error messages\n");
    func_0804ac50(" -p        ms error/warning compatibility mode\n");
    func_0804ac50(" -C\t\tdisable code size reduction pass\n");
    func_0804ac50("unsupported:\n");
    func_0804ac50("   -D        dump register during external allocation state\n");
    func_0804ac50("\t-d\t\tenable only dumb code generation.\n");
    func_0804ac50("\t-P\t\tdisable unused instruction/operand pruning pass.. this might break your code, particularly with size reduction enabled\n");
    func_0804ac50("\t-s\t\tskip register assign phase and only emit symbolic names\n");
    func_0804ac50("\n");
    func_0804adf0(0);
}

void func_0804a2a4()
{
    errorOccurred = 1;
}

static inline void reportErrors(Program *p)
{
    func_0804ad84("vcl thinks it found problem(s) in the input source:\n");
    reportUBS(p);
}

static inline void reportWarnings(Program *p)
{
    func_0804ac7c("vcl thinks it found problem(s) in the input source:\n");
    reportStalls(p);
}

bool func_0804a2b4(Program *p, bool a, bool b, bool c)
{
    if (!func_08060018(p, a)) {
        func_0804ad38("vcl found a bug in itself while chasing dependencies\n");
        return false;
    }
    if (p->proc.unk_68.unk_08) {
        if (!c) {
            reportErrors(p);
            return false;
        }
        func_0804acb0("on the final pass, was detected, but , it should be ok\n");
    }
    if (b && p->proc.unk_74.unk_08)
        reportWarnings(p);
    return true;
}

bool func_0804a358(Program *p)
{
    if (flag_080b3018 && !func_0806a74c(p)) {
        func_0804ad38("failed to collapse instructions\n");
        return false;
    }
    if (!detectMoves(p)) {
        func_0804ad38("failed to detect moves!\n");
        return false;
    }
    if (!func_0804a2b4(p, flag_080b300d, false, false)) {
        func_0804ad38("generic dependency code decided to die\n");
        return false;
    }
    if (!func_0806ae58(p)) {
        func_0804ad38("failed to prune moves\n");
        return false;
    }
    if (!func_0804a2b4(p, flag_080b300d, false, false))
        return false;
    return true;
}
