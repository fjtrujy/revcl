// Byte-exact VUC original entry point status flow.
extern bool ParseArguments(int argc, char **argv);
extern void ParseFile();
extern bool error;
int main(int argc, char **argv)
{
    if (!ParseArguments(argc, argv))
        return -1;
    ParseFile();
    if (error) return -2;
    return 0;
}
