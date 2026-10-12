/* Period-plausible glibc 2.3.2 ELF startup source, compiled as C.
 * PIC/GOT relocation authentication and full matching are still required.
 */
typedef void (*startup_hook)(void);
extern startup_hook __init_array_start[] __attribute__((visibility("hidden")));
extern startup_hook __init_array_end[] __attribute__((visibility("hidden")));
extern startup_hook __fini_array_start[] __attribute__((visibility("hidden")));
extern startup_hook __fini_array_end[] __attribute__((visibility("hidden")));
extern void _init(void);
extern void _fini(void);

void __libc_csu_init(int argc, char **argv, char **envp)
{
    unsigned int count;
    unsigned int i;
    _init();
    count = __init_array_end - __init_array_start;
    for (i = 0; i < count; ++i)
        (*__init_array_start[i])();
}

void __libc_csu_fini(void)
{
    unsigned int count = __fini_array_end - __fini_array_start;
    while (count-- > 0)
        (*__fini_array_start[count])();
    _fini();
}
