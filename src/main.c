#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

static int read_file(const char *path, char **buf, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return -1;
    }
    size_t cap = 4096;
    size_t n = 0;
    char *data = malloc(cap);
    if (data == NULL) {
        fclose(f);
        return -1;
    }
    size_t got;
    while ((got = fread(data + n, 1, cap - n, f)) > 0) {
        n += got;
        if (n == cap) {
            cap *= 2;
            char *tmp = realloc(data, cap);
            if (tmp == NULL) {
                free(data);
                fclose(f);
                return -1;
            }
            data = tmp;
        }
    }
    if (ferror(f)) {
        free(data);
        fclose(f);
        return -1;
    }
    fclose(f);
    *buf = data;
    *len = n;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("csv2json %s\n", APP_VERSION);
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "Использование: csv2json ФАЙЛ.csv\n");
        return 2;
    }

    char *data = NULL;
    size_t len = 0;
    if (read_file(argv[1], &data, &len) != 0) {
        fprintf(stderr, "csv2json: не удалось прочитать файл %s\n", argv[1]);
        return 1;
    }

    char *json = NULL;
    core_status st = core_convert(data, len, ',', &json);
    free(data);
    if (st != CORE_OK) {
        fprintf(stderr, "csv2json: %s\n", core_strerror(st));
        return 1;
    }
    puts(json);
    free(json);
    return 0;
}
