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

#define PXS_STREAM(...) PiXiuStr_init_stream((PXSMsg) __VA_ARGS__)

#define PXC_STR_NUM 65535

struct PiXiuChunk;

struct PXSRecordSmall {
    uint8_t head;
    uint8_t len;
    uint16_t idx;
    uint16_t to;
};

struct PXSRecordBig {
    uint8_t head;
    uint8_t sign;
    uint16_t idx;
    uint16_t to;
    uint16_t from;
};

struct PXSMsg {
    int chunk_idx__cmd;
    int pxs_idx;
    uint8_t val;
};

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

PiXiuStr * PiXiuStr_init_stream(PXSMsg);

void PiXiuStr_free(PiXiuStr *);

#endif