#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

static void print_usage(FILE *out)
{
    fputs("Использование:\n"
          "  csv2json convert [-d РАЗДЕЛИТЕЛЬ] [-o ФАЙЛ] ФАЙЛ.csv\n"
          "  csv2json --help\n"
          "  csv2json --version\n"
          "\n"
          "Преобразует CSV-файл с заголовком в JSON (массив объектов).\n"
          "\n"
          "Параметры команды convert:\n"
          "  -d, --delimiter СИМВОЛ  разделитель полей: один символ или \\t\n"
          "                          для табуляции (по умолчанию «,»)\n"
          "  -o, --output ФАЙЛ       записать JSON в файл, а не в stdout\n"
          "  -h, --help              показать эту справку\n",
          out);
}

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

static int write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return -1;
    }
    int rc = 0;
    if (fputs(text, f) == EOF || fputc('\n', f) == EOF) {
        rc = -1;
    }
    if (fclose(f) != 0) {
        rc = -1;
    }
    return rc;
}

static int parse_delimiter(const char *arg, char *out)
{
    if (strcmp(arg, "\\t") == 0 || strcmp(arg, "tab") == 0) {
        *out = '\t';
        return 0;
    }
    if (strlen(arg) == 1) {
        *out = arg[0];
        return 0;
    }
    return -1;
}

static int cmd_convert(int argc, char *argv[])
{
    static const struct option longopts[] = {
        {"delimiter", required_argument, NULL, 'd'},
        {"output", required_argument, NULL, 'o'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };
    char delimiter = ',';
    const char *output = NULL;
    int opt;

    while ((opt = getopt_long(argc, argv, "d:o:h", longopts, NULL)) != -1) {
        switch (opt) {
        case 'd':
            if (parse_delimiter(optarg, &delimiter) != 0) {
                fprintf(stderr, "csv2json: разделитель должен быть одним символом или \\t\n");
                return 2;
            }
            break;
        case 'o':
            output = optarg;
            break;
        case 'h':
            print_usage(stdout);
            return 0;
        default:
            print_usage(stderr);
            return 2;
        }
    }
    if (optind != argc - 1) {
        fprintf(stderr, "csv2json: укажите один входной файл\n");
        print_usage(stderr);
        return 2;
    }

    const char *input = argv[optind];
    char *data = NULL;
    size_t len = 0;
    if (read_file(input, &data, &len) != 0) {
        fprintf(stderr, "csv2json: не удалось прочитать файл %s\n", input);
        return 1;
    }

    char *json = NULL;
    core_status st = core_convert(data, len, delimiter, &json);
    free(data);
    if (st != CORE_OK) {
        fprintf(stderr, "csv2json: %s\n", core_strerror(st));
        return 1;
    }

    int rc = 0;
    if (output != NULL) {
        if (write_file(output, json) != 0) {
            fprintf(stderr, "csv2json: не удалось записать файл %s\n", output);
            rc = 1;
        }
    } else {
        puts(json);
    }
    free(json);
    return rc;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(stderr);
        return 2;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(stdout);
        return 0;
    }
    if (strcmp(argv[1], "--version") == 0) {
        printf("csv2json %s\n", APP_VERSION);
        return 0;
    }
    if (strcmp(argv[1], "convert") == 0) {
        return cmd_convert(argc - 1, argv + 1);
    }
    fprintf(stderr, "csv2json: неизвестная команда «%s»\n", argv[1]);
    print_usage(stderr);
    return 2;
}
