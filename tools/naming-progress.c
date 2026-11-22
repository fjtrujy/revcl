/*
 * Summarize the VCL naming review queues into a stable, commit-friendly report.
 * The lexical scanner handles opaque tokens; AST review queues and manually
 * flagged provisional types are separate inputs. No Python is required.
 */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

enum Metric {
    FILES,
    CLASSES_UNALIASED,
    CLASSES_MANUAL,
    STRUCTS_MANUAL,
    FUNCTIONS_UNALIASED,
    FIELDS,
    PARAMETERS,
    LOCALS,
    ADDRESS_NAMES,
    NUMERIC_PLACEHOLDERS,
    METRIC_COUNT
};

static const char *metricKeys[METRIC_COUNT] = {
    "files", "classes_unaliased", "classes_provisional",
    "structs_provisional", "functions_unaliased", "fields",
    "parameters", "locals", "address_names", "numeric_placeholders"
};

static const char *metricLabels[METRIC_COUNT] = {
    "Files with address-based filenames",
    "Classes with numeric names and no source alias",
    "Classes with provisional names (manual review)",
    "Structs with provisional names (manual review)",
    "Functions with numeric names and no source alias",
    "Fields with offset names (owner-qualified)",
    "Parameters with short/generic names (AST review)",
    "Local variables with short/generic names (AST review)",
    "Address-suffixed globals/other identifiers",
    "Explicit numeric placeholder identifiers"
};

struct OpaqueName {
    char name[128];
    int aliased;
};

struct Inventory {
    long metrics[METRIC_COUNT];
    struct OpaqueName functions[4096];
    int functionCount;
    struct OpaqueName classes[256];
    int classCount;
    int functionAliases;
    int classAliases;
};

static void fail(const char *reason, const char *path)
{
    fprintf(stderr, "naming-progress: %s: %s\n", reason, path);
    exit(1);
}

static FILE *openInput(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        fail(strerror(errno), path);
    return f;
}

static int splitTabs(char *line, char **parts, int capacity)
{
    int n = 0;
    char *start = line;
    char *p;
    for (p = line; ; ++p) {
        if (*p != '\t' && *p != '\n' && *p != '\0')
            continue;
        if (n >= capacity)
            break;
        parts[n++] = start;
        if (*p == '\t') {
            *p = '\0';
            start = p + 1;
        } else {
            *p = '\0';
            break;
        }
    }
    return n;
}

static void saveName(struct OpaqueName *names, int *count, int capacity,
                     const char *name)
{
    int i;
    if (strlen(name) >= sizeof(names[0].name))
        fail("identifier too long", name);
    for (i = 0; i < *count; ++i)
        if (!strcmp(names[i].name, name))
            return;
    if (*count == capacity)
        fail("too many candidates", name);
    strcpy(names[*count].name, name);
    names[*count].aliased = 0;
    ++*count;
}

static void readLexical(const char *path, struct Inventory *inv)
{
    FILE *f = openInput(path);
    char line[2048], *parts[8];
    if (!fgets(line, sizeof(line), f))
        fail("missing header", path);
    while (fgets(line, sizeof(line), f)) {
        int n = splitTabs(line, parts, 8);
        if (n < 5)
            fail("malformed identifier row", path);
        if (!strcmp(parts[0], "file"))
            ++inv->metrics[FILES];
        else if (!strcmp(parts[0], "function"))
            saveName(inv->functions, &inv->functionCount, 4096, parts[1]);
        else if (!strcmp(parts[0], "type"))
            saveName(inv->classes, &inv->classCount, 256, parts[1]);
        else if (!strcmp(parts[0], "address-name"))
            ++inv->metrics[ADDRESS_NAMES];
        else if (!strcmp(parts[0], "placeholder"))
            ++inv->metrics[NUMERIC_PLACEHOLDERS];
    }
    fclose(f);
}

static void countRows(const char *path, struct Inventory *inv, int declarations)
{
    FILE *f = openInput(path);
    char line[2048], *parts[8];
    if (!fgets(line, sizeof(line), f))
        fail("missing header", path);
    while (fgets(line, sizeof(line), f)) {
        int n = splitTabs(line, parts, 8);
        if (n < (declarations ? 6 : 3))
            fail("malformed declaration row", path);
        if (declarations) {
            struct stat st;
            if (stat(parts[2], &st) != 0 || !S_ISREG(st.st_mode))
                fail("AST queue points to a missing source file", parts[2]);
            if (!strcmp(parts[0], "parameter_review"))
                ++inv->metrics[PARAMETERS];
            else if (!strcmp(parts[0], "local_review"))
                ++inv->metrics[LOCALS];
        } else if (!strcmp(parts[0], "opaque_field")) {
            ++inv->metrics[FIELDS];
        }
    }
    fclose(f);
}

