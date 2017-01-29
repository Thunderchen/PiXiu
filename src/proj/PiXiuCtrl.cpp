#include "PiXiuCtrl.h"

extern PiXiuChunk * Glob_Reinsert_Chunk;

int PiXiuCtrl::setitem(uint8_t * k, int k_len, uint8_t * v, int v_len) {
#ifndef NDEBUG
    auto num = 0;
    for (int i = 0; i < k_len; ++i)
        if (k[i] == PXS_UNIQUE)
            num++;
    for (int i = 0; i < v_len; ++i)
        if (v[i] == PXS_UNIQUE)
            num++;
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

//    auto pxs_k = PiXiuStr_init_key(k, k_len);
    auto pxs_v = PiXiuStr_init(v, v_len);
//    auto pxs = pxs_k->concat(pxs_v);
//    PiXiuStr_free(pxs_k);
//    PiXiuStr_free(pxs_v);
    for (int i = 0; i < pxs_v->len; ++i) {
        printf("%c", pxs_v->data[i]);
    }
    printf("\n");

    auto product = this->st.setitem(pxs_v);
    auto out = product.cbt_chunk->getitem(product.idx)->parse(0, PXSG_MAX_TO, product.cbt_chunk)->consume_repr();
    for (int i = 0; out[i] != '\0'; i++) {
        assert(out[i] == pxs_v->data[i]);
    }
    return this->cbt.setitem(pxs_v, product.cbt_chunk, product.idx);
}

#define RETURN_APPLY(action) \
assert(k ## _len + 2 <= UINT16_MAX); \
auto pxs = PiXiuStr_init_key(k, k ## _len); \
auto ret = action(pxs); \
PiXiuStr_free(pxs); \
return ret;

bool PiXiuCtrl::contains(uint8_t * k, int k_len) {
    RETURN_APPLY(this->cbt.contains);
}

PXSGen * PiXiuCtrl::getitem(uint8_t * k, int k_len) {
    RETURN_APPLY(this->cbt.getitem);
}

int PiXiuCtrl::delitem(uint8_t * k, int k_len) {
    if (Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk) {
        this->reinsert(Glob_Reinsert_Chunk);
    }
    RETURN_APPLY(this->cbt.delitem);
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
            this->cbt.delitem(pxs);
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

#define rand_range(max) ((int) ((double) rand() / (double) RAND_MAX * (double) (max)))

void t_PiXiuCtrl(void) {
    Glob_Reinsert_Chunk = NULL;

    srand(19950207);
    List_init(PiXiuStr *, key_keep);
    List_init(PiXiuStr *, value_keep);
    PiXiuCtrl ctrl;
    ctrl.init_prop();

    uint8_t alphabet[] = {'A', 'B', 'C', 'D', 'E'};
    uint8_t key[5];
    uint8_t value[20];

    auto gen_rand = [&](uint8_t * des, int max_len) -> int {
        auto rand_len = rand_range(max_len - 1) + 1;
        for (int i = 0; i < rand_len; ++i) {
            des[i] = alphabet[(rand_range(lenOf(alphabet) - 1))];
        }
        return rand_len;
    };

    // 随机写入
    for (int i = 0; i < PXC_STR_NUM * 2; ++i) {
//        auto k_len = gen_rand(key, lenOf(key));
        auto v_len = gen_rand(value, lenOf(value));
//        List_append(PiXiuStr *, key_keep, PiXiuStr_init_key(key, k_len));
        List_append(PiXiuStr *, value_keep, PiXiuStr_init(value, v_len));
//        ctrl.setitem(key, k_len, value, v_len);
        SuffixTree g;
        g.init_prop();
        for (int j = 0; j < value_keep[value_keep_len - 1]->len; ++j) {
            printf("%c", value_keep[value_keep_len - 1]->data[j]);
        }
        printf("\n");
        g.setitem(value_keep[value_keep_len - 1]);
        g.free_prop();

    }

    List_free(key_keep);
    List_free(value_keep);
    ctrl.free_prop();
}