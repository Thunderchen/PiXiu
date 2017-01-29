#include "PiXiuCtrl.h"

extern PiXiuChunk * Glob_Reinsert_Chunk;

int PiXiuCtrl::setitem(uint8_t k[], int k_len, uint8_t v[], int v_len) {
#ifndef NDEBUG
    auto num = 0;
    for (int i = 0; i < k_len; ++i)
        if (k[i] == PXS_UNIQUE)
            num++;
    for (int i = 0; i < v_len; ++i)
        if (v[i] == PXS_UNIQUE)
            num++;
    assert(num + k_len + v_len + 2 + 2 <= UINT16_MAX);
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
    auto pxs_v = PiXiuStr_init_key(v, v_len);
    auto merge = pxs_k->concat(pxs_v);
    PiXiuStr_free(pxs_k);
    PiXiuStr_free(pxs_v);

    auto product = this->st.setitem(merge);
    return this->cbt.setitem(merge, product.cbt_chunk, product.idx);
}

#define RETURN_APPLY(action) \
auto pxs = PiXiuStr_init_key(k, k_len); \
auto ret = action(pxs); \
PiXiuStr_free(pxs); \
return ret;

bool PiXiuCtrl::contains(uint8_t k[], int k_len) {
    RETURN_APPLY(this->cbt.contains);
}

PXSGen * PiXiuCtrl::getitem(uint8_t k[], int k_len) {
    RETURN_APPLY(this->cbt.getitem);
}

int PiXiuCtrl::delitem(uint8_t k[], int k_len) {
    if (Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk) {
        this->reinsert(Glob_Reinsert_Chunk);
    }
    RETURN_APPLY(this->cbt.delitem);
}

CBTGen * PiXiuCtrl::iter(uint8_t prefix[], int prefix_len) {
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
    auto reserve_chunk = Glob_Reinsert_Chunk;

    for (int i = 0; i < lenOf(cbt_chunk->strs); ++i) {
        if (!cbt_chunk->is_delitem(i)) {

        }
    }
    for (int i = 0; i < lenOf(cbt_chunk->strs); ++i) {
        if (cbt_chunk->is_delitem(i)) {

        }
    }
    free(cbt_chunk);

    Glob_Reinsert_Chunk = reserve_chunk;
    cbt_chunk = NULL;
}

void t_PiXiuCtrl(void) {

}