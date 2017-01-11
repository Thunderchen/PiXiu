#include "../common/Que.h"
#include "CritBitTree.h"
#include <stddef.h>

int CBTInner::get_direct(void * cursor) {
    if (cursor == adr_de_spec(this->crit_node[0])) {
        return 0;
    } else {
        assert(cursor == adr_de_spec(this->crit_node[1]));
        return 1;
    }
};

int CritBitTree::setitem(PiXiuStr * src, PiXiuChunk * ctx, uint16_t chunk_idx) {
    if (this->root == NULL) {
        this->root = ctx;
        this->chunk_idx = chunk_idx;
    } else {
        auto ret = this->find_best_match(src);
        auto grand = (CBTInner *) ret.grand;
        auto pa = (CBTInner *) ret.pa;
        auto crit_node = (PiXiuChunk *) ret.crit_node;

        auto idx = pa->chunk_idx[pa->get_direct(crit_node)];
        auto crit_pxs = crit_node->getitem(idx);
        auto crit_gen = crit_pxs->parse(0, PXSG_MAX_TO, crit_node);
        auto src_gen = src->parse(0, PXSG_MAX_TO, NULL);

        uint8_t crit_rv, src_rv;
        uint16_t diff_at = 0;
        while (crit_gen->operator()(crit_rv) && src_gen->operator()(src_rv)) {

        }
    }
}

char * CritBitTree::repr(void) {

}

CritBitTree::fbm_ret CritBitTree::find_best_match(PiXiuStr * src) {
    auto q[] = {NULL, NULL, this->root};
    auto q_len = lenOf(q);
    auto q_cursor = 0;

    auto cursor = Que_get(q, 2);
    while (adr_is_spec(cursor)) {
        auto inner = (CBTInner *) adr_de_spec(cursor);

        uint8_t crit_byte = src->len > inner->diff_at ? src->data[inner->diff_at] : (uint8_t) 0;
        uint8_t direct = ((uint8_t) 1 + (inner->mask | crit_byte)) >> 8;
        cursor = inner->crit_node[direct];
        Que_push(q, cursor);
    }
    return CritBitTree::fbm_ret
            {adr_de_spec(Que_get(q, 0)), adr_de_spec(Que_get(q, 1)), adr_de_spec(Que_get(q, 2))};
}

CBTInner * CBTInner_init(void) {
    return (CBTInner *) malloc(offsetof(CBTInner, mask) + sizeof(CBTInner::mask));
}

void CBTInner_free(CBTInner * inner) {
    free(inner);
}