#include "../common/Que.h"
#include "CritBitTree.h"
#include <functional>

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

        int crit_chunk_idx = this->chunk_idx;
        if (pa != NULL) {
            crit_chunk_idx = pa->chunk_idx_arr[pa_direct];
        }
        auto crit_pxs = crit_chunk->getitem(crit_chunk_idx);

        auto crit_gen = crit_pxs->parse(0, PXSG_MAX_TO, crit_chunk);
        auto src_gen = src->parse(0, PXSG_MAX_TO, NULL);
        uint16_t diff_at = 0;
        uint8_t crit_rv, src_rv;

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

            CBTInner * replace_parent = NULL;
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

                replace_parent = replace_node;
                replace_ptr = replace_node->crit_node_arr[rp_direct];
                replace_node = (CBTInner *) normal(replace_ptr);
                replace_chunk_idx = replace_parent->chunk_idx_arr[rp_direct];
            }

            if (replace_parent == NULL) {
                this->root = special(inner_node);
            } else {
                assert(rp_direct >= 0 && pa_direct <= 1);
                replace_parent->crit_node_arr[rp_direct] = special(inner_node);
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

    int crit_chunk_idx = this->chunk_idx;
    if (pa != NULL) {
        crit_chunk_idx = pa->chunk_idx_arr[pa_direct];
    }

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

    if (chunk == NULL) {
        return NULL;
    }

    int chunk_idx = this->chunk_idx;
    if (pa != NULL) {
        chunk_idx = pa->chunk_idx_arr[pa_direct];
    }
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
    auto ptr = Que_get(q, 2);
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
    auto gen = (CBTGen *) malloc(sizeof(CBTGen));
    gen->_line = 0;

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
    free(gen);
}

void t_CritBitTree(void) {
    assert(sizeof(CBTInner) == 24);
    CritBitTree cbt;
    char * output;
    char * expect;
    auto chunk = PiXiuChunk_init();

    auto cbt_insert = [&](uint8_t src[]) {
        int len;
        for (len = 0; src[len] != '\0'; ++len);
        auto pxs = PiXiuStr_init_key(src, len);
        chunk->strs[chunk->used_num] = pxs;

        cbt.setitem(pxs, chunk, chunk->used_num);
        chunk->used_num++;
        assert(cbt.contains(pxs));

        auto repr = cbt.getitem(pxs)->consume_repr();
        assert(!strcmp((char *) src, repr));
        free(repr);
    };

    cbt_insert((uint8_t *) "EC.");
    cbt_insert((uint8_t *) "ABEC.");
    cbt_insert((uint8_t *) "EJJC.");
    cbt_insert((uint8_t *) "ACD.");

    expect = (char *) "diff: 0, mask: 251\n"
            "    diff: 1, mask: 254\n"
            "        ABEC.\n"
            "        ACD.\n"
            "    diff: 1, mask: 247\n"
            "        EC.\n"
            "        EJJC.\n";
    assert(!strcmp((output = cbt.repr()), expect));
    free(output);

    auto prefix = PiXiuStr_init((uint8_t *) "E", 1);
    auto cbt_gen = cbt.iter(prefix);

    expect = (char *) "EC.EJJC.";
    auto i = 0;
    uint8_t rv;
    PXSGen * pxs_gen;
    while (cbt_gen->operator()(pxs_gen)) {
        while (pxs_gen->operator()(rv)) {
            if (char_visible(rv)) {
                assert(rv == expect[i]);
                i++;
            }
        }
        PXSGen_free(pxs_gen);
    }
    assert(i!=0);
    CBTGen_free(cbt_gen);
    PiXiuStr_free(prefix);

    auto cbt_delete = [&](uint8_t src[]) {
        int len;
        for (len = 0; src[len] != '\0'; ++len);
        auto pxs = PiXiuStr_init_key(src, len);
        cbt.delitem(pxs);
        assert(!cbt.contains(pxs));
        assert(cbt.getitem(pxs) == NULL);
        PiXiuStr_free(pxs);
    };

    cbt_delete((uint8_t *) "EJJC.");
    cbt_delete((uint8_t *) "ABEC.");

    expect = (char *) "diff: 0, mask: 251\n"
            "    ACD.\n"
            "    EC.\n";
    assert(!strcmp((output = cbt.repr()), expect));
    free(output);

    cbt_delete((uint8_t *) "EC.");
    cbt_delete((uint8_t *) "ACD.");
    assert(!strcmp(cbt.repr(), "~"));

    cbt.free_prop();
}