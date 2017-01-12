#include "../proj/PiXiuStr.h"

#define CBT_SET_REPLACE 1

struct CBTInner {
    void * crit_node_arr[2];
    uint16_t chunk_idx_arr[2];

    uint16_t diff_at;
    uint8_t mask;

    int get_direct(void *);
};

struct CritBitTree {
    void * root = NULL;
    uint16_t chunk_idx;

    int setitem(PiXiuStr *, PiXiuChunk *, uint16_t);

    char * repr(void);

    struct fbm_ret {
        void * grand;
        void * pa;
        void * crit_node;
    };

    fbm_ret find_best_match(PiXiuStr *);
};

CBTInner * CBTInner_init(void);

void CBTInner_free(CBTInner *);

void CritBitTree_free(CritBitTree *);