#include "PiXiuCtrl.h"

#define REINSERT_FACTOR 0.5

extern PiXiuChunk * Glob_Reinsert_Chunk;

int PiXiuCtrl::setitem(uint8_t k[], int k_len, uint8_t v[], int v_len, bool reinsert = false) {
    if (this->st.local_chunk.used_num == UINT16_MAX) {
        auto last_chunk = this->st.cbt_chunk;
        this->st.free_prop();
        this->st.init_prop();

        if (last_chunk->used_num < REINSERT_FACTOR * PXC_STR_NUM) {
            if (last_chunk == Glob_Reinsert_Chunk) {
                Glob_Reinsert_Chunk = NULL;
            }
            this->reinsert(last_chunk);
        }
    }
    if (!reinsert && Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk) {
        this->reinsert(Glob_Reinsert_Chunk);
    }

    PiXiuStr * doc;
    if (k_len && v_len) {
        auto pxs_k = PiXiuStr_init_key(k, k_len);
        auto pxs_v = PiXiuStr_init_key(v, v_len);
        doc = pxs_k->concat(pxs_v);
        PiXiuStr_free(pxs_k);
        PiXiuStr_free(pxs_v);
    } else if (!k_len && !v_len) {
        doc = (PiXiuStr *) k;
    } else {
        assert(k_len);
        doc = PiXiuStr_init_key(k, k_len);
    }
    auto product = this->st.setitem(doc);
    return this->cbt.setitem(doc, product.cbt_chunk, product.idx);
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
    assert(cbt_chunk->used_num < REINSERT_FACTOR * PXC_STR_NUM);
    auto chunk = Glob_Reinsert_Chunk;

    for (int i = 0; i < lenOf(cbt_chunk->strs); ++i) {
        if (!cbt_chunk->is_delitem(i)) {
            auto pxs = cbt_chunk->strs[i];
            pxs->parse(0, PXSG_MAX_TO, cbt_chunk);
        }
    }
    PiXiuChunk_free(cbt_chunk);

    Glob_Reinsert_Chunk = chunk;
    cbt_chunk = NULL;
}

void t_PiXiuCtrl(void) {

}