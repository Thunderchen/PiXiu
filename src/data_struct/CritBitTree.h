#ifndef CRIT_BIT_TREE_H
#define CRIT_BIT_TREE_H

#include "../proj/PiXiuStr.h"

#define CBT_SET_REPLACE 1
#define CBT_DEL_NOT_FOUND 1

struct CBTInner {
    void * crit_node_arr[2];
    uint16_t chunk_idx_arr[2];

    uint16_t diff_at;
    uint8_t mask;
};

struct CritBitTree {
    void * root = NULL;
    uint16_t chunk_idx;

    int setitem(PiXiuStr *, PiXiuChunk *, uint16_t);

    int delitem(PiXiuStr *);

    bool contains(PiXiuStr *);

    char * repr(void);

    void free_prop(void);

    struct fbm_ret {
        void * grand;
        void * pa;
        void * crit_node;
        uint8_t pa_direct;
    };

    fbm_ret find_best_match(PiXiuStr *);
};

CBTInner * CBTInner_init(void);

void CBTInner_free(CBTInner *);

#endif