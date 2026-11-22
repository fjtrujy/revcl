#define _POSIX_C_SOURCE 200809L
/* Inventory address/offset-based C/C++ identifiers without parsing comments
 * or literals. This is a triage tool, not a complete C++ name resolver. */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct Candidate {
    char name[128];
    const char *kind;
    char location[512];
    unsigned long occurrences;
    unsigned files;
    unsigned lastFile;
};

static struct Candidate *candidates;
static size_t candidateCount, candidateCapacity;
static unsigned fileSerial;

static int allHex(const char *s, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i)
        if (!isxdigit((unsigned char)s[i]))
            return 0;
    return n != 0;
}

static int decimalSuffix(const char *s)
{
    if (!*s)
        return 0;
    for (; *s; ++s)
        if (!isdigit((unsigned char)*s))
            return 0;
    return 1;
}

static const char *classify(const char *name)
{
    size_t n = strlen(name);
    const char *suffix;
    if (n == 13 && strncmp(name, "func_", 5) == 0 && allHex(name + 5, 8))
        return "function";
    if (n == 11 && strncmp(name, "cls", 3) == 0 && allHex(name + 3, 8))
        return "type";
    if (strncmp(name, "unk_", 4) == 0 && n > 4 && allHex(name + 4, n - 4))
        return "offset-field";
    suffix = strrchr(name, '_');
    if (suffix && strlen(suffix + 1) == 8 && allHex(suffix + 1, 8))
        return "address-name";
    if (strstr(name, "zzzz") || strstr(name, "Placeholder") ||
        strstr(name, "Unknown") || strstr(name, "Dummy"))
        return "needs-review";
    if ((strncmp(name, "var_", 4) == 0 && decimalSuffix(name + 4)) ||
        (strncmp(name, "param_", 6) == 0 && decimalSuffix(name + 6)) ||
        (name[0] == 'v' && decimalSuffix(name + 1)) ||
        (name[0] == 'a' && decimalSuffix(name + 1)))
        return "placeholder";
    return NULL;
}

static void record(const char *name, const char *kind, const char *file, unsigned line)
{
    size_t i;
    for (i = 0; i < candidateCount; ++i)
        if (strcmp(candidates[i].name, name) == 0 &&
            strcmp(candidates[i].kind, kind) == 0)
            break;
    if (i == candidateCount) {
        struct Candidate *p;
        if (candidateCount == candidateCapacity) {
            candidateCapacity = candidateCapacity ? candidateCapacity * 2 : 256;
            p = (struct Candidate *)realloc(candidates,
                                               candidateCapacity * sizeof(*candidates));
            if (!p) {
                fputs("Out of memory\n", stderr);
                exit(1);
            }
            candidates = p;
        }
        memset(&candidates[i], 0, sizeof(candidates[i]));
        snprintf(candidates[i].name, sizeof(candidates[i].name), "%s", name);
        candidates[i].kind = kind;
        snprintf(candidates[i].location, sizeof(candidates[i].location),
                 "%s:%u", file, line);
        ++candidateCount;
    }
    ++candidates[i].occurrences;
    if (candidates[i].lastFile != fileSerial) {
        ++candidates[i].files;
        candidates[i].lastFile = fileSerial;
    }
}

static int isCodeFile(const char *name)
{
    size_t n = strlen(name);
    return (n >= 2 && strcmp(name + n - 2, ".h") == 0) ||
           (n >= 2 && strcmp(name + n - 2, ".c") == 0) ||
           (n >= 4 && strcmp(name + n - 4, ".cpp") == 0) ||
           (n >= 4 && strcmp(name + n - 4, ".hpp") == 0);
}

static int unnamedFile(const char *name)
{
    size_t n = strlen(name), stem;
    if (!isCodeFile(name))
        return 0;
    stem = n - (n >= 4 && strcmp(name + n - 4, ".cpp") == 0 ? 4 : 2);
    return (stem == 9 && name[0] == 'f' && allHex(name + 1, 8)) ||
           (stem == 11 && strncmp(name, "cls", 3) == 0 && allHex(name + 3, 8));
}

