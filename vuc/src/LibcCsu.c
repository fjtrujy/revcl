/* Source-level reconstruction of the original glibc startup unit.
 * Both __libc_csu_init and __libc_csu_fini match original i386 bytes.
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
    unsigned int i = 0;
    _init();
    const unsigned int count = __init_array_end - __init_array_start;
    while (i < count) {
        (*__init_array_start[i])();
        ++i;
    }
}

void __libc_csu_fini(void)
{
    unsigned int count = __fini_array_end - __fini_array_start;
    while (count-- > 0)
        (*__fini_array_start[count])();
    _fini();
}
