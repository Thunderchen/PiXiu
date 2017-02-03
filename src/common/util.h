#ifndef PX_UTIL_H
#define PX_UTIL_H

#include <stdint.h>
#include <stdio.h>

#define PRINT_FUNC \
printf(__FUNCTION__); \
printf("\n");

#define char_visible(c) (33 <= (c) && (c) <= 126)
#define lenOf(arr) (sizeof(arr) / sizeof(arr[0]))

#define adr_is_spec(p) ((bool) (((intptr_t) p) & 1))

template<typename T>
T adr_mk_spec(T p) {
    return (T) (((intptr_t) p) | 1);
}

template<typename T>
T adr_de_spec(T p) {
    return (T) (((intptr_t) p) & (INTPTR_MAX - 1));
}

template<typename T>
T min(T a, T b) {
    return a < b ? a : b;
}

template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

bool startswith(const char str[], const char sub_str[], size_t sub_len) {
    return strncmp(str, sub_str, sub_len) == 0;
}

#endif