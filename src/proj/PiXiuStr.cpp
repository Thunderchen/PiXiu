#include "../common/List.h"
#include "PiXiuStr.h"

static PiXiuStr * escape_unique(uint8_t[], int, bool);

PiXiuStr * PiXiuStr_init_key(uint8_t src[], int src_len) {
    return escape_unique(src, src_len, true);
};

PiXiuStr * PiXiuStr_init(uint8_t src[], int src_len) {
    return escape_unique(src, src_len, false);
};

PiXiuStr * PiXiuStr_init_stream(PXSMsg msg) {
    static int list_len;
    static int list_capacity;
    static uint8_t * list;

    static int compress_len;
    static int compress_idx;
    static int compress_to;

    auto chunk_idx = msg.chunk_idx__cmd;
    auto pxs_idx = msg.pxs_idx;
    auto msg_char = msg.val;
    PiXiuStr * ret;

    auto try_explode = [&]() {
        if (compress_len > 0) {
            if (compress_len > sizeof(PXSRecordSmall)) {
                list_len -= compress_len;

                if (compress_len > UINT8_MAX) {
                    auto record = (PXSRecordBig *) adrOf(list[list_len]);
                    record->head = PXS_UNIQUE;
                    record->sign = PXS_COMPRESS;
                    record->idx = (uint16_t) compress_idx;
                    record->to = (uint16_t) compress_to;
                    record->from = (uint16_t) (compress_to - compress_len);

                    list_len += sizeof(PXSRecordBig);
                } else {
                    auto record = (PXSRecordSmall *) adrOf(list[list_len]);
                    record->head = PXS_UNIQUE;
                    record->len = (uint8_t) compress_len;
                    record->idx = (uint16_t) compress_idx;
                    record->to = (uint16_t) compress_to;

                    list_len += sizeof(PXSRecordSmall);
                }
            }
            compress_len = 0;
        }
    };

    auto set_record = [&]() {
        compress_idx = chunk_idx;
        compress_to = pxs_idx + 1;
        compress_len++;
    };

    ret = NULL;
    switch (chunk_idx) {
        case PXS_STREAM_ON:
            list_len = 0;
            list_capacity = 2;
            list = (uint8_t *) malloc((size_t) list_capacity);

            compress_len = 0;
            break;

        case PXS_STREAM_PASS:
            try_explode();
            List_append(uint8_t, list, msg_char);
            break;

        case PXS_STREAM_OFF:
            try_explode();
            ret = (PiXiuStr *) malloc(sizeof(PiXiuStr) + list_len);
            ret->len = (uint16_t) list_len;
            assert(list_len <= PXSG_MAX_TO);
            memcpy(ret->data, list, (size_t) list_len);
            List_free(list);
            break;

        default:
            set_record();
            List_append(uint8_t, list, msg_char);
            break;
    }

    return ret;
};

PiXiuStr * PiXiuStr::concat(PiXiuStr * another) {
    auto ret = (PiXiuStr *) malloc(sizeof(PiXiuStr) + this->len + another->len);
    ret->len = this->len + another->len;
    assert(ret->len <= PXSG_MAX_TO);
    memcpy(ret->data, this->data, this->len);
    memcpy(adrOf(ret->data[this->len]), another->data, another->len);
    return ret;
}

bool PiXiuStr::key_eq(PiXiuStr * another, PiXiuChunk * ctx) {
    auto ret = false;
    auto my_gen = this->parse(0, PXSG_MAX_TO, ctx);
    auto your_gen = another->parse(0, PXSG_MAX_TO, ctx);

    auto spec_mode = false;
    uint8_t my_val, your_val;
    while (my_gen->operator()(my_val) && your_gen->operator()(your_val) && my_val == your_val) {
        if (!spec_mode && my_val == PXS_UNIQUE) { spec_mode = true; }
        else if (spec_mode) {
            if (my_val == PXS_KEY) {
                ret = true;
                break;
            } else { spec_mode = false; }
        }
    }

    PXSGen_free(my_gen);
    PXSGen_free(your_gen);
    return ret;
}

bool PiXiuStr::startswith(PiXiuStr * another, PiXiuChunk * ctx) {
    auto ret = true;

    uint8_t rv;
    auto i = 0;
    auto gen = this->parse(0, PXSG_MAX_TO, ctx);
    while (gen->operator()(rv) && i < another->len) {
        if (rv != another->data[i]) {
            ret = false;
            break;
        }
        i++;
    }
    if (i != another->len) {
        ret = false;
    }

    PXSGen_free(gen);
    return ret;
}

