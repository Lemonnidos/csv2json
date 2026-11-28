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
core_status core_parse_delimiter(const char *arg, char *out);
core_status core_parse_config(const char *text, size_t len, char *delimiter);
const char *core_strerror(core_status status);

#endif
