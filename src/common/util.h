#ifndef PX_UTIL_H
#define PX_UTIL_H

#include <stdint.h>

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

#endif