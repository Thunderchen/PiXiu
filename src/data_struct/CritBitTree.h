#ifndef CRIT_BIT_TREE_H
#define CRIT_BIT_TREE_H

#include "../proj/PiXiuStr.h"

#define CBT_SET_REPLACE 1
#define CBT_DEL_NOT_FOUND 1

struct CBTGen;

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

    CBTGen * iter(PiXiuStr *);
};

CBTInner * CBTInner_init(void);

void CBTInner_free(CBTInner *);

void CBTGen_free(CBTGen *);


$gen(CBTGen) {
    CritBitTree * self;
    PiXiuStr * prefix;

    $emit(PiXiuStr *)
            auto ret = self->find_best_match(prefix);
            auto pa = (CBTInner *) ret.pa;
            auto chunk = (PiXiuChunk *) ret.crit_node;
            uint8_t pa_direct = ret.pa_direct;

            int chunk_idx = self->chunk_idx;
            if (pa != NULL) {
                chunk_idx = pa->chunk_idx_arr[pa_direct];
            }
            auto pxs = chunk->getitem(chunk_idx);

            if (pxs->key_eq(prefix, NULL)) {
                $yield(pxs);

                if (pa_direct != 1) {

                }
            }
    $stop;
};

#endif