PXSGen * PiXiuStr::parse(int from, int to, PiXiuChunk * ctx) {
    auto gen = (PXSGen *) malloc(sizeof(PXSGen));
    gen->_line = 0;

    gen->self = this;
    gen->from = from;
    gen->to = to;
    gen->ctx = ctx;
    return gen;
}

void PiXiuChunk::delitem(int idx) {
    // todo: recycle event
    auto adr = this->strs[idx];
    assert(adr != NULL && !adr_is_spec(adr));
    this->strs[idx] = adr_mk_spec(adr);
}

bool PiXiuChunk::is_delitem(int idx) {
    auto adr = this->strs[idx];
    assert(adr != NULL);
    return adr_is_spec(adr);
}

PiXiuStr * PiXiuChunk::getitem(int idx) {
    auto adr = this->strs[idx];
    assert(adr != NULL);
    return adr_de_spec(adr);
}

void PiXiuStr_free(PiXiuStr * pxs) {
    free(pxs);
};

PiXiuChunk * PiXiuChunk_init(void) {
    return (PiXiuChunk *) calloc(1, sizeof(PiXiuChunk));
}

void PiXiuChunk_free(PiXiuChunk * chunk) {
    for (int i = 0; i < PXC_STR_NUM && chunk->strs[i] != NULL; ++i) {
        PiXiuStr_free(chunk->getitem(i));
    }
    free(chunk);
}

void PXSGen_free(PXSGen * gen) {
    free(gen);
}

static PiXiuStr * escape_unique(uint8_t src[], int src_len, bool is_key) {
    List_init(int, occur_list);

    for (int i = 0; i < src_len; ++i) {
        if (src[i] == PXS_UNIQUE) {
            List_append(int, occur_list, i);
        }
    }

    int len = src_len + occur_list_len + (is_key ? 2 : 0);
    assert(len <= PXSG_MAX_TO);
    auto pxs = (PiXiuStr *) malloc(sizeof(PiXiuStr) + len);
    pxs->len = (uint16_t) len;

    int src_cursor, ret_cursor;
    src_cursor = ret_cursor = 0;
    for (int i = 0; i < occur_list_len; ++i) {
        int cpy_len = occur_list[i] - src_cursor;
        if (cpy_len > 0) {
            memcpy(adrOf(pxs->data[ret_cursor]), adrOf(src[src_cursor]), (size_t) cpy_len);
            src_cursor += cpy_len;
            ret_cursor += cpy_len;
        }
        pxs->data[ret_cursor] = PXS_UNIQUE;
        ret_cursor++;
        pxs->data[ret_cursor] = PXS_UNIQUE;
        ret_cursor++;
        src_cursor++;
    }

    int cpy_len = src_len - src_cursor;
    if (cpy_len > 0) {
        memcpy(adrOf(pxs->data[ret_cursor]), adrOf(src[src_cursor]), (size_t) cpy_len);
        ret_cursor += cpy_len;
    }
    if (is_key) {
        pxs->data[ret_cursor] = PXS_UNIQUE;
        ret_cursor++;
        pxs->data[ret_cursor] = PXS_KEY;
    }

    List_free(occur_list);
    return pxs;
}

