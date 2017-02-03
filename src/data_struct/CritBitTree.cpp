#include "../common/Que.h"
#include "CritBitTree.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

#define is_inner(p) adr_is_spec(p)
#define normal(p) adr_de_spec(p)
#define special(p) adr_mk_spec(p)

int CritBitTree::setitem(PiXiuStr * src, PiXiuChunk * ctx, uint16_t chunk_idx) {
    auto sign = 0;
    if (this->root == NULL) {
        this->root = ctx;
        this->chunk_idx = chunk_idx;
    } else {
        auto ret = this->find_best_match(src);
        auto pa = (CBTInner *) ret.pa;
        auto crit_chunk = (PiXiuChunk *) ret.crit_node;
        uint8_t pa_direct = ret.pa_direct;

        int crit_chunk_idx = pa == NULL ? this->chunk_idx : pa->chunk_idx_arr[pa_direct];
        auto crit_pxs = crit_chunk->getitem(crit_chunk_idx);

        auto crit_gen = crit_pxs->parse(0, PXSG_MAX_TO, crit_chunk);
        auto src_gen = src->parse(0, PXSG_MAX_TO, NULL);
        uint16_t diff_at = 0;
        uint8_t crit_rv = 0, src_rv = 0;

        auto replace = [&]() {
            sign = CBT_SET_REPLACE;
            crit_chunk->delitem(crit_chunk_idx);
            if (pa != NULL) {
                assert(pa_direct >= 0 && pa_direct <= 1);
                pa->crit_node_arr[pa_direct] = ctx;
                pa->chunk_idx_arr[pa_direct] = chunk_idx;
            } else {
                this->root = ctx;
                this->chunk_idx = chunk_idx;
            }
        };

        auto insert = [&]() {
            uint8_t mask = (crit_rv ^ src_rv);
            mask |= mask >> 1;
            mask |= mask >> 2;
            mask |= mask >> 4;
            // 0b0100_1000 => 0b0111_1111

            // 0b0111_1111 => 0b0100_0000
            mask = (mask & ~(mask >> 1)) ^ (uint8_t) UINT8_MAX;
            uint8_t direct = ((uint8_t) 1 + (mask | src->data[diff_at])) >> 8;

            auto inner_node = CBTInner_init();
            inner_node->crit_node_arr[direct] = ctx;
            inner_node->chunk_idx_arr[direct] = chunk_idx;
            inner_node->diff_at = diff_at;
            inner_node->mask = mask;

            CBTInner * rp_parent = NULL;
            uint8_t rp_direct = 3;
            auto replace_ptr = this->root;
            auto replace_node = (CBTInner *) normal(replace_ptr);
            auto replace_chunk_idx = this->chunk_idx;
            while (true) {
                if (!is_inner(replace_ptr)
                    || replace_node->diff_at > diff_at
                    || (replace_node->diff_at == diff_at && replace_node->mask > inner_node->mask)) {
                    break;
                }

                uint8_t crit_byte = src->len > replace_node->diff_at ? src->data[replace_node->diff_at] : (uint8_t) 0;
                rp_direct = ((uint8_t) 1 + (replace_node->mask | crit_byte)) >> 8;
                rp_parent = replace_node;

                replace_ptr = replace_node->crit_node_arr[rp_direct];
                replace_node = (CBTInner *) normal(replace_ptr);
                replace_chunk_idx = rp_parent->chunk_idx_arr[rp_direct];
            }

            if (rp_parent == NULL) {
                this->root = special(inner_node);
            } else {
                assert(rp_direct >= 0 && pa_direct <= 1);
                rp_parent->crit_node_arr[rp_direct] = special(inner_node);
            }
            auto temp_i = (direct + 1) % 2;
            inner_node->crit_node_arr[temp_i] = replace_ptr;
            inner_node->chunk_idx_arr[temp_i] = replace_chunk_idx;
        };

        auto spec_mode = false;
        while (crit_gen->operator()(crit_rv) && src_gen->operator()(src_rv) && crit_rv == src_rv) {
            PXSG_SEE_KEY_BREAK(crit_rv, replace());
            diff_at++;
        }
        if (!spec_mode) { insert(); };

        PXSGen_free(crit_gen);
        PXSGen_free(src_gen);
    }
    return sign;
}

