#ifndef PIXIU_STR_H
#define PIXIU_STR_H

#include "../common/gen.h"
#include <stdint.h>

#define CHUNK_STR_NUM 65535

struct PiXiuChunk;

struct PiXiuStr {
    uint16_t len;
    uint8_t data[];

    Generator parse(int, int, PiXiuChunk *);

    PiXiuStr * concat(PiXiuStr *);
};

struct PiXiuChunk {
    PiXiuStr * strs[CHUNK_STR_NUM];
    int used_num = 0;

    PiXiuStr * getitem(int);

    void delitem(int);

    bool is_delitem(int);
};

PiXiuStr * PiXiuStr_init_key(uint8_t[], int);

PiXiuStr * PiXiuStr_init(uint8_t[], int);

PiXiuStr * PiXiuStr_init_stream(uint8_t, int, int);

#endif