// EXPERIMENTAL C++03 reconstruction of original VUC ParseArguments branches.
// Original ELF evidence: 0x0805b3e0, supplied output path at +0xca,
// implicit output path at +0x1a0; NOT an accepted byte-exact function.
#include <iostream>
#include <cstring>
extern char inputFileName[256];
extern char outputFileName[256];
extern char inputPath[256];
extern bool addSourceComments;
extern bool reduceConstantExpressions;
extern bool noWarnings;
extern bool error;

int ParseArguments(int argc, char **argv)
{
    if (argc <= 1) {
        std::cout << "VUC 2004.1 - VU compiler that generates a VCL file from a C like syntax.\n"
                     "(c)2004 Ola Olsson, Peter Pettersson\n"
                     "http://www.playstation2-linux.com/projects/vuc" << std::endl;
        return 0;
    }
    int i = 1;
    int options = 0;
    for (; i < argc; ++i) {
        char *arg = argv[i];
        if (arg[0] != '-' && arg[0] != '/') break;
        if (std::strcmp(arg + 1, "comments") == 0 || std::strcmp(arg + 1, "c") == 0)
            addSourceComments = true;
        else if (std::strcmp(arg + 1, "reduce") == 0)
            reduceConstantExpressions = true;
        else if (std::strcmp(arg + 1, "nowarnings") == 0 || std::strcmp(arg + 1, "nw") == 0)
            noWarnings = true;
        else {
            error = true;
            std::cout << "Error: Invalid input argument.\n";
        }
        ++options;
    }
    if (options + 1 >= argc) {
        error = true;
        std::cout << "Error: Input file name missing.\n";
        return 0;
    }
    char *source = argv[options + 1];
    if (std::strlen(source) > 255) {
        error = true;
        std::cout << "Error: Output file name too long.\n";
        return 0;
    }
    std::strcpy(inputFileName, source);
    if (options + 3 < argc) {
        char *destination = argv[options + 2];
        if (std::strlen(destination) > 255) {
            error = true;
            std::cout << "Error: Output file name too long.\n";
            return 0;
        }
        std::strcpy(outputFileName, destination);
        char *slash = std::strrchr(inputFileName, '/');
        if (!slash) slash = std::strrchr(inputFileName, '\\');
        if (slash) {
            unsigned size = (unsigned)(slash - inputFileName + 1);
            std::memcpy(inputPath, inputFileName, size);
            inputPath[size] = 0;
        }
    } else {
        std::strcpy(outputFileName, source);
        char *ext = std::strrchr(outputFileName, '.');
        if (ext) std::strcpy(ext, ".vcl");
        else std::strcpy(outputFileName + std::strlen(outputFileName), ".vcl");
        if (std::strcmp(inputFileName, outputFileName) == 0) {
            error = true;
            std::cout << "Error: Input and output file the same.\n";
            return 0;
        }
    }
    return 1;
}
