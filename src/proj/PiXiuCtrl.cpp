#include "PiXiuCtrl.h"
#include <map>
#include <string>
#include <vector>

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
    if (!reinsert && Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk
        && Glob_Reinsert_Chunk->used_num < REINSERT_RATE * PXC_STR_NUM) {
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
    if (Glob_Reinsert_Chunk != NULL && Glob_Reinsert_Chunk != this->st.cbt_chunk
        && Glob_Reinsert_Chunk->used_num < REINSERT_RATE * PXC_STR_NUM) {
        this->reinsert(Glob_Reinsert_Chunk);
    }
    RETURN_APPLY(this->cbt.delitem);
}

CBTGen * PiXiuCtrl::iter(uint8_t prefix[], int prefix_len) {
    auto pxs = PiXiuStr_init(prefix, prefix_len);
    auto ret = this->cbt.iter(pxs);
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
    auto curr_glob_chunk = Glob_Reinsert_Chunk;

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

    Glob_Reinsert_Chunk = curr_glob_chunk;
    chunk = NULL;
}

void t_PiXiuCtrl(void) {
    using namespace std;
    PiXiuCtrl pixiu_ctrl;
    srand(19950207); // 吉姆的生日

    // <max len elem>
    pixiu_ctrl.init_prop();

    uint8_t max_elem[PXSG_MAX_TO - 2];
    max_elem[0] = 233;
    for (int i = 1; i < lenOf(max_elem); ++i) max_elem[i] = 1;
    pixiu_ctrl.setitem(max_elem, lenOf(max_elem), NULL, 0);

    auto gen = pixiu_ctrl.getitem(max_elem, lenOf(max_elem));
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

    pixiu_ctrl.free_prop();
    // </>

    // <max len kv>
    pixiu_ctrl.init_prop();

    uint8_t max_k[100];
    uint8_t max_v[PXSG_MAX_TO - (lenOf(max_k) + 2) - 2];
    for (int i = 0; i < lenOf(max_k); ++i) max_k[i] = 6;
    for (int i = 0; i < lenOf(max_v); ++i) max_v[i] = 2;
    pixiu_ctrl.setitem(max_k, lenOf(max_k), max_v, lenOf(max_v));

    gen = pixiu_ctrl.getitem(max_k, lenOf(max_k));
    j = 0;
    while (gen->operator()(rv)) {
        if (0 <= j && j <= lenOf(max_k) - 1) { assert(rv == 6); }
        else { assert(rv != 6); }
        j++;
    }
    PXSGen_free(gen);

    pixiu_ctrl.free_prop();
    // </>

    // <CRUD>
    pixiu_ctrl.init_prop();
    map<string, string> cmp_map;

    string alphabet[] = {"A", "B", "C", "D", "E"};
    for (int i = 0; i < PXC_STR_NUM * 1.5; ++i) {
        // <CU>
        string sample_k;
        auto len = rand() % 50 + 1;
        for (int l = 0; l < len; ++l) {
            sample_k += alphabet[rand() % lenOf(alphabet)];
        }
        string sample_v;
        len = rand() % 50 + 1;
        for (int l = 0; l < len; ++l) {
            sample_v += alphabet[rand() % lenOf(alphabet)];
        }

        cmp_map[sample_k] = sample_v;
        pixiu_ctrl.setitem((uint8_t *) sample_k.c_str(), (int) sample_k.size(),
                           (uint8_t *) sample_v.c_str(), (int) sample_v.size());
        assert(pixiu_ctrl.contains((uint8_t *) sample_k.c_str(), (int) sample_k.size()));
        // </>

        // <R>
        auto mi = cmp_map.begin();
        advance(mi, rand() % cmp_map.size() % 100);
        sample_k = valIn(mi).first;
        sample_v = valIn(mi).second;

        auto repr = pixiu_ctrl.getitem((uint8_t *) sample_k.c_str(), (int) sample_k.size())->consume_repr();
        auto l = 0;
        for (int m = 0; m < sample_k.size(); ++m)
            assert(sample_k[m] == repr[l++]);
        for (int m = 0; m < sample_v.size(); ++m)
            assert(sample_v[m] == repr[l++]);
        free(repr);
        // </>

        // <D>
        if (rand() % 3 >= 1) {
            mi = cmp_map.begin();
            advance(mi, rand() % cmp_map.size() % 100);
            sample_k = valIn(mi).first;

            cmp_map.erase(sample_k);
            pixiu_ctrl.delitem((uint8_t *) sample_k.c_str(), (int) sample_k.size());
            assert(!pixiu_ctrl.contains((uint8_t *) sample_k.c_str(), (int) sample_k.size()));
        }
        // </>
    }

    // <遍历>
    vector<string> cmp_list;
    vector<string> tst_list;

    auto startswith_a = pixiu_ctrl.iter((uint8_t *) "A", 1);
    for (auto mi = cmp_map.begin(); mi != cmp_map.end(); ++mi) {
        auto k = valIn(mi).first;
        auto v = valIn(mi).second;
        if (k[0] != 'A') {
            assert(!startswith_a->operator()(gen));
            break;
        }
        cmp_list.push_back(k + v);

        startswith_a->operator()(gen);
        auto repr = gen->consume_repr();
        tst_list.push_back(string(repr));
        free(repr);
    }
    CBTGen_free(startswith_a);

    assert(tst_list.size() == cmp_list.size());
    sort(tst_list.begin(), tst_list.end());
    sort(cmp_list.begin(), cmp_list.end());
    for (int i = 0; i < tst_list.size(); ++i) {
        assert(tst_list[i] == cmp_list[i]);
    }
    // </>

    pixiu_ctrl.free_prop();
    // </>

    PRINT_FUNC;
}