static int isIdentChar(int c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static int hasTypeDeclaration(const char *path, const char *kind,
                              const char *name)
{
    FILE *f = openInput(path);
    char line[2048];
    size_t kindLen = strlen(kind), nameLen = strlen(name);
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        const char *p = line;
        while ((p = strstr(p, kind)) != NULL) {
            const char *q = p + kindLen;
            if ((p == line || !isIdentChar(p[-1])) &&
                isspace((unsigned char)*q)) {
                while (isspace((unsigned char)*q))
                    ++q;
                if (!strncmp(q, name, nameLen) &&
                    !isIdentChar(q[nameLen])) {
                    found = 1;
                    break;
                }
            }
            ++p;
        }
        if (found)
            break;
    }
    fclose(f);
    return found;
}

/* Only count curated manual candidates that remain flagged for review. */
static void readManual(const char *path, struct Inventory *inv)
{
    FILE *f = openInput(path);
    char line[2048], *parts[8];
    if (!fgets(line, sizeof(line), f))
        fail("missing header", path);
    while (fgets(line, sizeof(line), f)) {
        int n = splitTabs(line, parts, 8);
        if (n < 5)
            fail("malformed manual review row", path);
        if (strcmp(parts[4], "open") && strcmp(parts[4], "resolved"))
            fail("expected open/resolved status", parts[1]);
        if (strcmp(parts[4], "resolved") == 0)
            continue;
        if (!hasTypeDeclaration(parts[2], parts[0], parts[1]))
            fail("manual queue has no matching source type", parts[1]);
        if (!strcmp(parts[0], "class"))
            ++inv->metrics[CLASSES_MANUAL];
        else if (!strcmp(parts[0], "struct"))
            ++inv->metrics[STRUCTS_MANUAL];
        else
            fail("invalid manual category", parts[0]);
    }
    fclose(f);
}

static int markAlias(struct OpaqueName *names, int count, const char *target)
{
    int i;
    for (i = 0; i < count; ++i) {
        if (!strcmp(names[i].name, target) && !names[i].aliased) {
            names[i].aliased = 1;
            return 1;
        }
    }
    return 0;
}

static int isSourceFile(const char *name)
{
    size_t n = strlen(name);
    return (n > 2 && !strcmp(name + n - 2, ".h")) ||
           (n > 2 && !strcmp(name + n - 2, ".c")) ||
           (n > 4 && (!strcmp(name + n - 4, ".hpp") ||
                      !strcmp(name + n - 4, ".cpp")));
}

/* Alias declarations are intentionally counted, not ABI-visible renames. */
static void scanAliases(const char *dir, struct Inventory *inv)
{
    DIR *d = opendir(dir);
    struct dirent *entry;
    if (!d)
        fail(strerror(errno), dir);
    while ((entry = readdir(d)) != NULL) {
        char path[1024];
        FILE *f;
        char line[2048], word1[128], word2[128];
        if (!isSourceFile(entry->d_name))
            continue;
        if (snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name) >=
            (int)sizeof(path))
            fail("path too long", entry->d_name);
        f = openInput(path);
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, " #define %127s %127s", word1, word2) == 2 &&
                strncmp(word1, "func_", 5)) {
                inv->functionAliases += markAlias(
                    inv->functions, inv->functionCount, word2);
            }
            if (sscanf(line, " typedef %127s %127s", word1, word2) == 2) {
                inv->classAliases += markAlias(
                    inv->classes, inv->classCount, word1);
            }
        }
        fclose(f);
    }
    closedir(d);
    inv->metrics[CLASSES_UNALIASED] =
        inv->classCount - inv->classAliases;
    inv->metrics[FUNCTIONS_UNALIASED] =
        inv->functionCount - inv->functionAliases;
}

static void readBaseline(const char *path, long baseline[METRIC_COUNT])
{
    FILE *f = openInput(path);
    char line[512], *parts[3], *end;
    int i, seen[METRIC_COUNT] = {0};
    if (!fgets(line, sizeof(line), f))
        fail("missing header", path);
    while (fgets(line, sizeof(line), f)) {
        int n = splitTabs(line, parts, 3);
        if (n < 2)
            fail("malformed baseline row", path);
        for (i = 0; i < METRIC_COUNT; ++i)
            if (!strcmp(parts[0], metricKeys[i]))
                break;
        if (i == METRIC_COUNT || seen[i])
            fail("unknown/duplicate baseline metric", parts[0]);
        errno = 0;
        baseline[i] = strtol(parts[1], &end, 10);
        if (errno || *end || baseline[i] < 0)
            fail("invalid baseline count", parts[0]);
        seen[i] = 1;
    }
    fclose(f);
    for (i = 0; i < METRIC_COUNT; ++i)
        if (!seen[i])
            fail("missing baseline count", metricKeys[i]);
}

static void printBaseline(const struct Inventory *inv)
{
    int i;
    puts("metric\tcount");
    for (i = 0; i < METRIC_COUNT; ++i)
        printf("%s\t%ld\n", metricKeys[i], inv->metrics[i]);
}

