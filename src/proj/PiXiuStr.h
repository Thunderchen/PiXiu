#include <stdint.h>

#define CHUNK_PXS_NUM 65535

struct PiXiuStr {
    uint16_t len;
    uint8_t data[];
};

struct PiXiuChunk {
    PiXiuStr * strs[CHUNK_PXS_NUM];
    int used_num;
};