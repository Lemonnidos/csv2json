#include "core.h"

#include <stdlib.h>
#include <string.h>

#include <csv.h>
#include <cjson/cJSON.h>

typedef struct {
    cJSON *rows;
    char **header;
    size_t header_n;
    char **cells;
    size_t cells_n;
    size_t cells_cap;
    int have_header;
    core_status status;
} state;

static char *copy_field(const void *s, size_t len)
{
    char *p = malloc(len + 1);
    if (p == NULL) {
        return NULL;
    }
    if (len > 0) {
        memcpy(p, s, len);
    }
    p[len] = '\0';
    return p;
}

static void free_list(char **list, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        free(list[i]);
    }
    free(list);
}

static void on_field(void *s, size_t len, void *data)
{
    state *st = data;
    if (st->status != CORE_OK) {
        return;
    }
    if (st->cells_n == st->cells_cap) {
        size_t cap = st->cells_cap ? st->cells_cap * 2 : 8;
        char **tmp = realloc(st->cells, cap * sizeof *tmp);
        if (tmp == NULL) {
            st->status = CORE_ERR_MEMORY;
            return;
        }
        st->cells = tmp;
        st->cells_cap = cap;
    }
    char *field = copy_field(s, len);
    if (field == NULL) {
        st->status = CORE_ERR_MEMORY;
        return;
    }
    st->cells[st->cells_n++] = field;
}

static void add_record(state *st)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        st->status = CORE_ERR_MEMORY;
        return;
    }
    for (size_t i = 0; i < st->header_n; i++) {
        if (cJSON_AddStringToObject(obj, st->header[i], st->cells[i]) == NULL) {
            cJSON_Delete(obj);
            st->status = CORE_ERR_MEMORY;
            return;
        }
    }
    if (!cJSON_AddItemToArray(st->rows, obj)) {
        cJSON_Delete(obj);
        st->status = CORE_ERR_MEMORY;
    }
}

static void on_row(int c, void *data)
{
    (void)c;
    state *st = data;

    if (st->status == CORE_OK && st->cells_n > 0) {
        if (!st->have_header) {
            st->header = st->cells;
            st->header_n = st->cells_n;
            st->cells = NULL;
            st->cells_n = 0;
            st->cells_cap = 0;
            st->have_header = 1;
            return;
        }
        if (st->cells_n != st->header_n) {
            st->status = CORE_ERR_PARSE;
        } else {
            add_record(st);
        }
    }
    for (size_t i = 0; i < st->cells_n; i++) {
        free(st->cells[i]);
    }
    st->cells_n = 0;
}

core_status core_convert(const char *data, size_t len, char delimiter, char **out)
{
    if (data == NULL || out == NULL || delimiter == '\n' || delimiter == '\r'
        || delimiter == '"' || delimiter == '\0') {
        return CORE_ERR_ARGS;
    }
    *out = NULL;
    if (len == 0) {
        return CORE_ERR_EMPTY;
    }

    state st;
    memset(&st, 0, sizeof st);
    st.rows = cJSON_CreateArray();
    if (st.rows == NULL) {
        return CORE_ERR_MEMORY;
    }

    struct csv_parser parser;
    if (csv_init(&parser, 0) != 0) {
        cJSON_Delete(st.rows);
        return CORE_ERR_MEMORY;
    }
    csv_set_delim(&parser, (unsigned char)delimiter);

    size_t used = csv_parse(&parser, data, len, on_field, on_row, &st);
    if (used != len && st.status == CORE_OK) {
        st.status = CORE_ERR_PARSE;
    }
    csv_fini(&parser, on_field, on_row, &st);
    csv_free(&parser);

    if (st.status == CORE_OK && !st.have_header) {
        st.status = CORE_ERR_EMPTY;
    }
    if (st.status == CORE_OK) {
        char *text = cJSON_Print(st.rows);
        if (text == NULL) {
            st.status = CORE_ERR_MEMORY;
        } else {
            *out = text;
        }
    }

    free_list(st.header, st.header_n);
    free_list(st.cells, st.cells_n);
    cJSON_Delete(st.rows);
    return st.status;
}

const char *core_strerror(core_status status)
{
    switch (status) {
    case CORE_OK:
        return "успешно";
    case CORE_ERR_ARGS:
        return "неверные аргументы или недопустимый разделитель";
    case CORE_ERR_EMPTY:
        return "пустой ввод";
    case CORE_ERR_PARSE:
        return "ошибка разбора CSV: число полей в строке не совпадает с заголовком";
    case CORE_ERR_MEMORY:
        return "не хватило памяти";
    }
    return "неизвестная ошибка";
}

core_status core_parse_delimiter(const char *arg, char *out)
{
    if (arg == NULL || out == NULL) {
        return CORE_ERR_ARGS;
    }
    if (strcmp(arg, "\\t") == 0 || strcmp(arg, "tab") == 0) {
        *out = '\t';
        return CORE_OK;
    }
    if (strlen(arg) == 1) {
        *out = arg[0];
        return CORE_OK;
    }
    return CORE_ERR_ARGS;
}

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

core_status core_parse_config(const char *text, size_t len, char *delimiter)
{
    if (text == NULL || delimiter == NULL) {
        return CORE_ERR_ARGS;
    }
    size_t pos = 0;
    while (pos < len) {
        size_t end = pos;
        while (end < len && text[end] != '\n') {
            end++;
        }
        size_t start = pos;
        pos = end + 1;

        while (start < end && is_space(text[start])) {
            start++;
        }
        size_t stop = end;
        while (stop > start && is_space(text[stop - 1])) {
            stop--;
        }
        if (start == stop || text[start] == '#') {
            continue;
        }
        const char *eq = memchr(text + start, '=', stop - start);
        if (eq == NULL) {
            continue;
        }
        size_t eq_pos = (size_t)(eq - text);
        size_t key_stop = eq_pos;
        while (key_stop > start && is_space(text[key_stop - 1])) {
            key_stop--;
        }
        size_t value_start = eq_pos + 1;
        while (value_start < stop && is_space(text[value_start])) {
            value_start++;
        }
        if (key_stop - start == 9 && memcmp(text + start, "delimiter", 9) == 0) {
            char value[8];
            size_t value_len = stop - value_start;
            if (value_len == 0 || value_len >= sizeof value) {
                return CORE_ERR_ARGS;
            }
            memcpy(value, text + value_start, value_len);
            value[value_len] = '\0';
            core_status st = core_parse_delimiter(value, delimiter);
            if (st != CORE_OK) {
                return st;
            }
        }
    }
    return CORE_OK;
}
