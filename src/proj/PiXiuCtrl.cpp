#include "PiXiuCtrl.h"
#include <map>
#include <string>

#define REINSERT_RATE 0.5

extern PiXiuChunk * Glob_Reinsert_Chunk;

int PiXiuCtrl::setitem(uint8_t k[], int k_len, uint8_t v[], int v_len, bool reinsert) {
    if (this->st.local_chunk.used_num == PXC_STR_NUM) {
        auto last_chunk = this->st.cbt_chunk;
        this->st.free_prop();
        this->st.init_prop();

        if (last_chunk->used_num < REINSERT_RATE * PXC_STR_NUM) {
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
        doc->data[doc->len - 1] = PXS_KEY_SEC;
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
    this->cbt.root = NULL;
    this->st.init_prop();
}

void PiXiuCtrl::free_prop() {
    this->st.free_prop();
    this->cbt.free_prop();
}

void PiXiuCtrl::reinsert(PiXiuChunk *& chunk) {
    assert(chunk->used_num < REINSERT_RATE * PXC_STR_NUM);
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
    using namespace std;
    srand(19950207);
    PiXiuCtrl pxc;

    // <max len elem>
    pxc.init_prop();

    uint8_t max_elem[PXSG_MAX_TO - 2];
    max_elem[0] = 233;
    for (int i = 1; i < lenOf(max_elem); ++i) max_elem[i] = 1;
    pxc.setitem(max_elem, lenOf(max_elem), NULL, 0);

    auto gen = pxc.getitem(max_elem, lenOf(max_elem));
    auto j = 0;
    uint8_t rv;
    while (gen->operator()(rv)) {
        switch (j) {
            case 0:
                assert(rv == 233);
                break;
            case PXSG_MAX_TO - 1:
                assert(rv == 0);
                break;
            case PXSG_MAX_TO - 2:
                assert(rv == PXS_UNIQUE);
                break;
            default:
                assert(rv == 1);
                break;
        }
        j++;
    }
    assert(j == PXSG_MAX_TO);
    PXSGen_free(gen);

    pxc.free_prop();
    // </>

    // <max len kv>
    pxc.init_prop();

    uint8_t max_k[100];
    uint8_t max_v[PXSG_MAX_TO - (lenOf(max_k) + 2) - 2];
    for (int i = 0; i < lenOf(max_k); ++i) max_k[i] = 6;
    for (int i = 0; i < lenOf(max_v); ++i) max_v[i] = 2;
    pxc.setitem(max_k, lenOf(max_k), max_v, lenOf(max_v));

    gen = pxc.getitem(max_k, lenOf(max_k));
    j = 0;
    while (gen->operator()(rv)) {
        if (0 <= j && j <= lenOf(max_k) - 1) { assert(rv == 6); }
        else { assert(rv != 6); }
        j++;
    }
    PXSGen_free(gen);

    pxc.free_prop();
    // </>

    // <CRUD>
    pxc.init_prop();

    string alphabet[] = {"A", "B", "C", "D", "E"};
    map<string, string> ctrl_group;
    for (int i = 0; i < PXC_STR_NUM * 1.5; ++i) {
        string sample_k;
        auto len = rand() % 50 + 1;
        for (int k = 0; k < len; ++k) {
            sample_k += alphabet[rand() % lenOf(alphabet)];
        }
        sample_k += '.';
        string sample_v;
        len = rand() % 50 + 1;
        for (int k = 0; k < len; ++k) {
            sample_v += alphabet[rand() % lenOf(alphabet)];
        }
        sample_v += '.';
        pxc.setitem((uint8_t *) sample_k.c_str(), (int) sample_k.size(),
                    (uint8_t *) sample_v.c_str(), (int) sample_v.size());
    }

    pxc.free_prop();
    // </>

    PRINT_FUNC;
}