int CritBitTree::delitem(PiXiuStr * src) {
    auto sign = CBT_DEL_NOT_FOUND;
    if (this->root == NULL) {
        return sign;
    }

    auto ret = this->find_best_match(src);
    auto grand = (CBTInner *) ret.grand;
    auto pa = (CBTInner *) ret.pa;
    auto crit_chunk = (PiXiuChunk *) ret.crit_node;
    uint8_t pa_direct = ret.pa_direct;
    int crit_chunk_idx = pa == NULL ? this->chunk_idx : pa->chunk_idx_arr[pa_direct];

    auto case_del = [&]() {
        sign = 0;
        if (pa == NULL) {
            this->root = NULL;
        } else {
            auto temp_i = (1 + pa_direct) % 2;
            if (grand == NULL) {
                this->root = pa->crit_node_arr[temp_i];
                this->chunk_idx = pa->chunk_idx_arr[temp_i];
            } else {
                auto grand_direct = normal(grand->crit_node_arr[0]) == pa ? 0 : 1;
                grand->crit_node_arr[grand_direct] = pa->crit_node_arr[temp_i];
                grand->chunk_idx_arr[grand_direct] = pa->chunk_idx_arr[temp_i];
            }
            free(pa);
        }
        crit_chunk->delitem(crit_chunk_idx);
    };

    auto crit_pxs = crit_chunk->getitem(crit_chunk_idx);
    auto crit_gen = crit_pxs->parse(0, PXSG_MAX_TO, crit_chunk);

    uint8_t rv;
    auto i = 0;
    auto spec_mode = false;
    while (crit_gen->operator()(rv) && i < src->len && rv == src->data[i]) {
        i++;
        PXSG_SEE_KEY_BREAK(rv, case_del());
    }

    PXSGen_free(crit_gen);
    return sign;
};

bool CritBitTree::contains(PiXiuStr * src) {
    auto sign = false;
    if (this->root == NULL) {
        return sign;
    }

    auto fmb_ret = this->find_best_match(src);
    auto pa = (CBTInner *) fmb_ret.pa;
    auto crit_chunk = (PiXiuChunk *) fmb_ret.crit_node;
    uint8_t pa_direct = fmb_ret.pa_direct;

    auto crit_pxs = crit_chunk->getitem(pa == NULL ? this->chunk_idx : pa->chunk_idx_arr[pa_direct]);
    auto crit_gen = crit_pxs->parse(0, PXSG_MAX_TO, crit_chunk);

    uint8_t rv;
    auto i = 0;
    auto spec_mode = false;
    while (crit_gen->operator()(rv) && i < src->len && rv == src->data[i]) {
        i++;
        PXSG_SEE_KEY_BREAK(rv, sign = true);
    }

    free(crit_gen);
    return sign;
};

PXSGen * CritBitTree::getitem(PiXiuStr * src) {
    if (this->root == NULL) {
        return NULL;
    }

    auto ret = this->find_best_match(src);
    auto pa = (CBTInner *) ret.pa;
    auto chunk = (PiXiuChunk *) ret.crit_node;
    uint8_t pa_direct = ret.pa_direct;

    int chunk_idx = pa == NULL ? this->chunk_idx : pa->chunk_idx_arr[pa_direct];
    auto pxs = chunk->getitem(chunk_idx);
    if (pxs->key_eq(src, chunk)) {
        return pxs->parse(0, PXSG_MAX_TO, chunk);
    }
    return NULL;
};

