#include "../common/List.h"
#include "../common/util.h"
#include "PiXiuStr.h"
#include <assert.h>

static PiXiuStr * escape_unique(uint8_t[], int, bool);

PiXiuStr * PiXiuStr_init_key(uint8_t src[], int src_len) {
    return escape_unique(src, src_len, true);
};

PiXiuStr * PiXiuStr_init(uint8_t src[], int src_len) {
    return escape_unique(src, src_len, false);
};

PiXiuStr * PiXiuStr_init_stream(uint8_t msg_char, int chunk_idx, int offset) {

};

PiXiuStr * PiXiuStr::concat(PiXiuStr * another) {
    auto ret = (PiXiuStr *) malloc(sizeof(PiXiuStr) + this->len + another->len);
    ret->len = this->len + another->len;
    assert(ret->len <= 65535);
    memcpy(ret->data, this->data, this->len);
    memcpy(adrOf(ret->data[this->len]), another->data, another->len);
    return ret;
}

Generator PiXiuStr::parse(int op, int ed, PiXiuChunk * ctx) {

}

void PiXiuChunk::delitem(int idx) {
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

static PiXiuStr * escape_unique(uint8_t src[], int src_len, bool is_key) {
    List_init(int, occur_list);

    for (int i = 0; i < src_len; ++i) {
        if (src[i] == PXS_UNIQUE) {
            List_append(int, occur_list, i);
        }
    }

    int len = src_len + occur_list_len + (is_key ? 2 : 0);
    assert(len <= 65535);
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