static void printReport(const struct Inventory *inv,
                        const long baseline[METRIC_COUNT])
{
    int i;
    puts("# Naming progress (VCL branch)\n");
    puts("Tracked naming-review snapshot. The **baseline** is the pre-refactor "
         "`main` source at commit `00bf1a1b` (22 November 2026); its "
         "lexical counts were re-scanned from the Git archive, while its "
         "declaration counts are from the original Clang review. The **current** "
         "numbers are generated "
         "from the source token scanner, the owner-qualified field/AST review "
         "queues and the curated provisional-type queue. Lower counts generally "
         "mean fewer unresolved naming candidates; they do not measure functional "
         "decompilation completeness.\n");
    puts("| Area | Baseline | Current | Change |");
    puts("| --- | ---: | ---: | ---: |");
    for (i = 0; i < METRIC_COUNT; ++i)
        printf("| %s | %ld | %ld | %+ld |\n", metricLabels[i],
               baseline[i], inv->metrics[i],
               inv->metrics[i] - baseline[i]);
    puts("");
    printf("**Function alias coverage:** %d of %d address-named identifiers "
           "have a readable source alias; %ld remain unaliased.\n\n",
           inv->functionAliases, inv->functionCount,
           inv->metrics[FUNCTIONS_UNALIASED]);
    printf("**Numeric class alias coverage:** %d of %d have readable "
           "typedefs; %ld remain without an alias.\n\n",
           inv->classAliases, inv->classCount,
           inv->metrics[CLASSES_UNALIASED]);
    puts("## How to update this in every naming commit\n");
    puts("Run from the repository root:\n");
    puts("```sh\nsh scripts/ci/naming-progress.sh --update\n"
         "sh scripts/ci/naming-progress.sh --check\n```\n");
    puts("Commit this report and any changed inventory TSVs **with the source "
         "renames**. Git tracks the table's evolution: "
         "`git log -p -- docs/NAMING_PROGRESS.md` shows the "
         "change in each commit. The baseline is fixed in "
         "`docs/naming-progress-baseline.tsv`; do not reset it "
         "to hide regressions. The check mode verifies that generated "
         "files have been refreshed.\n");
    puts("## What is counted (and what is not)\n");
    puts("- **Files:** source basenames matching historical address patterns."
         "\n- **Functions/classes:** distinct address-coded identifiers "
         "minus those with readable source-level aliases or typedefs. "
         "The original ABI spellings often must remain for byte-exact GCC builds."
         "\n- **Provisional classes/structs:** manually recorded `open` "
         "items in [the curated queue](naming-manual-review.tsv); mark "
         "`resolved` when evidence supports the name."
         "\n- **Fields:** distinct owning type and `unk_XX` field "
         "declarations in [the field queue](naming-fields.tsv)."
         "\n- **Parameters/local variables:** short or generic declarations "
         "in [the AST review queue](naming-declarations.tsv)."
         "\n- **Other globals/numeric placeholders:** distinct token "
         "spellings from [the lexical inventory](naming-identifiers.tsv); "
         "these can overlap the local/parameter queues, so do not sum "
         "the table into one grand total.\n");
    puts("**Important limitations:** The lexical scanner does not prove that "
         "a name is meaningless. `i` in a short loop may be fine; "
         "a plausible-looking name can still be wrong. The two "
         "declaration queues were originally extracted using Clang AST and "
         "then manually pruned: **their counts change only when those TSVs "
         "are updated**. Review affected declarations after each rename, "
         "and periodically regenerate the AST queues. Zero manually "
         "flagged structs would not prove all structures are named. "
         "The VUC reconstruction is tracked independently on its own branch; "
         "its early candidate inventory is "
         "[here](naming-vuc-identifiers.tsv).\n");
    puts("For evidence, confidence levels, ABI constraints and next targets, "
         "see [the naming audit](NAMING_AUDIT.md).");
}

int main(int argc, char **argv)
{
    struct Inventory inv = {0};
    long baseline[METRIC_COUNT] = {0};
    if (argc != 7 || (strcmp(argv[1], "--baseline") &&
                      strcmp(argv[1], "--report"))) {
        fprintf(stderr, "usage: naming-progress --baseline|--report "
                "<identifiers.tsv> <fields.tsv> <declarations.tsv> "
                "<manual.tsv> <src-dir-or-baseline>\n");
        return 2;
    }
    readLexical(argv[2], &inv);
    countRows(argv[3], &inv, 0);
    countRows(argv[4], &inv, 1);
    readManual(argv[5], &inv);
    /* The source directory is a fixed relative path in both modes. */
    scanAliases("src", &inv);
    if (!strcmp(argv[1], "--baseline")) {
        printBaseline(&inv);
    } else {
        readBaseline(argv[6], baseline);
        printReport(&inv, baseline);
    }
    return 0;
}