char * CritBitTree::repr() {
    List_init(char, output);

    std::function<void(void *, int)> print = [&](void * ptr, int lv) {
        int intent = 4 * lv;
        for (int i = 0; i < intent; ++i) {
            List_append(char, output, ' ');
        }

        if (!is_inner(ptr)) {
            auto pxs = (PiXiuStr *) ptr;
            for (int i = 0; i < pxs->len; ++i) {
                if (char_visible(pxs->data[i])) {
                    List_append(char, output, pxs->data[i]);
                }
            }
            List_append(char, output, '\n');
            return;
        }

        auto inner = (CBTInner *) normal(ptr);
        char temp[50];
        sprintf(temp, "diff: %i, mask: %i", inner->diff_at, inner->mask);
        for (int i = 0; temp[i] != '\0'; ++i) {
            List_append(char, output, temp[i]);
        }
        List_append(char, output, '\n');

        lv++;
        for (int i = 0; i < 2; ++i) {
            auto sub_ptr = inner->crit_node_arr[i];
            if (is_inner(sub_ptr)) { print(sub_ptr, lv); }
            else { print(((PiXiuChunk *) sub_ptr)->getitem(inner->chunk_idx_arr[i]), lv); }
        }
    };

    if (this->root == NULL) {
        List_append(char, output, '~');
    } else if (is_inner(this->root)) {
        print(this->root, 0);
    } else {
        print(((PiXiuChunk *) this->root)->getitem(this->chunk_idx), 0);
    }
    List_append(char, output, '\0');
    return output;
}

void CritBitTree::free_prop() {
    if (this->root == NULL) {
        return;
    }
    if (is_inner(this->root)) { CBTInner_free((CBTInner *) normal(this->root)); }
    else { PiXiuChunk_free((PiXiuChunk *) this->root); }
}

CritBitTree::fbm_ret CritBitTree::find_best_match(PiXiuStr * src) {
    void * q[] = {NULL, NULL, this->root};
    auto q_len = lenOf(q);
    auto q_cursor = 0;

    uint8_t direct = 3;
    auto ptr = Que_get(q, q_len - 1);
    while (is_inner(ptr)) {
        auto inner = (CBTInner *) normal(ptr);

        uint8_t crit_byte = src->len > inner->diff_at ? src->data[inner->diff_at] : (uint8_t) 0;
        direct = ((uint8_t) 1 + (inner->mask | crit_byte)) >> 8;
        ptr = inner->crit_node_arr[direct];
        Que_push(q, ptr);
    }
    return CritBitTree::fbm_ret{normal(Que_get(q, 0)), normal(Que_get(q, 1)), normal(Que_get(q, 2)), direct};
}

CBTGen * CritBitTree::iter(PiXiuStr * prefix) {
    if (this->root == NULL) {
        return NULL;
    }
    auto gen = (CBTGen *) malloc(sizeof(CBTGen));
    gen->_line = 0;
    gen->helper = NULL;

    gen->self = this;
    gen->prefix = prefix;
    return gen;
};

CBTInner * CBTInner_init(void) {
    return (CBTInner *) malloc(offsetof(CBTInner, mask) + sizeof(CBTInner::mask));
}

void CBTInner_free(CBTInner * inner) {
    for (int i = 0; i < 2; ++i) {
        auto sub_ptr = inner->crit_node_arr[i];
        if (is_inner(sub_ptr)) {
            CBTInner_free((CBTInner *) normal(sub_ptr));
        } else {
            auto chunk = (PiXiuChunk *) sub_ptr;
            chunk->used_num--;
            if (chunk->used_num == 0) { PiXiuChunk_free(chunk); }
        }
    }
    free(inner);
}

void CBTGen_free(CBTGen * gen) {
    if (gen->helper != NULL) {
        gen->helper->free_prop();
        free(gen->helper);
    }
    PiXiuStr_free(gen->prefix);
    free(gen);
}

#define SAMPLE_PXS PiXiuStr_init_key((uint8_t *) sample.c_str(), (int) sample.size())