static void scanFile(const char *path, const char *base)
{
    FILE *f;
    long n;
    char *s;
    size_t i = 0;
    unsigned line = 1;
    ++fileSerial;
    if (unnamedFile(base))
        record(base, "file", path, 1);
    f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) { perror(path); exit(1); }
    s = (char *)malloc((size_t)n + 1);
    if (!s) { fputs("Out of memory\n", stderr); exit(1); }
    if (fread(s, 1, (size_t)n, f) != (size_t)n) { perror(path); exit(1); }
    s[n] = 0;
    fclose(f);
    while (i < (size_t)n) {
        if (s[i] == '/' && i + 1 < (size_t)n && s[i + 1] == '/') {
            while (i < (size_t)n && s[i] != '\n') ++i;
            continue;
        }
        if (s[i] == '/' && i + 1 < (size_t)n && s[i + 1] == '*') {
            i += 2;
            while (i < (size_t)n && !(s[i] == '*' && i + 1 < (size_t)n && s[i + 1] == '/')) {
                if (s[i] == '\n') ++line;
                ++i;
            }
            if (i < (size_t)n) i += 2;
            continue;
        }
        if (s[i] == '"' || s[i] == '\'') {
            char quote = s[i++];
            while (i < (size_t)n) {
                if (s[i] == '\\') { i += 2; continue; }
                if (s[i] == '\n') ++line;
                if (s[i++] == quote) break;
            }
            continue;
        }
        if (isalpha((unsigned char)s[i]) || s[i] == '_') {
            char name[128];
            size_t start = i, len;
            const char *kind;
            do { ++i; } while (i < (size_t)n &&
                (isalnum((unsigned char)s[i]) || s[i] == '_'));
            len = i - start;
            if (len >= sizeof(name)) continue;
            memcpy(name, s + start, len);
            name[len] = 0;
            kind = classify(name);
            if (kind) record(name, kind, path, line);
            continue;
        }
        if (s[i] == '\n') ++line;
        ++i;
    }
    free(s);
}

static void scanDir(const char *dir)
{
    struct dirent **entries;
    int n = scandir(dir, &entries, NULL, alphasort);
    int i;
    if (n < 0) { perror(dir); exit(1); }
    for (i = 0; i < n; ++i) {
        struct dirent *e = entries[i];
        char path[1024];
        struct stat st;
        if (e->d_name[0] != '.' &&
            snprintf(path, sizeof(path), "%s/%s", dir, e->d_name) < (int)sizeof(path)) {
            if (lstat(path, &st) != 0) { perror(path); exit(1); }
            if (S_ISDIR(st.st_mode)) scanDir(path);
            else if (S_ISREG(st.st_mode) && isCodeFile(e->d_name))
                scanFile(path, e->d_name);
        }
        free(e);
    }
    free(entries);
}

static int byKindAndFrequency(const void *aa, const void *bb)
{
    const struct Candidate *a = (const struct Candidate *)aa;
    const struct Candidate *b = (const struct Candidate *)bb;
    int c = strcmp(a->kind, b->kind);
    if (c) return c;
    if (a->occurrences != b->occurrences)
        return a->occurrences < b->occurrences ? 1 : -1;
    return strcmp(a->name, b->name);
}

int main(int argc, char **argv)
{
    int i;
    if (argc < 2) {
        fprintf(stderr, "usage: %s <source-dir> [source-dir...]\n", argv[0]);
        return 2;
    }
    for (i = 1; i < argc; ++i)
        scanDir(argv[i]);
    qsort(candidates, candidateCount, sizeof(*candidates), byKindAndFrequency);
    puts("category\tidentifier\toccurrences\tfiles\tfirst_location");
    for (i = 0; i < (int)candidateCount; ++i)
        printf("%s\t%s\t%lu\t%u\t%s\n", candidates[i].kind,
               candidates[i].name, candidates[i].occurrences,
               candidates[i].files, candidates[i].location);
    free(candidates);
    return 0;
}
