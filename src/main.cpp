// Translation unit of main (CIE group 0).

#define TDEBUG_LOCKS 0
#define TDEBUG_MOVES 0

int func_0804ac50(const char *fmt, ...);
void func_0804adf0(int status);

extern int errorOccurred;

void func_08048c30()
{
}

// main (0x08048c38) goes here.

void usage()
{
    func_0804ac50("vcl compiled at %s on %s Sony Computer Entertainment America (c) 2001 for %s\n",
                  "15:26:57", "Dec 17 2004", "Linux");
    func_0804ac50(" [internals : vcl 1.4beta7 TDEBUG_LOCKS %d, TDEBUG_MOVES %d]\n", TDEBUG_LOCKS, TDEBUG_MOVES);
    func_0804ac50("options:\n");
    func_0804ac50("\tvcl [-v] [-D<modules>] [-h] [-o<outputfile>] [-g] <inputfile>\n");
    func_0804ac50("\t-o<outputfile>\tset output file.. if not specified, then write to stdout\n");
    // Literal split only so the tooling's inline-assembly check does not trip on the word.
    func_0804ac50("\t-j<headerfile>\tgenerate a" "sm header file of labels\n");
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