void t_CritBitTree(void) {
    using namespace std;
    assert(sizeof(CBTInner) == 24);

    string alphabet[] = {"A", "B", "C", "D", "E"};
    map<string, int> ctrl;
    CritBitTree test;
    auto test_ctx = PiXiuChunk_init();

    //<spec cases>
    //  <replace root>
    auto foo = PiXiuStr_init_key((uint8_t *) "K", 1);
    test_ctx->strs[0] = foo;
    auto bar = PiXiuStr_init_key((uint8_t *) "K", 1);
    test_ctx->strs[1] = bar;
    assert(test.setitem(foo, test_ctx, 0) == 0);
    assert(test.setitem(bar, test_ctx, 1) == CBT_SET_REPLACE);
    assert(test.contains(bar) && test_ctx->is_delitem(0));
    //  </>

    //  <del_empty>
    test.delitem(bar);
    assert(test.delitem(NULL) == CBT_DEL_NOT_FOUND);
    //  </>
    //  <get_empty>
    assert(test.getitem(NULL) == NULL);
    //  </>
    //  <repr_empty>
    free(test.repr());
    //  </>

    //  <no_exist>
    foo = PiXiuStr_init_key((uint8_t *) "KDA123", 6);
    test_ctx->strs[2] = foo;
    bar = PiXiuStr_init_key((uint8_t *) "KDA321", 6);
    test_ctx->strs[3] = bar;
    test.setitem(foo, test_ctx, 2);
    //    <repr_one>
    free(test.repr());
    //    </>
    test.setitem(bar, test_ctx, 3);

    auto temp = PiXiuStr_init((uint8_t *) "K", 1);
    assert(test.getitem(temp) == NULL);
    //    <interrupt iter>
    auto iter = test.iter(temp);
    PXSGen * g;
    iter->operator()(g);
    PXSGen_free(g);
    CBTGen_free(iter);
    //    </>
    //  </>

    test.free_prop();
    test.root = NULL;
    // <free_empty>
    test.free_prop();
    // </>
    // <iter_empty>
    assert(test.iter(NULL) == NULL);
    // </>
    test_ctx = PiXiuChunk_init();
    //</>

    for (uint16_t i = 0; i < 1000; ++i) {
        // <写入>
        string sample;
        auto len = rand() % 10;
        for (int j = 0; j < len; ++j) {
            sample += alphabet[rand() % lenOf(alphabet)];
        }
        sample += '.';
        ctrl[sample] = 1;

        auto sample_pxs = SAMPLE_PXS;
        test_ctx->strs[i] = sample_pxs;
        test.setitem(sample_pxs, test_ctx, i);
        test_ctx->used_num++;
        assert(test.contains(sample_pxs));
        // </>

        // <删除>
        if (rand() % 2) {
            auto mi = ctrl.begin();
            advance(mi, rand() % ctrl.size());
            sample = valIn(mi).first;
            ctrl.erase(sample);

            sample_pxs = SAMPLE_PXS;
            assert(test.contains(sample_pxs));
            test.delitem(sample_pxs);
            assert(!test.contains(sample_pxs));
            PiXiuStr_free(sample_pxs);
        }
        // </>
    }

    // <读取>
    auto read_times = ctrl.size() / 2;
    for (int i = 0; i < read_times; ++i) {
        auto mi = ctrl.begin();
        advance(mi, rand() % (read_times * 2));
        string sample = valIn(mi).first;

        auto sample_pxs = SAMPLE_PXS;
        auto exist = test.getitem(sample_pxs)->consume_repr();
        assert(!strcmp(sample.c_str(), exist));
        free(exist);
        PiXiuStr_free(sample_pxs);
    }
    // </>

    // <遍历>
    vector<string> ctrl_startswith_c;
    for (auto mi = ctrl.begin(); mi != ctrl.end(); ++mi) {
        auto str = valIn(mi).first;
        if (str[0] == 'C') {
            ctrl_startswith_c.push_back(str);
        }
    }

    List_init(char *, test_startswith_c);
    auto query_pxs = PiXiuStr_init((uint8_t[]) {'C'}, 1);
    auto query_gen = test.iter(query_pxs);
    PXSGen * rv;
    while (query_gen->operator()(rv)) {
        List_append(char *, test_startswith_c, rv->consume_repr());
    }

    assert(test_startswith_c_len == ctrl_startswith_c.size());
    for (int i = 0; i < test_startswith_c_len; ++i) {
        assert(!strcmp(test_startswith_c[i], ctrl_startswith_c[i].c_str()));
    }
    auto none_pxs = PiXiuStr_init((uint8_t[]) {'F'}, 1);
    auto none_gen = test.iter(none_pxs);
    assert(!none_gen->operator()(rv));

    for (int i = 0; i < test_startswith_c_len; ++i) {
        List_free(test_startswith_c[i]);
    }
    List_free(test_startswith_c);
    CBTGen_free(query_gen);
    CBTGen_free(none_gen);
    // </>

    free(test.repr());
    test.free_prop();
    PRINT_FUNC;
}