void t_PiXiuStr(void) {
    // --- PXC
    assert(sizeof(PiXiuStr) == 2);
    PiXiuChunk chunk;
    auto str = (PiXiuStr *) malloc(sizeof(PiXiuStr));
    str->len = 17;

    assert(chunk.used_num++ == 0);
    chunk.strs[0] = str;
    assert(chunk.getitem(0)->len == 17);
    assert(!chunk.is_delitem(0));

    chunk.delitem(0);
    assert(chunk.is_delitem(0));
    assert(chunk.getitem(0)->len == 17);
    PiXiuStr_free(str);

    auto chunk_nf = (PiXiuChunk *) malloc(sizeof(PiXiuChunk));
    for (int i = 0; i < PXC_STR_NUM; ++i) {
        chunk_nf->strs[i] = (PiXiuStr *) malloc(1);
    }
    PiXiuChunk_free(chunk_nf);
    // --- PXS
#ifndef NDEBUG
#define PXS_STREAM(...) PiXiuStr_init_stream((PXSMsg) __VA_ARGS__)

#define set_expect(...) expect = (uint8_t[]) { __VA_ARGS__ }
#define assert_out(target) for(int i=0;i<target->len;++i) assert(target->data[i]==expect[i])
#define assert_pxs() assert_out(pxs)
#endif
    // init
    uint8_t * expect;

    uint8_t input[] = {1, PXS_UNIQUE, 2, PXS_UNIQUE, 4};
    auto pxs = PiXiuStr_init_key(input, 5);
    set_expect(1, 251, 251, 2, 251, 251, 4, 251, 0);
    assert_pxs();
    PiXiuStr_free(pxs);

    pxs = PiXiuStr_init(input, 5);
    assert(pxs->len == 7);
    assert_pxs();
    PiXiuStr_free(pxs);

    auto a = PiXiuStr_init(input, 2);
    auto b = PiXiuStr_init(input, 2);
    auto merge = a->concat(b);
    assert(merge->len == 6);
    set_expect(1, 251, 251, 1, 251, 251);
    assert_out(merge);
    PiXiuStr_free(a);
    PiXiuStr_free(b);
    PiXiuStr_free(merge);
    // stream init
    assert(sizeof(PXSRecordSmall) == 6);
    assert(sizeof(PXSRecordBig) == 8);

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    for (int i = 0; i < 4; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    }
    for (int i = 0; i < 6; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 2, .pxs_idx = i, .val = 3 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    for (int i = 0; i < 7; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 3, .pxs_idx = i, .val = 4 });
    }
    pxs = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    set_expect(1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 1, 251, 7, 3, 0, 7, 0);
    assert_pxs();
    PiXiuStr_free(pxs);

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    for (int i = 0; i < 1; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    }
    for (int i = 0; i < 255; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 2, .pxs_idx = i, .val = 3 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    for (int i = 0; i < 256; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 3, .pxs_idx = i, .val = 4 });
    }
    pxs = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    set_expect(1, 251, 255, 2, 0, 255, 0, 1, 251, 1, 3, 0, 0, 1, 0, 0);
    assert_pxs();
    PiXiuStr_free(pxs);

    // parse
    assert(sizeof(PXSRecord) == sizeof(PXSRecordBig));
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = 1, .pxs_idx = 0, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = 1, .pxs_idx = 1, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 3 });
    for (int i = 0; i < 11; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 2, .pxs_idx = i, .val = 2 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 3 });
    for (int i = 1; i < 257; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = 3, .pxs_idx = i, .val = 6 });
    }
    pxs = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    set_expect(1, 251, 251, 1, 251, 251, 3, 251, 11, 2, 0, 11, 0, 3, 251, 1, 3, 0, 1, 1, 1, 0);
    assert_pxs();

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    for (int i = 0; i < 11; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 2 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 8 });
    auto pxs_i2v2 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 8 });
    for (int i = 0; i < 256; ++i) {
        PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 6 });
    }
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 8 });
    auto pxs_i3v6 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    chunk.strs[2] = pxs_i2v2;
    chunk.strs[3] = pxs_i3v6;
    auto gen = pxs->parse(1, 272, adrOf(chunk));

    uint8_t rv_arr[271];
    int i = 0;
    while (gen->operator()(rv_arr[i])) {
        i++;
    }

    i = 6;
    for (int j = 0; j < 11; ++j) {
        assert(rv_arr[i++] == 2);
    }
    assert(rv_arr[i++] == 3);
    for (int j = 0; j < 271 - 1 - 11 - 6; ++j) {
        assert(rv_arr[i++] == 6);
    }

    PXSGen_free(gen);
    PiXiuStr_free(pxs);
    PiXiuStr_free(pxs_i2v2);
    PiXiuStr_free(pxs_i3v6);

    // key equal
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_KEY });
    auto k1 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 2 });
    auto k2 = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_ON });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 1 });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_UNIQUE });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = PXS_KEY });
    PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_PASS, .val = 7 });
    auto k1_ = PXS_STREAM({ .chunk_idx__cmd = PXS_STREAM_OFF });

    assert(k1->key_eq(k1_, adrOf(chunk)));
    assert(!k1->key_eq(k2, adrOf(chunk)));
    PiXiuStr_free(k1);
    PiXiuStr_free(k2);
    PiXiuStr_free(k1_);
}