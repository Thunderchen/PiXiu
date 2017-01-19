#include "PiXiuCtrl.h"

extern PiXiuChunk * Glob_Reinsert_Chunk;

int PiXiuCtrl::setitem(uint8_t * k, int k_len, uint8_t * v, int v_len) {
#ifndef NDEBUG
    auto num = 0;
    for (int i = 0; i < k_len; ++i) {
        if (k[i] == PXS_UNIQUE) {
            num++;
        }
    }
    for (int i = 0; i < v_len; ++i) {
        if (v[i] == PXS_UNIQUE) {
            num++;
        }
    }
    assert(num + k_len + v_len + 2 <= UINT16_MAX);
#endif

    if (this->st.local_chunk.used_num == UINT16_MAX) {
        auto last_chunk = this->st.cbt_chunk;
        this->st.free_prop();
        this->st.init_prop();

        if (last_chunk->used_num < 0.8 * PXC_STR_NUM) {
            if (last_chunk == Glob_Reinsert_Chunk) {
                Glob_Reinsert_Chunk = NULL;
            }
            this->reinsert(last_chunk);
        }
    }
    if (Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk) {
        this->reinsert(Glob_Reinsert_Chunk);
    }

    auto pxs_k = PiXiuStr_init_key(k, k_len);
    auto pxs_v = PiXiuStr_init(v, v_len);
    auto pxs = pxs_k->concat(pxs_v);
    PiXiuStr_free(pxs_k);
    PiXiuStr_free(pxs_v);

    auto product = this->st.setitem(pxs);
    return this->cbt.setitem(pxs, product.cbt_chunk, product.idx);
}

#define RETURN_KEY_APPLY(action) \
assert(k ## _len + 2 <= UINT16_MAX); \
auto pxs = PiXiuStr_init_key(k, k ## _len); \
auto ret = action(pxs); \
PiXiuStr_free(pxs); \
return ret;

bool PiXiuCtrl::contains(uint8_t * k, int k_len) {
    RETURN_KEY_APPLY(this->cbt.contains);
}

PXSGen * PiXiuCtrl::getitem(uint8_t * k, int k_len) {
    RETURN_KEY_APPLY(this->cbt.getitem);
}

int PiXiuCtrl::delitem(uint8_t * k, int k_len) {
    if (Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk) {
        this->reinsert(Glob_Reinsert_Chunk);
    }
    RETURN_KEY_APPLY(this->cbt.delitem);
}

CBTGen * PiXiuCtrl::iter(uint8_t * prefix, int prefix_len) {
    assert(prefix_len + 2 <= UINT16_MAX);
    auto pxs = PiXiuStr_init(prefix, prefix_len);
    auto ret = this->cbt.iter(pxs);
    PiXiuStr_free(pxs);
    return ret;
}

void PiXiuCtrl::init_prop() {
    this->st.init_prop();
}

void PiXiuCtrl::free_prop() {
    this->st.free_prop();
    this->cbt.free_prop();
}

void PiXiuCtrl::reinsert(PiXiuChunk *& cbt_chunk) {
    assert(cbt_chunk->used_num < 0.8 * PXC_STR_NUM);
    auto reserve = Glob_Reinsert_Chunk;

    for (int i = 0; cbt_chunk->used_num > 0; ++i) {
        assert(i <= PXC_STR_NUM - 1);
        if (cbt_chunk->is_delitem(i)) {
            PiXiuStr_free(cbt_chunk->getitem(i));
        } else {
            auto pxs = cbt_chunk->getitem(i);
            this->delitem(pxs->data, pxs->len);
            Glob_Reinsert_Chunk = NULL;

            auto product = this->st.setitem(pxs);
            this->cbt.setitem(pxs, product.cbt_chunk, product.idx);
            assert(Glob_Reinsert_Chunk == NULL);
        }
    }
    free(cbt_chunk);

    Glob_Reinsert_Chunk = reserve;
    cbt_chunk = NULL;
}