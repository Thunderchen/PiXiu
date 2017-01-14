#ifndef PX_UTIL_H
#define PX_UTIL_H

#include <stdint.h>

#define char_visible(c) (33 <= (c) && (c) <= 126)

#define lenOf(arr) (sizeof(arr) / sizeof(arr[0]))

#define adr_is_spec(p) ((bool) (((intptr_t) p) & 1))

template<typename T>
T adr_mk_spec(T p) {
    p = (T) (((intptr_t) p) | 1);
    return p;
}

template<typename T>
T adr_de_spec(T p) {
    return adr_is_spec(p) ? (T) (((intptr_t) p) ^ 1) : p;
}

template<typename T>
T min(T a, T b) {
    return a < b ? a : b;
}

template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

#endif