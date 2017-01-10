#include "../proj/PiXiuStr.h"

struct CBTInter;

union CBTNode {
    CBTInter * inter;
    PiXiuChunk * outer;
};

struct CBTInter {
    CBTNode crit_0;
    CBTNode crit_1;
    uint16_t c0_idx;
    uint16_t c1_idx;

    uint16_t diff_at;
    uint8_t mask;
};

struct CritBitTree {
    CBTNode root = NULL;
};

CBTInter * CBTInter_init(void);

void CBTInter_free(CBTInter *);