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

    bool contains(PiXiuStr *);

    PXSGen * getitem(PiXiuStr *);

    CBTGen * iter(PiXiuStr *);

    int delitem(PiXiuStr *);

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

void CBTGen_free(CBTGen *);


$gen(CBTGHelper) {
    PiXiuStr * prefix;
    bool * harvest;
    void * ptr;

    CBTGHelper * sub;
    CBTInner * cursor;

    uint16_t chunk_idx;
    uint8_t direct;
    uint8_t direct_end;
    bool include_all;

    $emit(PXSGen *)
            PiXiuChunk * chunk;
            PiXiuStr * pxs;
            PXSGen * rv;
            uint8_t crit_byte;

            if (!adr_is_spec(ptr)) {
                chunk = (PiXiuChunk *) ptr;
                pxs = chunk->getitem(chunk_idx);
                if (valIn(harvest)) {
                    $yield(pxs->parse(0, PXSG_MAX_TO, chunk));
                } else if (pxs->startswith(prefix, chunk)) {
                    valIn(harvest) = true;
                    $yield(pxs->parse(0, PXSG_MAX_TO, chunk));
                }
                goto stop;
            }

            cursor = (CBTInner *) adr_de_spec(ptr);
            crit_byte = prefix->len > cursor->diff_at ? prefix->data[cursor->diff_at] : (uint8_t) 0;
            direct = ((uint8_t) 1 + (cursor->mask | crit_byte)) >> 8;

            if (!include_all && cursor->diff_at >= prefix->len) {
                include_all = true;
            }
            if (include_all) {
                direct = 0;
                direct_end = 2;
            } else {
                direct_end = direct + (uint8_t) 1;
            }

            for (; direct < direct_end; ++direct) {
                sub = (CBTGHelper *) malloc(sizeof(CBTGHelper));
                sub->init_prop(prefix, harvest, cursor->crit_node_arr[direct], cursor->chunk_idx_arr[direct],
                               include_all);
                while (sub->operator()(rv)) {
                    $yield(rv);
                }
                sub->free_prop();
                free(sub);
                sub = NULL;
            }
    $stop;

    void init_prop(PiXiuStr * prefix, bool * harvest, void * ptr, uint16_t chunk_idx, bool include_all) {
        this->_line = 0;
        this->sub = NULL;

        this->prefix = prefix;
        this->harvest = harvest;
        this->ptr = ptr;
        this->chunk_idx = chunk_idx;
        this->include_all = include_all;
    }

    void free_prop(void) {
        if (this->sub != NULL) {
            this->sub->free_prop();
            free(this->sub);
        }
    }
};

$gen(CBTGen) {
    CritBitTree * self;
    PiXiuStr * prefix;

    CBTGHelper * helper;
    bool harvest;

    $emit(PXSGen *)
            PXSGen * rv;

            harvest = false;
            helper = (CBTGHelper *) malloc(sizeof(CBTGHelper));
            helper->init_prop(prefix, adrOf(harvest), self->root, self->chunk_idx, false);
            while (helper->operator()(rv)) {
                $yield(rv);
            }
            helper->free_prop();
            free(helper);
            helper = NULL;
    $stop;
};

#endif