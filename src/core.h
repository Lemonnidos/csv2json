#ifndef CORE_H
#define CORE_H

#include <stddef.h>

typedef enum {
    CORE_OK = 0,
    CORE_ERR_ARGS,
    CORE_ERR_EMPTY,
    CORE_ERR_PARSE,
    CORE_ERR_MEMORY
} core_status;

core_status core_convert(const char *data, size_t len, char delimiter, char **out);
const char *core_strerror(core_status status);

#endif
