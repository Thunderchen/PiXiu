#ifndef LIST_H
#define LIST_H

#include "style.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define List_init(T, name) \
int name ## _len = 0; \
int name ## _capacity = 2; \
T ptrAs(name) = (T *) malloc(sizeof(T) * name ## _capacity);

#define List_append(T, name, val) \
if (name ## _len >= name ## _capacity) { \
    name ## _capacity = (int) (name ## _capacity * 1.5); \
    name = (T *) realloc(name, sizeof(T) * name ## _capacity); \
} \
name[name ## _len] = val; \
name ## _len++;

#define List_del(T, name, idx) \
assert(name ## _len > idx); \
if (idx + 1 < name ## _len) { \
    memmove(adrOf(name[idx]), adrOf(name[idx + 1]), sizeof(T) * (name ## _len - (idx + 1))); \
} \
name ## _len--;

#define List_insert(T, name, idx, val) \
assert(name ## _len >= idx); \
if (idx == name ## _len) { \
    List_append(T, name, val); \
} else { \
    if (name ## _len >= name ## _capacity) { \
        name ## _capacity = (int) (name ## _capacity * 1.5); \
        T ptrAs(_ ## name) = (T *) malloc(sizeof(T) * name ## _capacity); \
        if (idx > 0) { \
            memcpy(_ ## name, name, sizeof(T) * idx); \
        } \
        _ ## name[idx] = val; \
        memcpy(adrOf(_ ## name[idx + 1]), adrOf(name[idx]), sizeof(T) * (name ## _len - idx)); \
        free(name); \
        name = _ ## name; \
    } else { \
        memmove(adrOf(name[idx + 1]), adrOf(name[idx]), sizeof(T) * (name ## _len - idx)); \
        name[idx] = val; \
    } \
    name ## _len++; \
}

#define List_free(name) free(name);
#endif