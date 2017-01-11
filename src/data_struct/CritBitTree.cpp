#include "../common/Que.h"
#include "CritBitTree.h"
#include <stddef.h>

int CritBitTree::setitem(PiXiuStr * src, PiXiuChunk * ctx, uint16_t chunk_idx) {
}

char * CritBitTree::repr(void) {
}

CritBitTree::fbm_ret CritBitTree::find_best_match(PiXiuStr * src) {
    void * q[] = {NULL, NULL, this->root};
    auto q_len = lenOf(q);
    auto q_cursor = 0;

    auto cursor = Que_get(q, 2);
    while (adr_is_spec(cursor)) {
        auto inner = (CBTInner *) adr_de_spec(cursor);

        uint8_t crit_byte = src->len > inner->diff_at ? src->data[inner->diff_at] : (uint8_t) 0;
        uint8_t direct = ((uint8_t) 1 + (inner->mask | crit_byte)) >> 8;
        Que_push(q, cursor);
        cursor = inner->crit_node[direct];
    }
    return CritBitTree::fbm_ret{.grand=Que_get(q, 0), .pa=Que_get(q, 1), .crit_node=Que_get(q, 2)};
}

CBTInner * CBTInner_init(void) {
    return (CBTInner *) malloc(offsetof(CBTInner, mask) + sizeof(CBTInner::mask));
}

void CBTInner_free(CBTInner * inner) {
    free(inner);
}