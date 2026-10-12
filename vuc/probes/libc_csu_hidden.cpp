// Experimental historical glibc .init/.fini array loops with hidden symbols.
// No acceptance without correct linked PIC relocations and exact original code.
extern "C" {
typedef void (*StartupHook)();
extern StartupHook __init_array_start[] __attribute__((visibility("hidden")));
extern StartupHook __init_array_end[] __attribute__((visibility("hidden")));
extern StartupHook __fini_array_start[] __attribute__((visibility("hidden")));
extern StartupHook __fini_array_end[] __attribute__((visibility("hidden")));
void _init();
void _fini();

void __libc_csu_init(int, char **, char **)
{
    _init();
    unsigned n = __init_array_end - __init_array_start;
    for (unsigned i = 0; i < n; ++i)
        (*__init_array_start[i])();
}

void __libc_csu_fini()
{
    unsigned n = __fini_array_end - __fini_array_start;
    while (n-- > 0)
        (*__fini_array_start[n])();
    _fini();
}
}
