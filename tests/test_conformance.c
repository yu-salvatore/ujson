#include "ujson.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 4096
#define MAX_FILE_SIZE 65535 /* unsigned short max */

static int read_file(const char *path, char *buf, size_t bufsz, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        return -1;
    }

    *out_len = fread(buf, 1, bufsz, f);
    fclose(f);
    return 0;
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(void)
{
    const char *dir = TEST_PARSING_DIR;
    DIR *d = opendir(dir);
    if (!d)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return 1;
    }

    /* Collect filenames for sorted iteration */
    char *names[512];
    int nfiles = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && nfiles < 512)
    {
        size_t len = strlen(ent->d_name);
        if (len > 5 && strcmp(ent->d_name + len - 5, ".json") == 0)
        {
            names[nfiles] = strdup(ent->d_name);
            nfiles++;
        }
    }
    closedir(d);

    qsort(names, (size_t)nfiles, sizeof(char *), cmp_str);

    int pass = 0, fail = 0, skip = 0;
    char path[1024];
    char buf[MAX_FILE_SIZE + 1];
    uj_tok tokens[MAX_TOKENS];

    for (int i = 0; i < nfiles; i++)
    {
        const char *name = names[i];
        char prefix = name[0];

        snprintf(path, sizeof(path), "%s/%s", dir, name);

        size_t flen;
        if (read_file(path, buf, MAX_FILE_SIZE, &flen) != 0)
        {
            printf("[JSONTestSuite] %-60s SKIP (read error)\n", name);
            skip++;
            continue;
        }

        if (flen > MAX_FILE_SIZE)
        {
            printf("[JSONTestSuite] %-60s SKIP (file too large)\n", name);
            skip++;
            continue;
        }

        uj_t parser;
        uj_init(&parser);
        int rc = uj_parse(&parser, buf, (unsigned short)flen, tokens, MAX_TOKENS);

        if (prefix == 'y')
        {
            /* Must accept */
            if (rc > 0)
            {
                printf("[JSONTestSuite] %-60s PASS\n", name);
                pass++;
            }
            else if (rc == UJ_ENOMEM)
            {
                printf("[JSONTestSuite] %-60s SKIP (token limit)\n", name);
                skip++;
            }
            else
            {
                printf("[JSONTestSuite] %-60s FAIL (expected accept, got %d)\n", name, rc);
                fail++;
            }
        }
        else if (prefix == 'n')
        {
            /* Must reject */
            if (rc < 0)
            {
                printf("[JSONTestSuite] %-60s PASS\n", name);
                pass++;
            }
            else
            {
                printf("[JSONTestSuite] %-60s FAIL (expected reject, got %d)\n", name, rc);
                fail++;
            }
        }
        else if (prefix == 'i')
        {
            /* Implementation-defined */
            printf("[JSONTestSuite] %-60s SKIP (%s, rc=%d)\n", name,
                   rc > 0 ? "accepted" : "rejected", rc);
            skip++;
        }
        else
        {
            printf("[JSONTestSuite] %-60s SKIP (unknown prefix)\n", name);
            skip++;
        }

        free(names[i]);
    }

    printf("\nTotal: %d | Pass: %d | Fail: %d | Skip: %d\n",
           nfiles, pass, fail, skip);

    return fail > 0 ? 1 : 0;
}
