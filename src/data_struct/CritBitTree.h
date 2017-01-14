#ifndef CRIT_BIT_TREE_H
#define CRIT_BIT_TREE_H

#include "../common/List.h"
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

    static void * node_sub(CBTInner * node, uint8_t direct) {
        auto ptr = node->crit_node_arr[direct];
        if (adr_is_spec(ptr)) {
            return ptr;
        } else {
            auto chunk = (PiXiuChunk *) ptr;
            return chunk->getitem(node->chunk_idx_arr[direct]);
        }
    }

    $emit(PiXiuStr *)
            auto ret = self->find_best_match(prefix);
            auto pa = (CBTInner *) ret.pa;
            auto chunk = (PiXiuChunk *) ret.crit_node;
            auto pa_direct = ret.pa_direct;

            auto chunk_idx = self->chunk_idx;
            if (pa != NULL) {
                chunk_idx = pa->chunk_idx_arr[pa_direct];
            }
            auto pxs = chunk->getitem(chunk_idx);

            if (pxs->key_eq(prefix, NULL)) {
                $yield(pxs);

                if (pa_direct != 1) {
                    List_init(void *, q);
                    List_append(void *, q, this->node_sub(pa, 1));

                    while (q_len) {
                        auto cursor = q[q_len - 1];
                        List_del(void *, q, q_len - 1);

                        if (!adr_is_spec(cursor)) {
                            $yield((PiXiuStr *) cursor);
                        } else {
                            auto inner = (CBTInner *) cursor;
                            List_append(void *, q, this->node_sub(inner, 1));
                            List_append(void *, q, this->node_sub(inner, 0));
                        }
                    }

                    List_free(q);
                }
            }
    $stop;
};

#endif