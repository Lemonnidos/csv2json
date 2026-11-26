#include <stdio.h>
#include <string.h>

#include "core.h"

int main(int argc, char *argv[])
{
    if (!core_ready()) {
        return 1;
    }
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("csv2json %s\n", APP_VERSION);
        return 0;
    }
    fprintf(stderr, "Использование: csv2json --version\n");
    return 1;
}
