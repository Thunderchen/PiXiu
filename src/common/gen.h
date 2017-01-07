#ifndef GEN_H
#define GEN_H

/*
 * generator/continuation for C++
 * author: Andrew Fedoniouk @ terrainformatica.com
 * idea borrowed from: "coroutines in C" Simon Tatham
 */

struct Generator {
    int _line = 0;

    bool operator()();
};

#define $gen(name) struct name: Generator

#define $emit(T) \
bool operator()(T & _rv) { \
    switch(_line) { case 0:;

#define $yield(val) \
do { \
    _line = __LINE__; \
    _rv = val; return true; case __LINE__:; \
} while (false);

#define $stop } _line = 0; return false; }
#endif