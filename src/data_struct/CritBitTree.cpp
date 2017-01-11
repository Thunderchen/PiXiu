#include "../common/Que.h"
#include "CritBitTree.h"
#include <stddef.h>

#define is_inner(p) adr_is_spec(p)
#define normal(p) adr_de_spec(p)

int CBTInner::get_direct(void * node) {
    if (node == normal(this->crit_node_arr[0])) {
        return 0;
    } else {
        assert(node == normal(this->crit_node_arr[1]));
        return 1;
    }
};

int CritBitTree::setitem(PiXiuStr * src, PiXiuChunk * ctx, uint16_t chunk_idx) {
    if (this->root == NULL) {
        this->root = ctx;
        this->chunk_idx = chunk_idx;
    } else {
        auto ret = this->find_best_match(src);
        auto pa = (CBTInner *) ret.pa;
        auto crit_node = (PiXiuChunk *) ret.crit_node;

        auto pa_direct = pa->get_direct(crit_node);
        auto crit_chunk_idx = pa->chunk_idx_arr[pa_direct];
        auto crit_pxs = crit_node->getitem(crit_chunk_idx);

        auto crit_gen = crit_pxs->parse(0, PXSG_MAX_TO, crit_node);
        auto src_gen = src->parse(0, PXSG_MAX_TO, NULL);

        uint16_t diff_at = 0;
        uint8_t crit_rv, src_rv;
        auto spec_mode = false;

        auto replace = [&]() {
            pa->crit_node_arr[pa_direct] = ctx;
            pa->chunk_idx_arr[pa_direct] = chunk_idx;
        };

        auto insert = [&]() {
            uint8_t mask = (crit_rv ^ src_rv);
            mask |= mask >> 1;
            mask |= mask >> 2;
            mask |= mask >> 4;
            // 0b0100_1000 => 0b0111_1111

            // 0b0111_1111 => 0b0100_0000
            mask = (mask & ~(mask >> 1)) ^ (uint8_t) UINT8_MAX;
            auto direct = (1 + (mask | src->data[diff_at])) >> 8;

            auto inner_node = CBTInner_init();
            inner_node->crit_node_arr[direct] = ctx;
            inner_node->chunk_idx_arr[diff_at] = chunk_idx;
            inner_node->diff_at = diff_at;
            inner_node->mask = mask;

            CBTInner * parent = NULL;
            auto replace_point = (CBTInner *) this->root;
            int curr_direct = -1;
            while (true) {
                if (!is_inner(replace_point)
                    || replace_point->diff_at > diff_at
                    || (replace_point->diff_at == diff_at && replace_point->mask > inner_node->mask)) {
                    break;
                }

                uint8_t crit_byte = src->len > replace_point->diff_at ? src->data[replace_point->diff_at] : (uint8_t) 0;
                curr_direct = (1 + (replace_point->mask | crit_byte)) >> 8;
                parent = replace_point;
                replace_point = (CBTInner *) replace_point->crit_node_arr[curr_direct];
            }

            if (parent == NULL) {
                this->root = inner_node;
            } else {
                assert(curr_direct >= 0);
                parent->crit_node_arr[curr_direct] = inner_node;
            }
            inner_node->crit_node_arr[(direct + 1) % 2] = replace_point;
        };

        while (crit_gen->operator()(crit_rv) && src_gen->operator()(src_rv) && crit_rv == src_rv) {
            if (!spec_mode && crit_rv == PXS_UNIQUE) { spec_mode = true; }
            else if (spec_mode) {
                if (crit_rv == PXS_KEY) {
                    replace();
                    break;
                } else { spec_mode = false; }
            }
            diff_at++;
        }
        if (!spec_mode) { insert(); };

        PXSGen_free(crit_gen);
        PXSGen_free(src_gen);
    }
}

char * CritBitTree::repr(void) {

}

CritBitTree::fbm_ret CritBitTree::find_best_match(PiXiuStr * src) {
    void * q[] = {NULL, NULL, this->root};
    auto q_len = lenOf(q);
    auto q_cursor = 0;

    auto ptr = Que_get(q, 2);
    while (is_inner(ptr)) {
        auto inner = (CBTInner *) normal(ptr);

        uint8_t crit_byte = src->len > inner->diff_at ? src->data[inner->diff_at] : (uint8_t) 0;
        uint8_t direct = ((uint8_t) 1 + (inner->mask | crit_byte)) >> 8;
        ptr = inner->crit_node_arr[direct];
        Que_push(q, ptr);
    }
    return CritBitTree::fbm_ret{normal(Que_get(q, 0)), normal(Que_get(q, 1)), normal(Que_get(q, 2))};
}

CBTInner * CBTInner_init(void) {
    return (CBTInner *) malloc(offsetof(CBTInner, mask) + sizeof(CBTInner::mask));
}

void CBTInner_free(CBTInner * inner) {
    free(inner);
}