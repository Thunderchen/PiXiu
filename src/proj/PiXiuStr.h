#ifndef PIXIU_STR_H
#define PIXIU_STR_H

#include "../common/gen.h"
#include <stdint.h>

#define PXS_UNIQUE 251
#define PXS_KEY 0
#define PXS_COMPRESS 1
#define PXS_STREAM_ON -1
#define PXS_STREAM_OFF -2
#define PXS_STREAM_PASS -3

#define PXC_STR_NUM 65535

struct PiXiuChunk;

struct PiXiuStr {
    uint16_t len;
    uint8_t data[];

    Generator parse(int, int, PiXiuChunk *);

    PiXiuStr * concat(PiXiuStr *);
};

struct PiXiuChunk {
    PiXiuStr * strs[PXC_STR_NUM];
    int used_num = 0;

    PiXiuStr * getitem(int);

    void delitem(int);

    bool is_delitem(int);
};

PiXiuStr * PiXiuStr_init_key(uint8_t[], int);

PiXiuStr * PiXiuStr_init(uint8_t[], int);

PiXiuStr * PiXiuStr_init_stream(uint8_t, int, int);

void PiXiuStr_free(PiXiuStr *);

#endif