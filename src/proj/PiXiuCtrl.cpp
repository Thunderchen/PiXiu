#include "PiXiuCtrl.h"

#define REINSERT_FACTOR 0.5

extern PiXiuChunk * Glob_Reinsert_Chunk;

int PiXiuCtrl::setitem(uint8_t k[], int k_len, uint8_t v[], int v_len, bool reinsert) {
    if (this->st.local_chunk.used_num == PXC_STR_NUM) {
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
    Glob_Reinsert_Chunk = NULL;
    this->st.init_prop();
}

void PiXiuCtrl::free_prop() {
    this->st.free_prop();
    this->cbt.free_prop();
}

void PiXiuCtrl::reinsert(PiXiuChunk *& chunk) {
    assert(chunk->used_num < REINSERT_FACTOR * PXC_STR_NUM);
    auto glob_chunk = Glob_Reinsert_Chunk;

    for (int i = 0; i < lenOf(chunk->strs); ++i) {
        if (!chunk->is_delitem(i)) {
            uint8_t decompress[PXSG_MAX_TO];
            auto gen = chunk->strs[i]->parse(0, PXSG_MAX_TO, chunk);

            uint8_t rv;
            uint16_t len = 0;
            while (gen->operator()(rv)) {
                decompress[len++] = rv;
            }
            PXSGen_free(gen);

            auto re_pxs = (PiXiuStr *) malloc(sizeof(PiXiuStr) + len);
            re_pxs->len = len;
            memcpy(re_pxs->data, decompress, len);
            this->setitem((uint8_t *) re_pxs, 0, NULL, 0, true);
        }
    }
    PiXiuChunk_free(chunk);

    Glob_Reinsert_Chunk = glob_chunk;
    chunk = NULL;
}

void t_PiXiuCtrl(void) {
    PiXiuCtrl ctrl;

    // <max len key>
    ctrl.init_prop();
    uint8_t max_len_key[PXC_STR_NUM - 2];
    max_len_key[0] = 233;
    for (int i = 1; i < lenOf(max_len_key); ++i)
        max_len_key[i] = 1;
    ctrl.setitem(max_len_key, lenOf(max_len_key), NULL, 0);

    auto gen = ctrl.getitem(max_len_key, lenOf(max_len_key));
    printf("Hey Hey\n");
    uint8_t rv;
    auto ii = 0;
    while (gen->operator()(rv)) {
        printf("%i\n", rv);
        ii++;
    }
    printf("Num is %i\n", ii);
    PXSGen_free(gen);

    ctrl.free_prop();
    // </>

    PRINT_FUNC;
}